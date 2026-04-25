#include "Cookie/Core/Application.h"
#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/EngineConfig.h"
#include "Cookie/Platform/PlatformLibrary.h"
#include "Cookie/Platform/PlatformPaths.h"
#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/RendererConfig.h"
#include "Cookie/Physics/JoltPhysicsBackend.h"
#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <filesystem>
#include <memory>
#include <string>

namespace {

using RendererCreateFn = cookie::renderer::IRendererBackend* (*)();
using RendererDestroyFn = void (*)(cookie::renderer::IRendererBackend*);

constexpr const char* kCreateRendererSymbol = "CookieCreateRendererBackend";
constexpr const char* kDestroyRendererSymbol = "CookieDestroyRendererBackend";

std::filesystem::path ResolveModulePath(
    const std::filesystem::path& root, const std::string& configured_path) {
  if (configured_path.empty()) {
    return {};
  }

  const std::filesystem::path configured(configured_path);
  if (configured.is_absolute()) {
    return configured;
  }

  return root / configured;
}

class DynamicRendererProxy final : public cookie::renderer::IRendererBackend {
 public:
  DynamicRendererProxy(
      cookie::platform::LibraryHandle module,
      cookie::renderer::IRendererBackend* backend, RendererDestroyFn destroy)
      : module_(module), backend_(backend), destroy_(destroy) {}

  ~DynamicRendererProxy() override {
    if (backend_ && destroy_) {
      destroy_(backend_);
    }
    if (module_) {
      cookie::platform::UnloadDynamicLibrary(module_);
    }
  }

  bool Initialize() override {
    return backend_ && backend_->Initialize();
  }

  bool BeginFrame() override {
    return backend_ && backend_->BeginFrame();
  }

  void Clear(const cookie::renderer::ClearColor& color) override {
    if (backend_) {
      backend_->Clear(color);
    }
  }

  void EndFrame() override {
    if (backend_) {
      backend_->EndFrame();
    }
  }

  void Shutdown() override {
    if (backend_) {
      backend_->Shutdown();
    }
  }

  std::string_view Name() const override {
    if (!backend_) {
      return "invalid";
    }
    return backend_->Name();
  }

 private:
  cookie::platform::LibraryHandle module_ = nullptr;
  cookie::renderer::IRendererBackend* backend_ = nullptr;
  RendererDestroyFn destroy_ = nullptr;
};

std::unique_ptr<cookie::renderer::IRendererBackend> TryCreateRendererFromModule(
    const std::filesystem::path& module_path) {
  const auto module = cookie::platform::LoadDynamicLibrary(module_path);
  if (!module) {
    return nullptr;
  }

  const auto create_fn = reinterpret_cast<RendererCreateFn>(
      cookie::platform::GetLibrarySymbol(module, kCreateRendererSymbol));
  const auto destroy_fn = reinterpret_cast<RendererDestroyFn>(
      cookie::platform::GetLibrarySymbol(module, kDestroyRendererSymbol));
  if (!create_fn || !destroy_fn) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  auto* backend = create_fn();
  if (!backend) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  return std::make_unique<DynamicRendererProxy>(module, backend, destroy_fn);
}

std::unique_ptr<cookie::renderer::IRendererBackend> CreateRendererBackend(
    const std::string& backend_name, const cookie::core::EngineConfig& engine_config,
    const cookie::core::StartupPaths& paths) {
  if (backend_name == "dx11") {
    const auto from_project_root =
        ResolveModulePath(paths.project_root, engine_config.renderer_dx11_module);
    if (auto module_backend = TryCreateRendererFromModule(from_project_root)) {
      return module_backend;
    }

    const auto from_runtime_dir = ResolveModulePath(
        cookie::platform::GetExecutableDirectory(),
        engine_config.renderer_dx11_module);
    if (auto module_backend = TryCreateRendererFromModule(from_runtime_dir)) {
      return module_backend;
    }

    // Transitional fallback: keep static factory path available while module
    // boundaries are being migrated.
    return cookie::renderer::dx11::CreateRendererDX11Backend();
  }

  return nullptr;
}

} // namespace

int main() {
  const cookie::core::StartupPaths paths = cookie::core::DiscoverStartupPaths();
  const cookie::core::EngineConfig engine_config =
      cookie::core::LoadEngineConfig(paths.engine_config);
  const cookie::renderer::RendererConfig renderer_config =
      cookie::renderer::LoadRendererConfig(paths.graphics_config);
  auto backend =
      CreateRendererBackend(renderer_config.backend_name, engine_config, paths);
  auto physics_backend = cookie::physics::CreateJoltPhysicsBackend();

  cookie::core::Application app({
      .application_name = engine_config.runtime_name,
      .renderer_backend_name = renderer_config.backend_name,
      .window_title = renderer_config.window_title,
      .window_width = renderer_config.window_width,
      .window_height = renderer_config.window_height,
      .max_frames = renderer_config.max_frames,
      .clear_color = renderer_config.clear_color,
  }, std::move(physics_backend), std::move(backend));

  return app.Run();
}

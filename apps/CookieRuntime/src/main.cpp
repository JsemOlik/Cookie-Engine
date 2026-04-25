#include "Cookie/Core/Application.h"
#include "Cookie/Core/AudioBackend.h"
#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/EngineConfig.h"
#include "Cookie/Core/PhysicsBackend.h"
#include "Cookie/Audio/NullAudioBackend.h"
#include "Cookie/Platform/PlatformLibrary.h"
#include "Cookie/Platform/PlatformPaths.h"
#include "Cookie/Physics/JoltPhysicsBackend.h"
#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/RendererConfig.h"
#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <filesystem>
#include <memory>
#include <string>

namespace {

using RendererCreateFn = cookie::renderer::IRendererBackend* (*)();
using RendererDestroyFn = void (*)(cookie::renderer::IRendererBackend*);
using AudioCreateFn = cookie::core::IAudioBackend* (*)();
using AudioDestroyFn = void (*)(cookie::core::IAudioBackend*);
using PhysicsCreateFn = cookie::core::IPhysicsBackend* (*)();
using PhysicsDestroyFn = void (*)(cookie::core::IPhysicsBackend*);
using CoreModuleVersionFn = int (*)();
using CoreModuleNameFn = const char* (*)();

constexpr const char* kCoreVersionSymbol = "CookieCoreModuleApiVersion";
constexpr const char* kCoreNameSymbol = "CookieCoreModuleName";
constexpr const char* kCreateRendererSymbol = "CookieCreateRendererBackend";
constexpr const char* kDestroyRendererSymbol = "CookieDestroyRendererBackend";
constexpr const char* kCreateAudioSymbol = "CookieCreateAudioBackend";
constexpr const char* kDestroyAudioSymbol = "CookieDestroyAudioBackend";
constexpr const char* kCreatePhysicsSymbol = "CookieCreatePhysicsBackend";
constexpr const char* kDestroyPhysicsSymbol = "CookieDestroyPhysicsBackend";

struct RendererBootstrapResult {
  std::unique_ptr<cookie::renderer::IRendererBackend> backend;
  std::string runtime_source;
  std::string module_path;
};

struct PhysicsBootstrapResult {
  std::unique_ptr<cookie::core::IPhysicsBackend> backend;
  std::string runtime_source;
  std::string module_path;
};

struct AudioBootstrapResult {
  std::unique_ptr<cookie::core::IAudioBackend> backend;
  std::string runtime_source;
  std::string module_path;
};

struct CoreBootstrapResult {
  std::string runtime_source;
  std::string module_path;
  std::string module_name;
  int module_api_version = 0;
};

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

class DynamicPhysicsProxy final : public cookie::core::IPhysicsBackend {
 public:
  DynamicPhysicsProxy(
      cookie::platform::LibraryHandle module,
      cookie::core::IPhysicsBackend* backend, PhysicsDestroyFn destroy)
      : module_(module), backend_(backend), destroy_(destroy) {}

  ~DynamicPhysicsProxy() override {
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

  cookie::core::PhysicsStepStats StepSimulation(
      const cookie::core::PhysicsStepConfig& config) override {
    if (!backend_) {
      return {};
    }
    return backend_->StepSimulation(config);
  }

  void Shutdown() override {
    if (backend_) {
      backend_->Shutdown();
    }
  }

 private:
  cookie::platform::LibraryHandle module_ = nullptr;
  cookie::core::IPhysicsBackend* backend_ = nullptr;
  PhysicsDestroyFn destroy_ = nullptr;
};

class DynamicAudioProxy final : public cookie::core::IAudioBackend {
 public:
  DynamicAudioProxy(
      cookie::platform::LibraryHandle module,
      cookie::core::IAudioBackend* backend, AudioDestroyFn destroy)
      : module_(module), backend_(backend), destroy_(destroy) {}

  ~DynamicAudioProxy() override {
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

  void Update(const cookie::core::AudioUpdateConfig& config) override {
    if (backend_) {
      backend_->Update(config);
    }
  }

  void Shutdown() override {
    if (backend_) {
      backend_->Shutdown();
    }
  }

 private:
  cookie::platform::LibraryHandle module_ = nullptr;
  cookie::core::IAudioBackend* backend_ = nullptr;
  AudioDestroyFn destroy_ = nullptr;
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

std::unique_ptr<cookie::core::IPhysicsBackend> TryCreatePhysicsFromModule(
    const std::filesystem::path& module_path) {
  const auto module = cookie::platform::LoadDynamicLibrary(module_path);
  if (!module) {
    return nullptr;
  }

  const auto create_fn = reinterpret_cast<PhysicsCreateFn>(
      cookie::platform::GetLibrarySymbol(module, kCreatePhysicsSymbol));
  const auto destroy_fn = reinterpret_cast<PhysicsDestroyFn>(
      cookie::platform::GetLibrarySymbol(module, kDestroyPhysicsSymbol));
  if (!create_fn || !destroy_fn) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  auto* backend = create_fn();
  if (!backend) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  return std::make_unique<DynamicPhysicsProxy>(module, backend, destroy_fn);
}

std::unique_ptr<cookie::core::IAudioBackend> TryCreateAudioFromModule(
    const std::filesystem::path& module_path) {
  const auto module = cookie::platform::LoadDynamicLibrary(module_path);
  if (!module) {
    return nullptr;
  }

  const auto create_fn = reinterpret_cast<AudioCreateFn>(
      cookie::platform::GetLibrarySymbol(module, kCreateAudioSymbol));
  const auto destroy_fn = reinterpret_cast<AudioDestroyFn>(
      cookie::platform::GetLibrarySymbol(module, kDestroyAudioSymbol));
  if (!create_fn || !destroy_fn) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  auto* backend = create_fn();
  if (!backend) {
    cookie::platform::UnloadDynamicLibrary(module);
    return nullptr;
  }

  return std::make_unique<DynamicAudioProxy>(module, backend, destroy_fn);
}

RendererBootstrapResult CreateRendererBackend(
    const std::string& backend_name, const cookie::core::EngineConfig& engine_config,
    const cookie::core::StartupPaths& paths) {
  if (backend_name == "dx11") {
    const auto from_project_root =
        ResolveModulePath(paths.project_root, engine_config.renderer_dx11_module);
    if (auto module_backend = TryCreateRendererFromModule(from_project_root)) {
      return {
          .backend = std::move(module_backend),
          .runtime_source = "module",
          .module_path = from_project_root.string(),
      };
    }

    const auto from_runtime_dir = ResolveModulePath(
        cookie::platform::GetExecutableDirectory(),
        engine_config.renderer_dx11_module);
    if (auto module_backend = TryCreateRendererFromModule(from_runtime_dir)) {
      return {
          .backend = std::move(module_backend),
          .runtime_source = "module",
          .module_path = from_runtime_dir.string(),
      };
    }

    // Transitional fallback: keep static factory path available while module
    // boundaries are being migrated.
    return {
        .backend = cookie::renderer::dx11::CreateRendererDX11Backend(),
        .runtime_source = "static-fallback",
    };
  }

  return {};
}

PhysicsBootstrapResult CreatePhysicsBackend(
    const cookie::core::EngineConfig& engine_config,
    const cookie::core::StartupPaths& paths) {
  const auto from_project_root =
      ResolveModulePath(paths.project_root, engine_config.physics_module);
  if (auto module_backend = TryCreatePhysicsFromModule(from_project_root)) {
    return {
        .backend = std::move(module_backend),
        .runtime_source = "module",
        .module_path = from_project_root.string(),
    };
  }

  const auto from_runtime_dir = ResolveModulePath(
      cookie::platform::GetExecutableDirectory(), engine_config.physics_module);
  if (auto module_backend = TryCreatePhysicsFromModule(from_runtime_dir)) {
    return {
        .backend = std::move(module_backend),
        .runtime_source = "module",
        .module_path = from_runtime_dir.string(),
    };
  }

  return {
      .backend = cookie::physics::CreateJoltPhysicsBackend(),
      .runtime_source = "static-fallback",
  };
}

AudioBootstrapResult CreateAudioBackend(
    const cookie::core::EngineConfig& engine_config,
    const cookie::core::StartupPaths& paths) {
  const auto from_project_root =
      ResolveModulePath(paths.project_root, engine_config.audio_module);
  if (auto module_backend = TryCreateAudioFromModule(from_project_root)) {
    return {
        .backend = std::move(module_backend),
        .runtime_source = "module",
        .module_path = from_project_root.string(),
    };
  }

  const auto from_runtime_dir = ResolveModulePath(
      cookie::platform::GetExecutableDirectory(), engine_config.audio_module);
  if (auto module_backend = TryCreateAudioFromModule(from_runtime_dir)) {
    return {
        .backend = std::move(module_backend),
        .runtime_source = "module",
        .module_path = from_runtime_dir.string(),
    };
  }

  return {
      .backend = cookie::audio::CreateNullAudioBackend(),
      .runtime_source = "static-fallback",
  };
}

CoreBootstrapResult ProbeCoreModule(
    const cookie::core::EngineConfig& engine_config,
    const cookie::core::StartupPaths& paths) {
  const auto probe = [&](const std::filesystem::path& module_path) -> CoreBootstrapResult {
    const auto module = cookie::platform::LoadDynamicLibrary(module_path);
    if (!module) {
      return {};
    }

    const auto version_fn = reinterpret_cast<CoreModuleVersionFn>(
        cookie::platform::GetLibrarySymbol(module, kCoreVersionSymbol));
    const auto name_fn = reinterpret_cast<CoreModuleNameFn>(
        cookie::platform::GetLibrarySymbol(module, kCoreNameSymbol));
    if (!version_fn || !name_fn) {
      cookie::platform::UnloadDynamicLibrary(module);
      return {};
    }

    const char* module_name = name_fn();
    CoreBootstrapResult result{
        .runtime_source = "module",
        .module_path = module_path.string(),
        .module_name = module_name ? module_name : "unknown",
        .module_api_version = version_fn(),
    };
    cookie::platform::UnloadDynamicLibrary(module);
    return result;
  };

  const auto from_project_root =
      ResolveModulePath(paths.project_root, engine_config.core_module);
  if (auto result = probe(from_project_root); !result.runtime_source.empty()) {
    return result;
  }

  const auto from_runtime_dir =
      ResolveModulePath(cookie::platform::GetExecutableDirectory(), engine_config.core_module);
  if (auto result = probe(from_runtime_dir); !result.runtime_source.empty()) {
    return result;
  }

  return {
      .runtime_source = "static-fallback",
  };
}

} // namespace

int main() {
  const cookie::core::StartupPaths paths = cookie::core::DiscoverStartupPaths();
  const cookie::core::EngineConfig engine_config =
      cookie::core::LoadEngineConfig(paths.engine_config);
  const auto core_bootstrap = ProbeCoreModule(engine_config, paths);
  const cookie::renderer::RendererConfig renderer_config =
      cookie::renderer::LoadRendererConfig(paths.graphics_config);
  auto renderer_bootstrap =
      CreateRendererBackend(renderer_config.backend_name, engine_config, paths);
  auto audio_bootstrap = CreateAudioBackend(engine_config, paths);
  auto physics_bootstrap = CreatePhysicsBackend(engine_config, paths);

  cookie::core::Application app({
      .application_name = engine_config.runtime_name,
      .core_runtime_source = core_bootstrap.runtime_source,
      .core_module_path = core_bootstrap.module_path,
      .core_module_name = core_bootstrap.module_name,
      .core_module_api_version = core_bootstrap.module_api_version,
      .renderer_backend_name = renderer_config.backend_name,
      .renderer_runtime_source = renderer_bootstrap.runtime_source,
      .renderer_module_path = renderer_bootstrap.module_path,
      .physics_runtime_source = physics_bootstrap.runtime_source,
      .physics_module_path = physics_bootstrap.module_path,
      .audio_runtime_source = audio_bootstrap.runtime_source,
      .audio_module_path = audio_bootstrap.module_path,
      .window_title = renderer_config.window_title,
      .window_width = renderer_config.window_width,
      .window_height = renderer_config.window_height,
      .max_frames = renderer_config.max_frames,
      .clear_color = renderer_config.clear_color,
  }, std::move(audio_bootstrap.backend), std::move(physics_bootstrap.backend),
     std::move(renderer_bootstrap.backend));

  return app.Run();
}

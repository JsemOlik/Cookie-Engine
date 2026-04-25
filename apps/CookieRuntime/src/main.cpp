#include "Cookie/Core/Application.h"
#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/RendererConfig.h"
#include "Cookie/Physics/JoltPhysicsBackend.h"
#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <memory>
#include <string>

namespace {

std::unique_ptr<cookie::renderer::IRendererBackend> CreateRendererBackend(
    const std::string& backend_name) {
  if (backend_name == "dx11") {
    return cookie::renderer::dx11::CreateRendererDX11Backend();
  }

  return nullptr;
}

} // namespace

int main() {
  const cookie::core::StartupPaths paths = cookie::core::DiscoverStartupPaths();
  const cookie::renderer::RendererConfig renderer_config =
      cookie::renderer::LoadRendererConfig(paths.graphics_config);
  auto backend = CreateRendererBackend(renderer_config.backend_name);
  auto physics_backend = cookie::physics::CreateJoltPhysicsBackend();

  cookie::core::Application app({
      .application_name = "CookieRuntime",
      .renderer_backend_name = renderer_config.backend_name,
      .window_title = renderer_config.window_title,
      .window_width = renderer_config.window_width,
      .window_height = renderer_config.window_height,
      .max_frames = renderer_config.max_frames,
      .clear_color = renderer_config.clear_color,
  }, std::move(physics_backend), std::move(backend));

  return app.Run();
}

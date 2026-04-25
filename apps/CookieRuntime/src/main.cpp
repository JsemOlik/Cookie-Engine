#include "Cookie/Core/Application.h"
#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/RendererConfig.h"
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

  cookie::core::Application app({
      .application_name = "CookieRuntime",
      .renderer_backend_name = renderer_config.backend_name,
  }, std::move(backend));

  return app.Run();
}

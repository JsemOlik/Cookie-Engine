#include "Cookie/Core/Application.h"

#include <string>
#include <utility>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/Logger.h"

namespace cookie::core {

Application::Application(
    ApplicationConfig config,
    std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend)
    : config_(std::move(config)), renderer_backend_(std::move(renderer_backend)) {}

int Application::Run() const {
  const StartupPaths paths = DiscoverStartupPaths();
  Logger logger(paths.log_file);

  if (!logger.IsReady()) {
    return 1;
  }

  logger.Info("CookieRuntime startup sequence initialized.");
  logger.Info("Application: " + config_.application_name);
  logger.Info("Selected renderer backend: " + config_.renderer_backend_name);
  logger.Info("Project root: " + paths.project_root.string());
  logger.Info("Config directory: " + paths.config_dir.string());
  logger.Info("Engine config: " + paths.engine_config.string());
  logger.Info("Graphics config: " + paths.graphics_config.string());
  logger.Info("Input config: " + paths.input_config.string());
  logger.Info("Game config: " + paths.game_config.string());

  if (!renderer_backend_) {
    logger.Error("No renderer backend instance is available.");
    return 2;
  }

  logger.Info("Initializing renderer backend: " +
              std::string(renderer_backend_->Name()));
  if (!renderer_backend_->Initialize()) {
    logger.Error("Renderer backend failed to initialize.");
    return 3;
  }

  logger.Info("Renderer backend initialized successfully.");
  renderer_backend_->Shutdown();
  logger.Info("Renderer backend shut down successfully.");
  logger.Info("Phase 2 skeleton complete. Renderer interface and DX11 module wired.");

  return 0;
}

} // namespace cookie::core

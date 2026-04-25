#include "Cookie/Core/Application.h"

#include <string>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/Logger.h"

namespace cookie::core {

Application::Application(ApplicationConfig config) : config_(std::move(config)) {}

int Application::Run() const {
  const StartupPaths paths = DiscoverStartupPaths();
  Logger logger(paths.log_file);

  if (!logger.IsReady()) {
    return 1;
  }

  logger.Info("CookieRuntime startup sequence initialized.");
  logger.Info("Application: " + config_.application_name);
  logger.Info("Project root: " + paths.project_root.string());
  logger.Info("Config directory: " + paths.config_dir.string());
  logger.Info("Engine config: " + paths.engine_config.string());
  logger.Info("Graphics config: " + paths.graphics_config.string());
  logger.Info("Input config: " + paths.input_config.string());
  logger.Info("Game config: " + paths.game_config.string());
  logger.Info("Phase 1 skeleton complete. No renderer/editor/physics loaded.");

  return 0;
}

} // namespace cookie::core

#include "Cookie/Core/ConfigPaths.h"

#include "Cookie/Platform/PlatformPaths.h"

namespace cookie::core {

StartupPaths DiscoverStartupPaths() {
  StartupPaths paths{};
  paths.project_root = cookie::platform::GetProjectRoot();

  paths.config_dir = paths.project_root / "config";
  paths.logs_dir = paths.project_root / "logs";
  paths.log_file = paths.logs_dir / "latest.log";
  paths.engine_config = paths.config_dir / "engine.json";
  paths.graphics_config = paths.config_dir / "graphics.json";
  paths.input_config = paths.config_dir / "input.cfg";
  paths.game_config = paths.config_dir / "game.json";

  return paths;
}

} // namespace cookie::core

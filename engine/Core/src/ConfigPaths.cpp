#include "Cookie/Core/ConfigPaths.h"

#include "Cookie/Platform/PlatformPaths.h"

namespace cookie::core {
namespace {

bool LooksLikeExportedGameRoot(const std::filesystem::path& candidate) {
  return std::filesystem::exists(candidate / "config" / "engine.json") &&
         std::filesystem::exists(candidate / "config" / "graphics.json") &&
         std::filesystem::exists(candidate / "content") &&
         std::filesystem::exists(candidate / "logs");
}

} // namespace

StartupPaths DiscoverStartupPaths() {
  StartupPaths paths{};
  const auto executable_dir = cookie::platform::GetExecutableDirectory();
  if (!executable_dir.empty() && LooksLikeExportedGameRoot(executable_dir)) {
    paths.project_root = executable_dir;
  } else {
    paths.project_root = cookie::platform::GetProjectRoot();
  }

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

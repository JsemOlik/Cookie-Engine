#pragma once

#include <filesystem>
#include <string>

namespace cookie::core {

struct GameConfig {
  std::string game_name = "CookieGame";
  std::string startup_scene_asset_id = "2e5d1005d0f0496f8c4e8f86d2318d14";
};

GameConfig LoadGameConfig(const std::filesystem::path& game_config_path);
bool SaveGameConfig(const std::filesystem::path& game_config_path,
                    const GameConfig& config);

}  // namespace cookie::core

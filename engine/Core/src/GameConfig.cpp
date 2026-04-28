#include "Cookie/Core/GameConfig.h"

#include <fstream>
#include <iterator>

namespace cookie::core {
namespace {

std::string ExtractStringValue(const std::string& source, std::string key_name) {
  const std::string key = "\"" + std::move(key_name) + "\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return {};
  }

  const std::size_t colon_position = source.find(':', key_position + key.size());
  if (colon_position == std::string::npos) {
    return {};
  }

  const std::size_t first_quote = source.find('"', colon_position + 1);
  if (first_quote == std::string::npos) {
    return {};
  }

  const std::size_t second_quote = source.find('"', first_quote + 1);
  if (second_quote == std::string::npos || second_quote <= first_quote + 1) {
    return {};
  }

  return source.substr(first_quote + 1, second_quote - first_quote - 1);
}

}  // namespace

GameConfig LoadGameConfig(const std::filesystem::path& game_config_path) {
  GameConfig config{};
  std::ifstream file(game_config_path);
  if (!file.is_open()) {
    return config;
  }

  const std::string contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  const std::string parsed_game_name = ExtractStringValue(contents, "game_name");
  if (!parsed_game_name.empty()) {
    config.game_name = parsed_game_name;
  }

  const std::string parsed_startup_scene_asset_id =
      ExtractStringValue(contents, "startup_scene_asset_id");
  if (!parsed_startup_scene_asset_id.empty()) {
    config.startup_scene_asset_id = parsed_startup_scene_asset_id;
  } else {
    const std::string legacy_startup_scene =
        ExtractStringValue(contents, "startup_scene");
    if (!legacy_startup_scene.empty()) {
      config.startup_scene_asset_id = legacy_startup_scene;
    }
  }

  return config;
}

bool SaveGameConfig(const std::filesystem::path& game_config_path,
                    const GameConfig& config) {
  std::error_code error;
  std::filesystem::create_directories(game_config_path.parent_path(), error);
  if (error) {
    return false;
  }

  std::ofstream file(game_config_path, std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  file << "{\n";
  file << "  \"game_name\": \"" << config.game_name << "\",\n";
  file << "  \"startup_scene_asset_id\": \"" << config.startup_scene_asset_id
       << "\"\n";
  file << "}\n";
  return true;
}

}  // namespace cookie::core

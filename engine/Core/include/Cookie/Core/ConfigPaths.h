#pragma once

#include <filesystem>

namespace cookie::core {

struct StartupPaths {
  std::filesystem::path project_root;
  std::filesystem::path config_dir;
  std::filesystem::path logs_dir;
  std::filesystem::path log_file;
  std::filesystem::path engine_config;
  std::filesystem::path graphics_config;
  std::filesystem::path input_config;
  std::filesystem::path game_config;
};

StartupPaths DiscoverStartupPaths();

} // namespace cookie::core

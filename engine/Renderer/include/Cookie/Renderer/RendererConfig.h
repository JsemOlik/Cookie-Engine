#pragma once

#include <filesystem>
#include <string>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

struct RendererConfig {
  std::string backend_name = "dx11";
  std::string window_title = "CookieRuntime";
  std::string camera_mode = "perspective";
  int window_width = 1280;
  int window_height = 720;
  ClearColor clear_color{};
};

RendererConfig LoadRendererConfig(const std::filesystem::path& graphics_config_path);

} // namespace cookie::renderer

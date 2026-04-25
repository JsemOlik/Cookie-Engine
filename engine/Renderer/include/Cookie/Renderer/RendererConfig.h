#pragma once

#include <filesystem>
#include <string>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

struct RendererConfig {
  std::string backend_name = "dx11";
  std::string window_title = "CookieRuntime";
  int window_width = 1280;
  int window_height = 720;
  int max_frames = 0;
  std::string camera_mode = "orthographic";
  float camera_ortho_height = 2.8f;
  float camera_perspective_fov_degrees = 60.0f;
  float camera_near_plane = 0.0f;
  float camera_far_plane = 1.0f;
  ClearColor clear_color{};
};

RendererConfig LoadRendererConfig(const std::filesystem::path& graphics_config_path);

} // namespace cookie::renderer

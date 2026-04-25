#pragma once

#include <filesystem>
#include <string>

namespace cookie::renderer {

struct RendererConfig {
  std::string backend_name = "dx11";
};

RendererConfig LoadRendererConfig(const std::filesystem::path& graphics_config_path);

} // namespace cookie::renderer

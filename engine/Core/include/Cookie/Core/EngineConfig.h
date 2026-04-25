#pragma once

#include <filesystem>
#include <string>

namespace cookie::core {

struct EngineConfig {
  std::string runtime_name = "CookieRuntime";
  std::string log_file = "logs/latest.log";
  std::string renderer_dx11_module = "bin/RendererDX11.dll";
  std::string physics_module = "bin/Physics.dll";
  std::string audio_module = "bin/Audio.dll";
  std::string core_module = "bin/Core.dll";
};

EngineConfig LoadEngineConfig(const std::filesystem::path& engine_config_path);

} // namespace cookie::core

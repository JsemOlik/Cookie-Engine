#include "Cookie/Core/EngineConfig.h"

#include <fstream>
#include <iterator>
#include <string>

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

void ApplyStringIfPresent(
    const std::string& source, const char* key_name, std::string& out_value) {
  const std::string parsed = ExtractStringValue(source, key_name);
  if (!parsed.empty()) {
    out_value = parsed;
  }
}

} // namespace

EngineConfig LoadEngineConfig(const std::filesystem::path& engine_config_path) {
  EngineConfig config{};

  std::ifstream file(engine_config_path);
  if (!file.is_open()) {
    return config;
  }

  const std::string contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  ApplyStringIfPresent(contents, "runtime", config.runtime_name);
  ApplyStringIfPresent(contents, "log_file", config.log_file);
  ApplyStringIfPresent(
      contents, "renderer_dx11_module", config.renderer_dx11_module);
  ApplyStringIfPresent(contents, "physics_module", config.physics_module);
  ApplyStringIfPresent(contents, "audio_module", config.audio_module);
  ApplyStringIfPresent(contents, "core_module", config.core_module);

  return config;
}

} // namespace cookie::core

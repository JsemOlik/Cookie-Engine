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

bool ExtractBooleanValue(const std::string& source, std::string key_name) {
  const std::string key = "\"" + std::move(key_name) + "\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return false;
  }

  const std::size_t colon_position = source.find(':', key_position + key.size());
  if (colon_position == std::string::npos) {
    return false;
  }

  const std::size_t value_start =
      source.find_first_not_of(" \t\r\n", colon_position + 1);
  if (value_start == std::string::npos) {
    return false;
  }

  if (source.compare(value_start, 4, "true") == 0) {
    return true;
  }
  if (source.compare(value_start, 1, "1") == 0) {
    return true;
  }

  return false;
}

void ApplyBooleanIfPresent(
    const std::string& source, const char* key_name, bool& out_value) {
  const std::string key = "\"" + std::string(key_name) + "\"";
  if (source.find(key) == std::string::npos) {
    return;
  }
  out_value = ExtractBooleanValue(source, key_name);
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
  ApplyBooleanIfPresent(contents, "strict_module_mode", config.strict_module_mode);
  ApplyBooleanIfPresent(contents, "require_core_module", config.require_core_module);
  ApplyBooleanIfPresent(
      contents, "require_renderer_module", config.require_renderer_module);
  ApplyBooleanIfPresent(
      contents, "require_physics_module", config.require_physics_module);
  ApplyBooleanIfPresent(contents, "require_audio_module", config.require_audio_module);

  return config;
}

} // namespace cookie::core

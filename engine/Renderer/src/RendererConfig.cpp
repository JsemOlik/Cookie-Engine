#include "Cookie/Renderer/RendererConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string>

namespace cookie::renderer {
namespace {

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string ExtractRendererValue(const std::string& source) {
  const std::string key = "\"renderer\"";
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

} // namespace

RendererConfig LoadRendererConfig(const std::filesystem::path& graphics_config_path) {
  RendererConfig config{};

  std::ifstream file(graphics_config_path);
  if (!file.is_open()) {
    return config;
  }

  const std::string contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  const std::string parsed_backend = ToLower(ExtractRendererValue(contents));
  if (!parsed_backend.empty()) {
    config.backend_name = parsed_backend;
  }

  return config;
}

} // namespace cookie::renderer

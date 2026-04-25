#include "Cookie/Renderer/RendererConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
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

int ExtractIntegerValue(const std::string& source, std::string key_name) {
  const std::string key = "\"" + std::move(key_name) + "\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return 0;
  }

  const std::size_t colon_position = source.find(':', key_position + key.size());
  if (colon_position == std::string::npos) {
    return 0;
  }

  const std::size_t number_start = source.find_first_of("-0123456789", colon_position + 1);
  if (number_start == std::string::npos) {
    return 0;
  }

  const std::size_t number_end =
      source.find_first_not_of("-0123456789", number_start);
  const std::string value_text =
      source.substr(number_start, number_end - number_start);
  if (value_text.empty()) {
    return 0;
  }

  try {
    return std::stoi(value_text);
  } catch (...) {
    return 0;
  }
}

bool ExtractBoolValue(const std::string& source, std::string key_name,
                      bool default_value) {
  const std::string key = "\"" + std::move(key_name) + "\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return default_value;
  }

  const std::size_t colon_position = source.find(':', key_position + key.size());
  if (colon_position == std::string::npos) {
    return default_value;
  }

  const std::size_t value_start =
      source.find_first_not_of(" \t\r\n", colon_position + 1);
  if (value_start == std::string::npos) {
    return default_value;
  }

  if (source.compare(value_start, 4, "true") == 0) {
    return true;
  }
  if (source.compare(value_start, 5, "false") == 0) {
    return false;
  }
  return default_value;
}

float ExtractFloatValue(const std::string& source, std::string key_name) {
  const std::string key = "\"" + std::move(key_name) + "\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return 0.0f;
  }

  const std::size_t colon_position = source.find(':', key_position + key.size());
  if (colon_position == std::string::npos) {
    return 0.0f;
  }

  const std::size_t number_start =
      source.find_first_of("-0123456789.", colon_position + 1);
  if (number_start == std::string::npos) {
    return 0.0f;
  }

  const std::size_t number_end =
      source.find_first_not_of("-0123456789.eE+", number_start);
  const std::string value_text =
      source.substr(number_start, number_end - number_start);
  if (value_text.empty()) {
    return 0.0f;
  }

  try {
    return std::stof(value_text);
  } catch (...) {
    return 0.0f;
  }
}

void TryExtractClearColor(const std::string& source, ClearColor& out_color) {
  const std::string key = "\"clear_color\"";
  const std::size_t key_position = source.find(key);
  if (key_position == std::string::npos) {
    return;
  }

  const std::size_t array_start = source.find('[', key_position + key.size());
  if (array_start == std::string::npos) {
    return;
  }

  const std::size_t array_end = source.find(']', array_start + 1);
  if (array_end == std::string::npos || array_end <= array_start + 1) {
    return;
  }

  std::string values = source.substr(array_start + 1, array_end - array_start - 1);
  std::replace(values.begin(), values.end(), ',', ' ');
  std::istringstream stream(values);

  float parsed[4]{};
  for (float& item : parsed) {
    if (!(stream >> item)) {
      return;
    }
  }

  out_color.red = parsed[0];
  out_color.green = parsed[1];
  out_color.blue = parsed[2];
  out_color.alpha = parsed[3];
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

  const std::string parsed_title = ExtractStringValue(contents, "window_title");
  if (!parsed_title.empty()) {
    config.window_title = parsed_title;
  }

  const int parsed_width = ExtractIntegerValue(contents, "window_width");
  if (parsed_width > 0) {
    config.window_width = parsed_width;
  }

  const int parsed_height = ExtractIntegerValue(contents, "window_height");
  if (parsed_height > 0) {
    config.window_height = parsed_height;
  }

  const std::string parsed_camera_mode =
      ToLower(ExtractStringValue(contents, "camera_mode"));
  if (parsed_camera_mode == "orthographic" || parsed_camera_mode == "perspective") {
    config.camera_mode = parsed_camera_mode;
  }

  const float parsed_ortho_height =
      ExtractFloatValue(contents, "camera_ortho_height");
  if (parsed_ortho_height > 0.0f) {
    config.camera_ortho_height = parsed_ortho_height;
  }

  const float parsed_perspective_fov =
      ExtractFloatValue(contents, "camera_perspective_fov_degrees");
  if (parsed_perspective_fov > 0.0f) {
    config.camera_perspective_fov_degrees = parsed_perspective_fov;
  }

  const float parsed_near_plane = ExtractFloatValue(contents, "camera_near_plane");
  const float parsed_far_plane = ExtractFloatValue(contents, "camera_far_plane");
  if (parsed_far_plane != parsed_near_plane) {
    config.camera_near_plane = parsed_near_plane;
    config.camera_far_plane = parsed_far_plane;
  }

  config.camera_orbit_enabled =
      ExtractBoolValue(contents, "camera_orbit_enabled", config.camera_orbit_enabled);

  const float parsed_orbit_radius =
      ExtractFloatValue(contents, "camera_orbit_radius");
  if (parsed_orbit_radius > 0.0f) {
    config.camera_orbit_radius = parsed_orbit_radius;
  }

  const float parsed_orbit_height =
      ExtractFloatValue(contents, "camera_orbit_height");
  config.camera_orbit_height = parsed_orbit_height;

  const float parsed_orbit_speed =
      ExtractFloatValue(contents, "camera_orbit_speed");
  if (parsed_orbit_speed > 0.0f) {
    config.camera_orbit_speed = parsed_orbit_speed;
  }

  TryExtractClearColor(contents, config.clear_color);

  return config;
}

} // namespace cookie::renderer

#include "Cookie/Assets/SceneAsset.h"

#include <fstream>

namespace cookie::assets {
namespace {

std::string Trim(std::string value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

bool ParseKeyValue(const std::string& line, std::string& out_key,
                   std::string& out_value) {
  const std::size_t separator = line.find(':');
  if (separator == std::string::npos) {
    return false;
  }
  out_key = Trim(line.substr(0, separator));
  out_value = Trim(line.substr(separator + 1));
  return !out_key.empty();
}

}  // namespace

bool LoadSceneAsset(const std::filesystem::path& scene_path, SceneAsset& out_scene,
                    std::string* error_message) {
  std::ifstream file(scene_path);
  if (!file.is_open()) {
    if (error_message) {
      *error_message = "Failed to open scene asset: " + scene_path.string();
    }
    return false;
  }

  SceneAsset parsed{};
  SceneAssetObject current_object{};
  bool in_object = false;

  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }

    std::string key;
    std::string value;
    if (!ParseKeyValue(trimmed, key, value)) {
      continue;
    }

    if (key == "object") {
      if (in_object && !current_object.name.empty()) {
        parsed.objects.push_back(current_object);
      }
      current_object = {};
      current_object.name = value;
      in_object = true;
      continue;
    }

    if (key == "endobject") {
      if (in_object && !current_object.name.empty()) {
        parsed.objects.push_back(current_object);
      }
      current_object = {};
      in_object = false;
      continue;
    }

    if (key == "nested_scene") {
      if (!value.empty()) {
        parsed.nested_scene_asset_ids.push_back(value);
      }
      continue;
    }

    if (in_object) {
      if (key == "mesh") {
        current_object.mesh_asset_id = value;
      } else if (key == "material") {
        current_object.material_asset_id = value;
      }
    }
  }

  if (in_object && !current_object.name.empty()) {
    parsed.objects.push_back(current_object);
  }

  out_scene = std::move(parsed);
  return true;
}

}  // namespace cookie::assets

#include "Cookie/Assets/SceneAsset.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <system_error>

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

bool ParseFloat(const std::string& value, float& out_value) {
  const char* begin = value.data();
  const char* end = value.data() + value.size();
  auto result = std::from_chars(begin, end, out_value);
  return result.ec == std::errc() && result.ptr == end;
}

bool ParseFloat3(const std::string& value, float (&out_values)[3]) {
  std::string token;
  std::size_t start = 0;
  int parsed_count = 0;
  while (start <= value.size() && parsed_count < 3) {
    const std::size_t separator = value.find(',', start);
    token = Trim(value.substr(start, separator == std::string::npos
                                         ? std::string::npos
                                         : separator - start));
    float parsed_value = 0.0f;
    if (!ParseFloat(token, parsed_value)) {
      return false;
    }
    out_values[parsed_count++] = parsed_value;
    if (separator == std::string::npos) {
      break;
    }
    start = separator + 1;
  }
  return parsed_count == 3;
}

std::string Float3ToString(const float (&values)[3]) {
  return std::to_string(values[0]) + "," + std::to_string(values[1]) + "," +
         std::to_string(values[2]);
}

void FinalizeObjectIfNamed(SceneAssetObject& object, bool& in_object,
                           SceneAsset& scene) {
  if (in_object && !object.name.empty()) {
    scene.objects.push_back(object);
  }
  object = {};
  in_object = false;
}

void AddDependencyIfSet(std::vector<std::string>& dependencies,
                        std::string_view value) {
  if (!value.empty()) {
    dependencies.emplace_back(value);
  }
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
      FinalizeObjectIfNamed(current_object, in_object, parsed);
      current_object = {};
      current_object.name = value;
      in_object = true;
      continue;
    }

    if (key == "endobject") {
      FinalizeObjectIfNamed(current_object, in_object, parsed);
      continue;
    }

    if (key == "nested_scene") {
      if (!value.empty()) {
        parsed.nested_scene_asset_ids.push_back(value);
      }
      continue;
    }

    if (!in_object) {
      continue;
    }

    if (key == "mesh") {
      current_object.has_mesh_renderer = true;
      current_object.mesh_renderer.mesh_asset_id = value;
      continue;
    }
    if (key == "material") {
      current_object.has_mesh_renderer = true;
      current_object.mesh_renderer.material_asset_id = value;
      continue;
    }
    if (key == "transform.position") {
      ParseFloat3(value, current_object.transform.position);
      continue;
    }
    if (key == "transform.rotation") {
      ParseFloat3(value, current_object.transform.rotation_euler_degrees);
      continue;
    }
    if (key == "transform.scale") {
      ParseFloat3(value, current_object.transform.scale);
      continue;
    }
    if (key == "meshrenderer.mesh") {
      current_object.has_mesh_renderer = true;
      current_object.mesh_renderer.mesh_asset_id = value;
      continue;
    }
    if (key == "meshrenderer.material") {
      current_object.has_mesh_renderer = true;
      current_object.mesh_renderer.material_asset_id = value;
      continue;
    }
    if (key == "rigidbody.enabled") {
      // Legacy migration behavior: rigidbody.enabled=false means component absent.
      // true means component present.
      current_object.has_rigidbody = (value == "true" || value == "1");
      continue;
    }
    if (key == "rigidbody.type") {
      current_object.has_rigidbody = true;
      current_object.rigidbody.body_type = value;
      continue;
    }
    if (key == "rigidbody.mass") {
      current_object.has_rigidbody = true;
      float parsed_mass = 1.0f;
      if (ParseFloat(value, parsed_mass)) {
        current_object.rigidbody.mass = parsed_mass;
      }
      continue;
    }
  }

  FinalizeObjectIfNamed(current_object, in_object, parsed);
  out_scene = std::move(parsed);
  return true;
}

bool SaveSceneAsset(const std::filesystem::path& scene_path, const SceneAsset& scene,
                    std::string* error_message) {
  std::error_code error;
  std::filesystem::create_directories(scene_path.parent_path(), error);
  if (error) {
    if (error_message) {
      *error_message =
          "Failed to create scene directory: " + scene_path.parent_path().string();
    }
    return false;
  }

  std::ofstream file(scene_path, std::ios::trunc);
  if (!file.is_open()) {
    if (error_message) {
      *error_message = "Failed to write scene asset: " + scene_path.string();
    }
    return false;
  }

  file << "# Cookie Scene Asset v0\n";
  file << "version: 1\n";

  std::vector<std::string> nested = scene.nested_scene_asset_ids;
  std::sort(nested.begin(), nested.end());
  nested.erase(std::unique(nested.begin(), nested.end()), nested.end());
  for (const auto& nested_scene_id : nested) {
    if (!nested_scene_id.empty()) {
      file << "nested_scene: " << nested_scene_id << "\n";
    }
  }

  for (const auto& object : scene.objects) {
    if (object.name.empty()) {
      continue;
    }
    file << "\nobject: " << object.name << "\n";
    file << "transform.position: " << Float3ToString(object.transform.position)
         << "\n";
    file << "transform.rotation: "
         << Float3ToString(object.transform.rotation_euler_degrees) << "\n";
    file << "transform.scale: " << Float3ToString(object.transform.scale) << "\n";
    if (object.has_mesh_renderer) {
      file << "meshrenderer.mesh: " << object.mesh_renderer.mesh_asset_id << "\n";
      file << "meshrenderer.material: " << object.mesh_renderer.material_asset_id
           << "\n";
    }
    if (object.has_rigidbody) {
      file << "rigidbody.type: " << object.rigidbody.body_type << "\n";
      file << "rigidbody.mass: " << object.rigidbody.mass << "\n";
    }
    file << "endobject: true\n";
  }

  return true;
}

std::vector<std::string> ExtractSceneDependencies(const SceneAsset& scene) {
  std::vector<std::string> dependencies;
  dependencies.reserve(scene.nested_scene_asset_ids.size() + scene.objects.size() * 2);
  for (const auto& nested_scene : scene.nested_scene_asset_ids) {
    AddDependencyIfSet(dependencies, nested_scene);
  }
  for (const auto& object : scene.objects) {
    if (object.has_mesh_renderer) {
      AddDependencyIfSet(dependencies, object.mesh_renderer.mesh_asset_id);
      AddDependencyIfSet(dependencies, object.mesh_renderer.material_asset_id);
    }
  }

  std::sort(dependencies.begin(), dependencies.end());
  dependencies.erase(std::unique(dependencies.begin(), dependencies.end()),
                     dependencies.end());
  return dependencies;
}

}  // namespace cookie::assets

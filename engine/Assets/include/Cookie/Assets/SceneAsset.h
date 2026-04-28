#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cookie::assets {

struct SceneAssetObject {
  std::string name;
  std::string mesh_asset_id;
  std::string material_asset_id;
};

struct SceneAsset {
  std::vector<SceneAssetObject> objects;
  std::vector<std::string> nested_scene_asset_ids;
};

bool LoadSceneAsset(const std::filesystem::path& scene_path, SceneAsset& out_scene,
                    std::string* error_message = nullptr);

}  // namespace cookie::assets

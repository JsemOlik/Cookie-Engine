#pragma once

#include <filesystem>
#include <string_view>
#include <string>
#include <vector>

namespace cookie::assets {

struct TransformComponent {
  float position[3] = {0.0f, 0.0f, 0.0f};
  float rotation_euler_degrees[3] = {0.0f, 0.0f, 0.0f};
  float scale[3] = {1.0f, 1.0f, 1.0f};
};

struct MeshRendererComponent {
  std::string mesh_asset_id;
  std::string material_asset_id;
};

struct RigidBodyStubComponent {
  bool enabled = false;
  std::string body_type = "dynamic";
  float mass = 1.0f;
};

struct SceneAssetObject {
  std::string name;
  TransformComponent transform;
  MeshRendererComponent mesh_renderer;
  bool has_rigidbody = false;
  RigidBodyStubComponent rigidbody;
};

struct SceneAsset {
  std::vector<SceneAssetObject> objects;
  std::vector<std::string> nested_scene_asset_ids;
};

bool LoadSceneAsset(const std::filesystem::path& scene_path, SceneAsset& out_scene,
                    std::string* error_message = nullptr);
bool SaveSceneAsset(const std::filesystem::path& scene_path, const SceneAsset& scene,
                    std::string* error_message = nullptr);
std::vector<std::string> ExtractSceneDependencies(const SceneAsset& scene);

}  // namespace cookie::assets

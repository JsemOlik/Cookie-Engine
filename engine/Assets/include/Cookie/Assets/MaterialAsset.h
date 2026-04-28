#pragma once

#include <filesystem>
#include <string>

namespace cookie::assets {

struct MaterialAsset {
  std::string albedo_asset_id;
  float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

bool LoadMaterialAsset(const std::filesystem::path& material_path,
                       MaterialAsset& out_material,
                       std::string* error_message = nullptr);

}  // namespace cookie::assets

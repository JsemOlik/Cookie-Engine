#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Cookie/Assets/AssetPackage.h"

namespace cookie::assets {

class AssetRegistry {
 public:
  bool MountPackage(const std::filesystem::path& pak_path);
  std::size_t GetMountedPackageCount() const;
  std::size_t GetAssetCount() const;
  bool HasAsset(std::string_view asset_id) const;
  std::filesystem::path ResolveAssetPath(std::string_view asset_id) const;
  std::vector<std::string> ListAssetIds() const;

 private:
  std::vector<MountedPackage> packages_;
};

} // namespace cookie::assets

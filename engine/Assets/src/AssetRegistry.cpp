#include "Cookie/Assets/AssetRegistry.h"

#include <algorithm>
#include <utility>

namespace cookie::assets {

bool AssetRegistry::MountPackage(const std::filesystem::path& pak_path) {
  MountedPackage package{};
  if (!LoadPackageFromTextPak(pak_path, package)) {
    return false;
  }

  packages_.push_back(std::move(package));
  return true;
}

std::size_t AssetRegistry::GetMountedPackageCount() const {
  return packages_.size();
}

std::size_t AssetRegistry::GetAssetCount() const {
  std::size_t total = 0;
  for (const auto& package : packages_) {
    total += package.asset_ids.size();
  }
  return total;
}

bool AssetRegistry::HasAsset(std::string_view asset_id) const {
  for (const auto& package : packages_) {
    const auto iter = std::find(package.asset_ids.begin(), package.asset_ids.end(),
                                asset_id);
    if (iter != package.asset_ids.end()) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> AssetRegistry::ListAssetIds() const {
  std::vector<std::string> all_assets;
  all_assets.reserve(GetAssetCount());
  for (const auto& package : packages_) {
    all_assets.insert(all_assets.end(), package.asset_ids.begin(),
                      package.asset_ids.end());
  }
  return all_assets;
}

} // namespace cookie::assets

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>

#include "Cookie/Assets/AssetPackage.h"
#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Assets/CookedAssetRegistry.h"

namespace cookie::assets {

struct DiscoveredAsset {
  std::filesystem::path meta_path;
  std::filesystem::path source_path;
  std::string source_relative_path;
  AssetMeta meta;
};

class AssetRegistry {
 public:
  bool MountPackage(const std::filesystem::path& pak_path);
  bool LoadCookedRegistry(const std::filesystem::path& registry_path);
  bool DiscoverProjectAssets(const std::filesystem::path& project_root);
  std::size_t GetMountedPackageCount() const;
  std::size_t GetAssetCount() const;
  bool HasAsset(std::string_view asset_id) const;
  std::filesystem::path ResolveAssetPath(std::string_view asset_id) const;
  std::optional<CookedAssetRecord> ResolveCookedRecord(
      std::string_view asset_id) const;
  std::vector<std::string> ListAssetIds() const;
  const std::vector<DiscoveredAsset>& GetDiscoveredAssets() const;

 private:
  std::filesystem::path ResolveCookedAssetPath(
      const CookedAssetRecord& record) const;
  std::vector<MountedPackage> packages_;
  CookedAssetRegistry cooked_registry_;
  std::filesystem::path cooked_registry_path_;
  std::vector<DiscoveredAsset> discovered_assets_;
  mutable std::unordered_map<std::string, std::filesystem::path> extracted_cache_paths_;
};

} // namespace cookie::assets

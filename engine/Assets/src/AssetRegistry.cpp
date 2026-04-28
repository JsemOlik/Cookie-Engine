#include "Cookie/Assets/AssetRegistry.h"

#include <algorithm>
#include <system_error>
#include <utility>

namespace cookie::assets {
namespace {

bool IsMetaFile(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename.size() > 5 &&
         filename.substr(filename.size() - 5) == ".meta";
}

std::filesystem::path ResolveSourcePathFromMeta(
    const std::filesystem::path& meta_path) {
  const std::string filename = meta_path.filename().string();
  return meta_path.parent_path() / filename.substr(0, filename.size() - 5);
}

}  // namespace

bool AssetRegistry::MountPackage(const std::filesystem::path& pak_path) {
  MountedPackage package{};
  if (!LoadPackageFromTextPak(pak_path, package)) {
    return false;
  }

  packages_.push_back(std::move(package));
  return true;
}

bool AssetRegistry::LoadCookedRegistry(
    const std::filesystem::path& registry_path) {
  cooked_registry_path_ = registry_path;
  return cooked_registry_.LoadFromFile(registry_path);
}

bool AssetRegistry::DiscoverProjectAssets(
    const std::filesystem::path& project_root) {
  discovered_assets_.clear();

  std::vector<std::filesystem::path> roots;
  const auto assets_root = project_root / "Assets";
  if (std::filesystem::exists(assets_root)) {
    roots.push_back(assets_root);
  }
  const auto content_root = project_root / "content";
  if (std::filesystem::exists(content_root)) {
    roots.push_back(content_root);
  }

  bool discovered_any = false;
  for (const auto& root : roots) {
    std::error_code error;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(root, error)) {
      if (error || !entry.is_regular_file() || !IsMetaFile(entry.path())) {
        continue;
      }
      AssetMeta meta;
      if (!LoadAssetMeta(entry.path(), meta, nullptr)) {
        continue;
      }
      const auto source_path = ResolveSourcePathFromMeta(entry.path());
      std::error_code relative_error;
      const auto relative_source =
          std::filesystem::relative(source_path, project_root, relative_error);
      DiscoveredAsset asset{};
      asset.meta_path = entry.path();
      asset.source_path = source_path;
      asset.source_relative_path =
          relative_error ? source_path.string() : relative_source.string();
      asset.meta = std::move(meta);
      discovered_assets_.push_back(std::move(asset));
      discovered_any = true;
    }
  }

  return discovered_any;
}

std::size_t AssetRegistry::GetMountedPackageCount() const {
  return packages_.size();
}

std::size_t AssetRegistry::GetAssetCount() const {
  std::size_t total = discovered_assets_.size() + cooked_registry_.Entries().size();
  for (const auto& package : packages_) {
    total += package.asset_ids.size();
  }
  return total;
}

bool AssetRegistry::HasAsset(std::string_view asset_id) const {
  if (cooked_registry_.FindByAssetId(asset_id) != nullptr) {
    return true;
  }
  for (const auto& asset : discovered_assets_) {
    if (asset.meta.asset_id == asset_id) {
      return true;
    }
  }
  for (const auto& package : packages_) {
    const auto iter = std::find(package.asset_ids.begin(), package.asset_ids.end(),
                                asset_id);
    if (iter != package.asset_ids.end()) {
      return true;
    }
  }
  return false;
}

std::filesystem::path AssetRegistry::ResolveAssetPath(
    std::string_view asset_id) const {
  if (const CookedAssetRecord* cooked = cooked_registry_.FindByAssetId(asset_id)) {
    if (!cooked_registry_path_.empty()) {
      return cooked_registry_path_.parent_path() / cooked->runtime_relative_path;
    }
  }

  for (const auto& asset : discovered_assets_) {
    if (asset.meta.asset_id == asset_id) {
      return asset.source_path;
    }
  }

  for (const auto& package : packages_) {
    const auto iter = std::find(package.asset_ids.begin(), package.asset_ids.end(),
                                asset_id);
    if (iter != package.asset_ids.end()) {
      return package.pak_path.parent_path() / *iter;
    }
  }
  return {};
}

std::optional<CookedAssetRecord> AssetRegistry::ResolveCookedRecord(
    std::string_view asset_id) const {
  if (const CookedAssetRecord* record =
          cooked_registry_.FindByAssetId(asset_id)) {
    return *record;
  }
  return std::nullopt;
}

std::vector<std::string> AssetRegistry::ListAssetIds() const {
  std::vector<std::string> all_assets;
  all_assets.reserve(GetAssetCount() + discovered_assets_.size() +
                     cooked_registry_.Entries().size());
  for (const auto& record : cooked_registry_.Entries()) {
    all_assets.push_back(record.asset_id);
  }
  for (const auto& asset : discovered_assets_) {
    all_assets.push_back(asset.meta.asset_id);
  }
  for (const auto& package : packages_) {
    all_assets.insert(all_assets.end(), package.asset_ids.begin(),
                      package.asset_ids.end());
  }
  return all_assets;
}

const std::vector<DiscoveredAsset>& AssetRegistry::GetDiscoveredAssets() const {
  return discovered_assets_;
}

} // namespace cookie::assets

#include "Cookie/Tools/PackageInspector.h"

#include "Cookie/Assets/AssetRegistry.h"

namespace cookie::tools {

PackageInspectResult InspectPackage(const std::filesystem::path& package_path) {
  PackageInspectResult result;
  result.package_path = package_path;

  assets::AssetRegistry registry;
  if (!registry.MountPackage(package_path.string())) {
    result.error = "Failed to mount package: " + package_path.string();
    return result;
  }

  result.success = true;
  result.mounted_package_count = registry.GetMountedPackageCount();
  result.asset_ids = registry.ListAssetIds();
  return result;
}

}  // namespace cookie::tools

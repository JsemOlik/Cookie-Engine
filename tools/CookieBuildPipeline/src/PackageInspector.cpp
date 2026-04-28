#include "Cookie/Tools/PackageInspector.h"

#include "Cookie/Assets/AssetPackage.h"

namespace cookie::tools {

PackageInspectResult InspectPackage(const std::filesystem::path& package_path) {
  PackageInspectResult result;
  result.package_path = package_path;

  assets::PakArchive archive;
  if (!archive.Open(package_path)) {
    result.error = "Failed to mount package: " + package_path.string();
    return result;
  }

  result.success = true;
  result.mounted_package_count = 1;
  for (const auto& entry : archive.Entries()) {
    result.asset_ids.push_back(entry.name);
  }
  return result;
}

}  // namespace cookie::tools

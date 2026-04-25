#include <iostream>
#include <string>

#include "Cookie/Assets/AssetRegistry.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: CookiePakTool <path-to-pak>\n";
    return 1;
  }

  cookie::assets::AssetRegistry registry;
  if (!registry.MountPackage(argv[1])) {
    std::cout << "Failed to mount package: " << argv[1] << '\n';
    return 2;
  }

  std::cout << "Mounted packages: " << registry.GetMountedPackageCount() << '\n';
  std::cout << "Asset count: " << registry.GetAssetCount() << '\n';
  for (const auto& asset_id : registry.ListAssetIds()) {
    std::cout << " - " << asset_id << '\n';
  }

  return 0;
}

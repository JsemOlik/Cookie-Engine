#include <iostream>

#include "Cookie/Tools/PackageInspector.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: CookiePakTool <path-to-pak>\n";
    return 1;
  }

  const auto result = cookie::tools::InspectPackage(argv[1]);
  if (!result.success) {
    std::cout << result.error << '\n';
    return 2;
  }

  std::cout << "Mounted packages: " << result.mounted_package_count << '\n';
  std::cout << "Asset count: " << result.asset_ids.size() << '\n';
  for (const auto& asset_id : result.asset_ids) {
    std::cout << " - " << asset_id << '\n';
  }

  return 0;
}

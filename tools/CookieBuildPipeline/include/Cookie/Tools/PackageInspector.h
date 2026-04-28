#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cookie::tools {

struct PackageInspectResult {
  bool success = false;
  std::filesystem::path package_path;
  std::string error;
  size_t mounted_package_count = 0;
  std::vector<std::string> asset_ids;
};

PackageInspectResult InspectPackage(const std::filesystem::path& package_path);

}  // namespace cookie::tools

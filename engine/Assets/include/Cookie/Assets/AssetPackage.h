#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cookie::assets {

struct MountedPackage {
  std::filesystem::path pak_path;
  std::vector<std::string> asset_ids;
};

bool LoadPackageFromTextPak(const std::filesystem::path& pak_path,
                            MountedPackage& out_package);

} // namespace cookie::assets

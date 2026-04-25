#include "Cookie/Assets/AssetPackage.h"

#include <fstream>
#include <string>
#include <utility>

namespace cookie::assets {
namespace {

std::string Trim(const std::string& value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }

  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

} // namespace

bool LoadPackageFromTextPak(const std::filesystem::path& pak_path,
                            MountedPackage& out_package) {
  std::ifstream file(pak_path);
  if (!file.is_open()) {
    return false;
  }

  MountedPackage parsed{};
  parsed.pak_path = pak_path;

  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }
    parsed.asset_ids.push_back(trimmed);
  }

  out_package = std::move(parsed);
  return true;
}

} // namespace cookie::assets

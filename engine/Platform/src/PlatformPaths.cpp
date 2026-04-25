#include "Cookie/Platform/PlatformPaths.h"

#include <array>

namespace cookie::platform {
namespace {

bool LooksLikeProjectRoot(const std::filesystem::path& candidate) {
  const std::array<std::filesystem::path, 2> markers = {
      candidate / "vcpkg.json",
      candidate / "CMakeLists.txt",
  };

  for (const auto& marker : markers) {
    if (!std::filesystem::exists(marker)) {
      return false;
    }
  }

  return true;
}

} // namespace

std::filesystem::path GetWorkingDirectory() {
  return std::filesystem::current_path();
}

std::filesystem::path GetProjectRoot() {
  std::filesystem::path current = GetWorkingDirectory();

  while (!current.empty()) {
    if (LooksLikeProjectRoot(current)) {
      return current;
    }

    const auto parent = current.parent_path();
    if (parent == current) {
      break;
    }
    current = parent;
  }

  return GetWorkingDirectory();
}

} // namespace cookie::platform

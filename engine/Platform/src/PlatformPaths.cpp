#include "Cookie/Platform/PlatformPaths.h"

#include <array>

#ifndef COOKIE_ENGINE_SOURCE_ROOT
#define COOKIE_ENGINE_SOURCE_ROOT ""
#endif

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

std::filesystem::path GetCompileTimeSourceRoot() {
  constexpr const char* source_root = COOKIE_ENGINE_SOURCE_ROOT;
  if (source_root[0] == '\0') {
    return {};
  }

  const std::filesystem::path root(source_root);
  if (!root.empty() && std::filesystem::exists(root) && LooksLikeProjectRoot(root)) {
    return root;
  }

  return {};
}

} // namespace

std::filesystem::path GetWorkingDirectory() {
  return std::filesystem::current_path();
}

std::filesystem::path GetProjectRoot() {
  const auto compile_time_root = GetCompileTimeSourceRoot();
  if (!compile_time_root.empty()) {
    return compile_time_root;
  }

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

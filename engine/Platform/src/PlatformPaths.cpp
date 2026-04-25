#include "Cookie/Platform/PlatformPaths.h"

#include <array>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

std::filesystem::path GetExecutableDirectory() {
#if defined(_WIN32)
  std::array<wchar_t, 32768> buffer{};
  const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                          static_cast<DWORD>(buffer.size()));
  if (length == 0 || length >= buffer.size()) {
    return {};
  }
  return std::filesystem::path(buffer.data(), buffer.data() + length).parent_path();
#else
  return {};
#endif
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

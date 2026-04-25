#include "Cookie/Platform/PlatformPaths.h"

namespace cookie::platform {

std::filesystem::path GetWorkingDirectory() {
  return std::filesystem::current_path();
}

std::filesystem::path GetProjectRoot() {
  return GetWorkingDirectory();
}

} // namespace cookie::platform

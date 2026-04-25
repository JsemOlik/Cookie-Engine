#pragma once

#include <filesystem>

namespace cookie::platform {

std::filesystem::path GetWorkingDirectory();
std::filesystem::path GetExecutableDirectory();
std::filesystem::path GetProjectRoot();

} // namespace cookie::platform

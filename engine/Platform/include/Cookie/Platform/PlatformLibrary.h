#pragma once

#include <filesystem>
#include <string>

namespace cookie::platform {

using LibraryHandle = void*;

LibraryHandle LoadDynamicLibrary(const std::filesystem::path& path);
void* GetLibrarySymbol(LibraryHandle library, const char* symbol_name);
void UnloadDynamicLibrary(LibraryHandle library);
std::string GetLastDynamicLibraryError();

} // namespace cookie::platform

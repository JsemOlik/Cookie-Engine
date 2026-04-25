#include "Cookie/Platform/PlatformLibrary.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

namespace cookie::platform {

LibraryHandle LoadDynamicLibrary(const std::filesystem::path& path) {
  return reinterpret_cast<LibraryHandle>(LoadLibraryW(path.c_str()));
}

void* GetLibrarySymbol(LibraryHandle library, const char* symbol_name) {
  if (!library || !symbol_name) {
    return nullptr;
  }

  auto* module = reinterpret_cast<HMODULE>(library);
  return reinterpret_cast<void*>(GetProcAddress(module, symbol_name));
}

void UnloadDynamicLibrary(LibraryHandle library) {
  if (!library) {
    return;
  }

  auto* module = reinterpret_cast<HMODULE>(library);
  FreeLibrary(module);
}

std::string GetLastDynamicLibraryError() {
  const DWORD error_code = GetLastError();
  if (error_code == 0) {
    return {};
  }

  LPSTR message_buffer = nullptr;
  const DWORD length = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);

  std::string message;
  if (length > 0 && message_buffer != nullptr) {
    message.assign(message_buffer, length);
    LocalFree(message_buffer);
  }

  return message;
}

} // namespace cookie::platform

#else

namespace cookie::platform {

LibraryHandle LoadDynamicLibrary(const std::filesystem::path& path) {
  (void)path;
  return nullptr;
}

void* GetLibrarySymbol(LibraryHandle library, const char* symbol_name) {
  (void)library;
  (void)symbol_name;
  return nullptr;
}

void UnloadDynamicLibrary(LibraryHandle library) {
  (void)library;
}

std::string GetLastDynamicLibraryError() {
  return "Dynamic library loading is not implemented on this platform.";
}

} // namespace cookie::platform

#endif

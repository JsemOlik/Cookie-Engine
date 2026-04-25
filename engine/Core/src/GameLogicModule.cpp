#include "Cookie/Core/GameLogicModule.h"

#include "Cookie/Platform/PlatformLibrary.h"

namespace cookie::core {
namespace {

constexpr const char* kGameLogicStartup = "CookieGameStartup";
constexpr const char* kGameLogicUpdate = "CookieGameUpdate";
constexpr const char* kGameLogicShutdown = "CookieGameShutdown";

} // namespace

bool GameLogicModule::Load(const std::filesystem::path& path) {
  Unload();

  library_ = cookie::platform::LoadDynamicLibrary(path);
  if (!library_) {
    return false;
  }

  startup_ = reinterpret_cast<StartupFn>(
      cookie::platform::GetLibrarySymbol(library_, kGameLogicStartup));
  update_ = reinterpret_cast<UpdateFn>(
      cookie::platform::GetLibrarySymbol(library_, kGameLogicUpdate));
  shutdown_ = reinterpret_cast<ShutdownFn>(
      cookie::platform::GetLibrarySymbol(library_, kGameLogicShutdown));

  if (!startup_ || !update_ || !shutdown_) {
    Unload();
    return false;
  }

  return true;
}

bool GameLogicModule::IsLoaded() const {
  return library_ != nullptr;
}

bool GameLogicModule::Startup() const {
  if (!startup_) {
    return false;
  }
  return startup_();
}

void GameLogicModule::Update(float delta_seconds) const {
  if (!update_) {
    return;
  }
  update_(delta_seconds);
}

void GameLogicModule::Shutdown() const {
  if (!shutdown_) {
    return;
  }
  shutdown_();
}

void GameLogicModule::Unload() {
  if (library_) {
    cookie::platform::UnloadDynamicLibrary(library_);
  }

  library_ = nullptr;
  startup_ = nullptr;
  update_ = nullptr;
  shutdown_ = nullptr;
}

} // namespace cookie::core

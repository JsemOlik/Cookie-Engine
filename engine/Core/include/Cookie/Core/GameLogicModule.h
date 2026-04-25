#pragma once

#include <filesystem>

namespace cookie::core {

class GameLogicModule {
 public:
  bool Load(const std::filesystem::path& path);
  bool IsLoaded() const;
  bool Startup() const;
  void Update(float delta_seconds) const;
  void Shutdown() const;
  void Unload();

 private:
  using StartupFn = bool (*)();
  using UpdateFn = void (*)(float);
  using ShutdownFn = void (*)();

  void* library_ = nullptr;
  StartupFn startup_ = nullptr;
  UpdateFn update_ = nullptr;
  ShutdownFn shutdown_ = nullptr;
};

} // namespace cookie::core

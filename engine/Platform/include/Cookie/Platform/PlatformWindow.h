#pragma once

#include <memory>
#include <string>

namespace cookie::platform {

enum class KeyCode {
  W,
  A,
  S,
  D,
  Q,
  E,
  Shift,
  Escape,
};

struct WindowCreateInfo {
  std::string title = "CookieRuntime";
  int width = 1280;
  int height = 720;
};

class IPlatformWindow {
 public:
  virtual ~IPlatformWindow() = default;

  virtual void PollEvents() = 0;
  virtual bool ShouldClose() const = 0;
  virtual void RequestClose() = 0;
  virtual void* GetNativeHandle() const = 0;
  virtual bool IsKeyDown(KeyCode key) const = 0;
  virtual void ConsumeMouseDelta(float& delta_x, float& delta_y) = 0;
};

std::unique_ptr<IPlatformWindow> CreatePlatformWindow(
    const WindowCreateInfo& create_info);

} // namespace cookie::platform

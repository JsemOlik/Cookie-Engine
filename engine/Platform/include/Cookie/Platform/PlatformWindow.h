#pragma once

#include <memory>
#include <string>

namespace cookie::platform {

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
};

std::unique_ptr<IPlatformWindow> CreatePlatformWindow(
    const WindowCreateInfo& create_info);

} // namespace cookie::platform

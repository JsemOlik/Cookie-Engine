#pragma once

#include <string_view>

namespace cookie::renderer {

struct ClearColor {
  float red = 0.1f;
  float green = 0.1f;
  float blue = 0.2f;
  float alpha = 1.0f;
};

struct RendererInitInfo {
  void* native_window_handle = nullptr;
  int window_width = 1280;
  int window_height = 720;
  bool enable_vsync = true;
};

class IRendererBackend {
 public:
  virtual ~IRendererBackend() = default;

  virtual bool Initialize(const RendererInitInfo& init_info) = 0;
  virtual bool BeginFrame() = 0;
  virtual void Clear(const ClearColor& color) = 0;
  virtual void EndFrame() = 0;
  virtual void Shutdown() = 0;
  virtual std::string_view Name() const = 0;
};

} // namespace cookie::renderer

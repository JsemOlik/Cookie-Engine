#pragma once

#include <cstddef>
#include <cstdint>
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

struct Float4x4 {
  float m[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
  };
};

struct SceneVertex {
  float position[3] = {0.0f, 0.0f, 0.0f};
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct RenderMeshInstance {
  const SceneVertex* vertices = nullptr;
  std::size_t vertex_count = 0;
  const std::uint32_t* indices = nullptr;
  std::size_t index_count = 0;
  Float4x4 model_transform{};
};

struct RenderScene {
  const RenderMeshInstance* instances = nullptr;
  std::size_t instance_count = 0;
};

class IRendererBackend {
 public:
  virtual ~IRendererBackend() = default;

  virtual bool Initialize(const RendererInitInfo& init_info) = 0;
  virtual bool BeginFrame() = 0;
  virtual void Clear(const ClearColor& color) = 0;
  virtual void SubmitScene(const RenderScene& scene) = 0;
  virtual void EndFrame() = 0;
  virtual void Shutdown() = 0;
  virtual std::string_view Name() const = 0;
};

} // namespace cookie::renderer

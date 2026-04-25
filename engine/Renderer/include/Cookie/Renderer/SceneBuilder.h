#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/Transform.h"

namespace cookie::renderer {

class SceneBuilder {
 public:
  void Reset(const Float4x4& view_projection = MakeIdentityTransform());
  bool AddMeshInstance(const SceneVertex* vertices, std::size_t vertex_count,
                       const Float4x4& model_transform = MakeIdentityTransform());
  bool AddIndexedMeshInstance(
      const SceneVertex* vertices, std::size_t vertex_count,
      const std::uint32_t* indices, std::size_t index_count,
      const Float4x4& model_transform = MakeIdentityTransform());
  const RenderScene& Build();

 private:
  struct PendingInstance {
    std::size_t vertex_offset = 0;
    std::size_t vertex_count = 0;
    std::size_t index_offset = 0;
    std::size_t index_count = 0;
    Float4x4 model_transform{};
  };

  std::vector<SceneVertex> vertex_storage_;
  std::vector<std::uint32_t> index_storage_;
  std::vector<PendingInstance> pending_instances_;
  std::vector<RenderMeshInstance> built_instances_;
  RenderCamera camera_{};
  RenderScene scene_{};
};

} // namespace cookie::renderer

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/Renderer/Transform.h"

namespace cookie::renderer {

class SceneBuilder {
 public:
  std::size_t AddMaterial(const RenderMaterial& material);
  void Reset();
  bool AddMeshInstance(const SceneVertex* vertices, std::size_t vertex_count,
                       const Float4x4& model_transform = MakeIdentityTransform());
  bool AddMeshInstance(const SceneVertex* vertices, std::size_t vertex_count,
                       std::size_t material_index,
                       const Float4x4& model_transform = MakeIdentityTransform());
  bool AddIndexedMeshInstance(
      const SceneVertex* vertices, std::size_t vertex_count,
      const std::uint32_t* indices, std::size_t index_count,
      const Float4x4& model_transform = MakeIdentityTransform());
  bool AddIndexedMeshInstance(
      const SceneVertex* vertices, std::size_t vertex_count,
      const std::uint32_t* indices, std::size_t index_count,
      std::size_t material_index,
      const Float4x4& model_transform = MakeIdentityTransform());
  const RenderScene& Build();

 private:
  struct PendingMaterial {
    std::size_t path_index = 0;
    float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool use_albedo = false;
  };

  struct PendingInstance {
    std::size_t vertex_offset = 0;
    std::size_t vertex_count = 0;
    std::size_t index_offset = 0;
    std::size_t index_count = 0;
    std::size_t material_index = 0;
    Float4x4 model_transform{};
  };

  std::vector<SceneVertex> vertex_storage_;
  std::vector<std::uint32_t> index_storage_;
  std::vector<std::string> material_paths_;
  std::vector<PendingMaterial> pending_materials_;
  std::vector<PendingInstance> pending_instances_;
  std::vector<std::string> built_material_paths_;
  std::vector<RenderMaterial> built_materials_;
  std::vector<RenderMeshInstance> built_instances_;
  RenderScene scene_{};
};

} // namespace cookie::renderer

#include "Cookie/Renderer/SceneBuilder.h"

namespace cookie::renderer {

void SceneBuilder::Reset(const Float4x4& view_projection) {
  vertex_storage_.clear();
  index_storage_.clear();
  pending_instances_.clear();
  built_instances_.clear();
  camera_.view_projection = view_projection;
  scene_.camera = camera_;
  scene_.instances = nullptr;
  scene_.instance_count = 0;
}

bool SceneBuilder::AddMeshInstance(const SceneVertex* vertices,
                                   std::size_t vertex_count,
                                   const Float4x4& model_transform) {
  if (vertices == nullptr || vertex_count == 0) {
    return false;
  }

  const std::size_t offset = vertex_storage_.size();
  vertex_storage_.insert(vertex_storage_.end(), vertices, vertices + vertex_count);
  pending_instances_.push_back({
      .vertex_offset = offset,
      .vertex_count = vertex_count,
      .index_offset = 0,
      .index_count = 0,
      .model_transform = model_transform,
  });
  return true;
}

bool SceneBuilder::AddIndexedMeshInstance(
    const SceneVertex* vertices, std::size_t vertex_count,
    const std::uint32_t* indices, std::size_t index_count,
    const Float4x4& model_transform) {
  if (vertices == nullptr || vertex_count == 0 ||
      indices == nullptr || index_count == 0) {
    return false;
  }

  const std::size_t vertex_offset = vertex_storage_.size();
  vertex_storage_.insert(vertex_storage_.end(), vertices, vertices + vertex_count);

  const std::size_t index_offset = index_storage_.size();
  for (std::size_t i = 0; i < index_count; ++i) {
    index_storage_.push_back(
        static_cast<std::uint32_t>(indices[i] + vertex_offset));
  }

  pending_instances_.push_back({
      .vertex_offset = vertex_offset,
      .vertex_count = vertex_count,
      .index_offset = index_offset,
      .index_count = index_count,
      .model_transform = model_transform,
  });
  return true;
}

const RenderScene& SceneBuilder::Build() {
  built_instances_.clear();
  built_instances_.reserve(pending_instances_.size());

  const SceneVertex* base_vertices =
      vertex_storage_.empty() ? nullptr : vertex_storage_.data();
  const std::uint32_t* base_indices =
      index_storage_.empty() ? nullptr : index_storage_.data();
  for (const PendingInstance& pending : pending_instances_) {
    built_instances_.push_back({
        .vertices = (base_vertices == nullptr) ? nullptr
                                               : base_vertices + pending.vertex_offset,
        .vertex_count = pending.vertex_count,
        .indices = (base_indices == nullptr || pending.index_count == 0)
                       ? nullptr
                       : base_indices + pending.index_offset,
        .index_count = pending.index_count,
        .model_transform = pending.model_transform,
    });
  }

  scene_.camera = camera_;
  scene_.instances = built_instances_.empty() ? nullptr : built_instances_.data();
  scene_.instance_count = built_instances_.size();
  return scene_;
}

} // namespace cookie::renderer

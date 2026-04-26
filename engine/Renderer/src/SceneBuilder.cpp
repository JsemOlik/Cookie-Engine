#include "Cookie/Renderer/SceneBuilder.h"

namespace cookie::renderer {

std::size_t SceneBuilder::AddMaterial(const RenderMaterial& material) {
  const std::size_t path_index = material_paths_.size();
  if (material.albedo_texture_path != nullptr) {
    material_paths_.emplace_back(material.albedo_texture_path);
  } else {
    material_paths_.emplace_back();
  }

  PendingMaterial pending{};
  pending.path_index = path_index;
  pending.tint[0] = material.tint[0];
  pending.tint[1] = material.tint[1];
  pending.tint[2] = material.tint[2];
  pending.tint[3] = material.tint[3];
  pending.use_albedo = material.use_albedo;
  pending_materials_.push_back(pending);
  return pending_materials_.size() - 1;
}

void SceneBuilder::Reset() {
  vertex_storage_.clear();
  index_storage_.clear();
  material_paths_.clear();
  pending_materials_.clear();
  pending_instances_.clear();
  built_material_paths_.clear();
  built_materials_.clear();
  built_instances_.clear();
  AddMaterial({});
  scene_.instances = nullptr;
  scene_.instance_count = 0;
  scene_.materials = nullptr;
  scene_.material_count = 0;
}

bool SceneBuilder::AddMeshInstance(const SceneVertex* vertices,
                                   std::size_t vertex_count,
                                   const Float4x4& model_transform) {
  return AddMeshInstance(vertices, vertex_count, 0, model_transform);
}

bool SceneBuilder::AddMeshInstance(const SceneVertex* vertices,
                                   std::size_t vertex_count,
                                   std::size_t material_index,
                                   const Float4x4& model_transform) {
  if (pending_materials_.empty()) {
    AddMaterial({});
  }
  if (vertices == nullptr || vertex_count == 0) {
    return false;
  }
  if (material_index >= pending_materials_.size()) {
    return false;
  }

  const std::size_t offset = vertex_storage_.size();
  vertex_storage_.insert(vertex_storage_.end(), vertices, vertices + vertex_count);
  pending_instances_.push_back({
      .vertex_offset = offset,
      .vertex_count = vertex_count,
      .index_offset = 0,
      .index_count = 0,
      .material_index = material_index,
      .model_transform = model_transform,
  });
  return true;
}

bool SceneBuilder::AddIndexedMeshInstance(
    const SceneVertex* vertices, std::size_t vertex_count,
    const std::uint32_t* indices, std::size_t index_count,
    const Float4x4& model_transform) {
  return AddIndexedMeshInstance(vertices, vertex_count, indices, index_count, 0,
                                model_transform);
}

bool SceneBuilder::AddIndexedMeshInstance(
    const SceneVertex* vertices, std::size_t vertex_count,
    const std::uint32_t* indices, std::size_t index_count,
    std::size_t material_index,
    const Float4x4& model_transform) {
  if (pending_materials_.empty()) {
    AddMaterial({});
  }
  if (vertices == nullptr || vertex_count == 0 ||
      indices == nullptr || index_count == 0) {
    return false;
  }
  if (material_index >= pending_materials_.size()) {
    return false;
  }

  const std::size_t vertex_offset = vertex_storage_.size();
  vertex_storage_.insert(vertex_storage_.end(), vertices, vertices + vertex_count);

  const std::size_t index_offset = index_storage_.size();
  for (std::size_t i = 0; i < index_count; ++i) {
    index_storage_.push_back(indices[i]);
  }

  pending_instances_.push_back({
      .vertex_offset = vertex_offset,
      .vertex_count = vertex_count,
      .index_offset = index_offset,
      .index_count = index_count,
      .material_index = material_index,
      .model_transform = model_transform,
  });
  return true;
}

const RenderScene& SceneBuilder::Build() {
  built_material_paths_ = material_paths_;
  built_materials_.clear();
  built_materials_.reserve(pending_materials_.size());
  for (std::size_t i = 0; i < pending_materials_.size(); ++i) {
    const PendingMaterial& pending_material = pending_materials_[i];
    const std::string& path = built_material_paths_[pending_material.path_index];
    RenderMaterial material{};
    material.albedo_texture_path = path.empty() ? nullptr : path.c_str();
    material.tint[0] = pending_material.tint[0];
    material.tint[1] = pending_material.tint[1];
    material.tint[2] = pending_material.tint[2];
    material.tint[3] = pending_material.tint[3];
    material.use_albedo = pending_material.use_albedo;
    built_materials_.push_back(material);
  }

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
        .material_index = pending.material_index,
        .model_transform = pending.model_transform,
    });
  }

  scene_.instances = built_instances_.empty() ? nullptr : built_instances_.data();
  scene_.instance_count = built_instances_.size();
  scene_.materials = built_materials_.empty() ? nullptr : built_materials_.data();
  scene_.material_count = built_materials_.size();
  return scene_;
}

} // namespace cookie::renderer

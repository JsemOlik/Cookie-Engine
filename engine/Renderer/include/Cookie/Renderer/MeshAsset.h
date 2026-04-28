#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

struct ImportedPrimitive {
  std::vector<SceneVertex> vertices;
  std::vector<std::uint32_t> indices;
  int material_index = -1;
};

struct ImportedMesh {
  std::vector<ImportedPrimitive> primitives;
};

bool LoadMeshFromPath(const std::filesystem::path& path, ImportedMesh& out_mesh,
                      std::string* out_error = nullptr);

} // namespace cookie::renderer

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace cookie::assets {

struct GlbMeshData {
  std::vector<float> positions_xyz;
  std::vector<std::uint32_t> indices;
};

bool LoadFirstMeshFromGlb(const std::filesystem::path& glb_path,
                          GlbMeshData& out_mesh, std::string* out_error = nullptr);

} // namespace cookie::assets

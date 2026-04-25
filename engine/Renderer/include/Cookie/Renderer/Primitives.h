#pragma once

#include <array>
#include <cstdint>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle();
std::array<SceneVertex, 8> MakeColoredCubeVertices(float half_extent = 0.5f);
std::array<std::uint32_t, 36> MakeCubeTriangleIndices();

} // namespace cookie::renderer

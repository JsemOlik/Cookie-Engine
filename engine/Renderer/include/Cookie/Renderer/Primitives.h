#pragma once

#include <array>
#include <cstdint>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle();
std::array<SceneVertex, 24> MakeColoredCubeVertices();
std::array<std::uint32_t, 36> MakeCubeIndices();

} // namespace cookie::renderer

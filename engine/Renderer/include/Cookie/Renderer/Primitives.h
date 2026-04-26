#pragma once

#include <array>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle();
std::array<SceneVertex, 36> MakeColoredCube();

} // namespace cookie::renderer

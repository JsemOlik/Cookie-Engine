#include "Cookie/Renderer/Primitives.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle() {
  return {{
      {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}},
      {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}},
      {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}},
  }};
}

std::array<SceneVertex, 8> MakeColoredCubeVertices(float half_extent) {
  const float h = (half_extent > 0.0f) ? half_extent : 0.5f;
  return {{
      {{-h, -h, -h}, {1.0f, 0.2f, 0.2f, 1.0f}},
      {{h, -h, -h}, {0.2f, 1.0f, 0.2f, 1.0f}},
      {{h, h, -h}, {0.2f, 0.4f, 1.0f, 1.0f}},
      {{-h, h, -h}, {1.0f, 1.0f, 0.2f, 1.0f}},
      {{-h, -h, h}, {1.0f, 0.2f, 1.0f, 1.0f}},
      {{h, -h, h}, {0.2f, 1.0f, 1.0f, 1.0f}},
      {{h, h, h}, {1.0f, 0.6f, 0.2f, 1.0f}},
      {{-h, h, h}, {0.8f, 0.8f, 0.8f, 1.0f}},
  }};
}

std::array<std::uint32_t, 36> MakeCubeTriangleIndices() {
  return {{
      0, 1, 2, 0, 2, 3, // back
      4, 6, 5, 4, 7, 6, // front
      0, 4, 5, 0, 5, 1, // bottom
      3, 2, 6, 3, 6, 7, // top
      1, 5, 6, 1, 6, 2, // right
      0, 3, 7, 0, 7, 4, // left
  }};
}

} // namespace cookie::renderer

#include "Cookie/Renderer/Primitives.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle() {
  return {{
      {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}, {0.5f, 0.0f}},
      {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}, {1.0f, 1.0f}},
      {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}, {0.0f, 1.0f}},
  }};
}

std::array<SceneVertex, 8> MakeColoredCubeVertices() {
  return {{
      {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
      {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

      {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
  }};
}

std::array<std::uint32_t, 36> MakeCubeIndices() {
  return {{
      0, 1, 2,
      0, 2, 3,
      4, 6, 5,
      4, 7, 6,
      4, 5, 1,
      4, 1, 0,
      3, 2, 6,
      3, 6, 7,
      1, 5, 6,
      1, 6, 2,
      4, 0, 3,
      4, 3, 7,
  }};
}

} // namespace cookie::renderer

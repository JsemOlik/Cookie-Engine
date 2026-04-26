#include "Cookie/Renderer/Primitives.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle() {
  return {{
      {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}, {0.5f, 0.0f}},
      {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}, {1.0f, 1.0f}},
      {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}, {0.0f, 1.0f}},
  }};
}

std::array<SceneVertex, 24> MakeColoredCubeVertices() {
  auto MakeVertex = [](float x, float y, float z, float u, float v) {
    SceneVertex vertex{};
    vertex.position[0] = x;
    vertex.position[1] = y;
    vertex.position[2] = z;
    vertex.color[0] = 1.0f;
    vertex.color[1] = 1.0f;
    vertex.color[2] = 1.0f;
    vertex.color[3] = 1.0f;
    vertex.uv[0] = u;
    vertex.uv[1] = v;
    return vertex;
  };

  return {{
      // Front (+Z)
      MakeVertex(-1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
      MakeVertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f),
      MakeVertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f),
      MakeVertex(1.0f, -1.0f, 1.0f, 1.0f, 1.0f),

      // Back (-Z)
      MakeVertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
      MakeVertex(1.0f, 1.0f, -1.0f, 0.0f, 0.0f),
      MakeVertex(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f),
      MakeVertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

      // Left (-X)
      MakeVertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
      MakeVertex(-1.0f, 1.0f, -1.0f, 0.0f, 0.0f),
      MakeVertex(-1.0f, 1.0f, 1.0f, 1.0f, 0.0f),
      MakeVertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f),

      // Right (+X)
      MakeVertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
      MakeVertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f),
      MakeVertex(1.0f, 1.0f, -1.0f, 1.0f, 0.0f),
      MakeVertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

      // Top (+Y)
      MakeVertex(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f),
      MakeVertex(-1.0f, 1.0f, -1.0f, 0.0f, 0.0f),
      MakeVertex(1.0f, 1.0f, -1.0f, 1.0f, 0.0f),
      MakeVertex(1.0f, 1.0f, 1.0f, 1.0f, 1.0f),

      // Bottom (-Y)
      MakeVertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
      MakeVertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f),
      MakeVertex(1.0f, -1.0f, 1.0f, 1.0f, 0.0f),
      MakeVertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f),
  }};
}

std::array<std::uint32_t, 36> MakeCubeIndices() {
  return {{
      0, 1, 2, 0, 2, 3,
      4, 5, 6, 4, 6, 7,
      8, 9, 10, 8, 10, 11,
      12, 13, 14, 12, 14, 15,
      16, 17, 18, 16, 18, 19,
      20, 21, 22, 20, 22, 23,
  }};
}

} // namespace cookie::renderer

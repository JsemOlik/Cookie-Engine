#include "Cookie/Renderer/Primitives.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle() {
  return {{
      {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}},
      {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}},
      {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}},
  }};
}

} // namespace cookie::renderer

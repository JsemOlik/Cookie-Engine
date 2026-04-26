#include "Cookie/Renderer/Primitives.h"

namespace cookie::renderer {

std::array<SceneVertex, 3> MakeColoredTriangle() {
  return {{
      {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}},
      {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}},
      {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}},
  }};
}

std::array<SceneVertex, 36> MakeColoredCube() {
  constexpr float h = 0.5f;
  constexpr float red[4] = {0.9f, 0.25f, 0.25f, 1.0f};
  constexpr float green[4] = {0.25f, 0.85f, 0.35f, 1.0f};
  constexpr float blue[4] = {0.25f, 0.45f, 0.95f, 1.0f};
  constexpr float yellow[4] = {0.95f, 0.85f, 0.25f, 1.0f};
  constexpr float cyan[4] = {0.25f, 0.85f, 0.9f, 1.0f};
  constexpr float magenta[4] = {0.85f, 0.25f, 0.9f, 1.0f};

  return {{
      {{-h, -h, -h}, {red[0], red[1], red[2], red[3]}},
      {{-h, h, -h}, {red[0], red[1], red[2], red[3]}},
      {{h, h, -h}, {red[0], red[1], red[2], red[3]}},
      {{-h, -h, -h}, {red[0], red[1], red[2], red[3]}},
      {{h, h, -h}, {red[0], red[1], red[2], red[3]}},
      {{h, -h, -h}, {red[0], red[1], red[2], red[3]}},

      {{-h, -h, h}, {green[0], green[1], green[2], green[3]}},
      {{h, h, h}, {green[0], green[1], green[2], green[3]}},
      {{-h, h, h}, {green[0], green[1], green[2], green[3]}},
      {{-h, -h, h}, {green[0], green[1], green[2], green[3]}},
      {{h, -h, h}, {green[0], green[1], green[2], green[3]}},
      {{h, h, h}, {green[0], green[1], green[2], green[3]}},

      {{-h, -h, -h}, {blue[0], blue[1], blue[2], blue[3]}},
      {{-h, -h, h}, {blue[0], blue[1], blue[2], blue[3]}},
      {{-h, h, h}, {blue[0], blue[1], blue[2], blue[3]}},
      {{-h, -h, -h}, {blue[0], blue[1], blue[2], blue[3]}},
      {{-h, h, h}, {blue[0], blue[1], blue[2], blue[3]}},
      {{-h, h, -h}, {blue[0], blue[1], blue[2], blue[3]}},

      {{h, -h, -h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},
      {{h, h, h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},
      {{h, -h, h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},
      {{h, -h, -h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},
      {{h, h, -h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},
      {{h, h, h}, {yellow[0], yellow[1], yellow[2], yellow[3]}},

      {{-h, h, -h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},
      {{-h, h, h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},
      {{h, h, h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},
      {{-h, h, -h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},
      {{h, h, h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},
      {{h, h, -h}, {cyan[0], cyan[1], cyan[2], cyan[3]}},

      {{-h, -h, -h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
      {{h, -h, h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
      {{-h, -h, h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
      {{-h, -h, -h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
      {{h, -h, -h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
      {{h, -h, h}, {magenta[0], magenta[1], magenta[2], magenta[3]}},
  }};
}

} // namespace cookie::renderer

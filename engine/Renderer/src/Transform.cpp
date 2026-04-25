#include "Cookie/Renderer/Transform.h"

#include <cmath>

namespace cookie::renderer {
namespace {

Float4x4 MakeZeroMatrix() {
  Float4x4 matrix{};
  for (float& value : matrix.m) {
    value = 0.0f;
  }
  return matrix;
}

} // namespace

Float4x4 MakeIdentityTransform() {
  return {};
}

Float4x4 MakeZRotationTransform(float radians) {
  Float4x4 matrix = MakeIdentityTransform();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  matrix.m[0] = c;
  matrix.m[1] = s;
  matrix.m[4] = -s;
  matrix.m[5] = c;
  return matrix;
}

Float4x4 MakeTranslationTransform(float x, float y, float z) {
  Float4x4 matrix = MakeIdentityTransform();
  matrix.m[12] = x;
  matrix.m[13] = y;
  matrix.m[14] = z;
  return matrix;
}

Float4x4 MakeScaleTransform(float x, float y, float z) {
  Float4x4 matrix = MakeIdentityTransform();
  matrix.m[0] = x;
  matrix.m[5] = y;
  matrix.m[10] = z;
  return matrix;
}

Float4x4 MultiplyTransforms(const Float4x4& a, const Float4x4& b) {
  Float4x4 result = MakeZeroMatrix();
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.m[row * 4 + k] * b.m[k * 4 + col];
      }
      result.m[row * 4 + col] = sum;
    }
  }
  return result;
}

Float4x4 MakeOrthographicProjection(float width, float height, float near_plane,
                                    float far_plane) {
  if (width == 0.0f || height == 0.0f || near_plane == far_plane) {
    return MakeIdentityTransform();
  }

  Float4x4 matrix = MakeZeroMatrix();
  matrix.m[0] = 2.0f / width;
  matrix.m[5] = 2.0f / height;
  matrix.m[10] = 1.0f / (far_plane - near_plane);
  matrix.m[14] = near_plane / (near_plane - far_plane);
  matrix.m[15] = 1.0f;
  return matrix;
}

Float4x4 MakePerspectiveProjection(float fov_y_radians, float aspect_ratio,
                                   float near_plane, float far_plane) {
  if (aspect_ratio == 0.0f || near_plane <= 0.0f || near_plane == far_plane ||
      fov_y_radians <= 0.0f) {
    return MakeIdentityTransform();
  }

  const float y_scale = 1.0f / std::tan(fov_y_radians * 0.5f);
  const float x_scale = y_scale / aspect_ratio;

  Float4x4 matrix = MakeZeroMatrix();
  matrix.m[0] = x_scale;
  matrix.m[5] = y_scale;
  matrix.m[10] = far_plane / (far_plane - near_plane);
  matrix.m[11] = 1.0f;
  matrix.m[14] = (-near_plane * far_plane) / (far_plane - near_plane);
  return matrix;
}

} // namespace cookie::renderer

#include "Cookie/Renderer/Transform.h"

#include <cmath>

namespace cookie::renderer {
namespace {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

Float4x4 MakeZeroMatrix() {
  Float4x4 matrix{};
  for (float& value : matrix.m) {
    value = 0.0f;
  }
  return matrix;
}

Vec3 Subtract(const Vec3& a, const Vec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

float Dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

Vec3 Normalize(const Vec3& value) {
  const float length =
      std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
  if (length <= 0.000001f) {
    return {};
  }
  const float inverse = 1.0f / length;
  return {value.x * inverse, value.y * inverse, value.z * inverse};
}

} // namespace

Float4x4 MakeIdentityTransform() {
  return {};
}

Float4x4 MakeXRotationTransform(float radians) {
  Float4x4 matrix = MakeIdentityTransform();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  matrix.m[5] = c;
  matrix.m[6] = s;
  matrix.m[9] = -s;
  matrix.m[10] = c;
  return matrix;
}

Float4x4 MakeYRotationTransform(float radians) {
  Float4x4 matrix = MakeIdentityTransform();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  matrix.m[0] = c;
  matrix.m[2] = -s;
  matrix.m[8] = s;
  matrix.m[10] = c;
  return matrix;
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

Float4x4 MakeLookAtView(float eye_x, float eye_y, float eye_z, float target_x,
                        float target_y, float target_z, float up_x, float up_y,
                        float up_z) {
  const Vec3 eye{eye_x, eye_y, eye_z};
  const Vec3 target{target_x, target_y, target_z};
  const Vec3 up{up_x, up_y, up_z};

  const Vec3 forward = Normalize(Subtract(target, eye));
  const Vec3 right = Normalize(Cross(up, forward));
  const Vec3 camera_up = Cross(forward, right);

  Float4x4 matrix = MakeIdentityTransform();
  matrix.m[0] = right.x;
  matrix.m[1] = camera_up.x;
  matrix.m[2] = forward.x;

  matrix.m[4] = right.y;
  matrix.m[5] = camera_up.y;
  matrix.m[6] = forward.y;

  matrix.m[8] = right.z;
  matrix.m[9] = camera_up.z;
  matrix.m[10] = forward.z;

  matrix.m[12] = -Dot(eye, right);
  matrix.m[13] = -Dot(eye, camera_up);
  matrix.m[14] = -Dot(eye, forward);
  return matrix;
}

} // namespace cookie::renderer

#pragma once

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {

Float4x4 MakeIdentityTransform();
Float4x4 MakeZRotationTransform(float radians);
Float4x4 MakeTranslationTransform(float x, float y, float z);
Float4x4 MakeScaleTransform(float x, float y, float z);
Float4x4 MultiplyTransforms(const Float4x4& a, const Float4x4& b);
Float4x4 MakeOrthographicProjection(float width, float height, float near_plane,
                                    float far_plane);
Float4x4 MakePerspectiveProjection(float fov_y_radians, float aspect_ratio,
                                   float near_plane, float far_plane);

} // namespace cookie::renderer

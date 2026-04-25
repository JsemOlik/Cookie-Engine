#pragma once

#include <memory>

#include "Cookie/Core/PhysicsBackend.h"

namespace cookie::physics {

std::unique_ptr<cookie::core::IPhysicsBackend> CreateJoltPhysicsBackend();

} // namespace cookie::physics

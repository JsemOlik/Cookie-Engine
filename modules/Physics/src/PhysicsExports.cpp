#include "Cookie/Core/PhysicsBackend.h"
#include "Cookie/Physics/JoltPhysicsBackend.h"

#if defined(_WIN32)
#define COOKIE_PHYSICS_EXPORT __declspec(dllexport)
#else
#define COOKIE_PHYSICS_EXPORT
#endif

extern "C" COOKIE_PHYSICS_EXPORT cookie::core::IPhysicsBackend*
CookieCreatePhysicsBackend() {
  return cookie::physics::CreateJoltPhysicsBackend().release();
}

extern "C" COOKIE_PHYSICS_EXPORT void CookieDestroyPhysicsBackend(
    cookie::core::IPhysicsBackend* backend) {
  delete backend;
}

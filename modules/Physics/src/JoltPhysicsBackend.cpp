#include "Cookie/Physics/JoltPhysicsBackend.h"

#include <memory>

#ifndef COOKIE_HAS_JOLT_PACKAGE
#define COOKIE_HAS_JOLT_PACKAGE 0
#endif

#if __has_include(<Jolt/Jolt.h>)
#include <Jolt/Jolt.h>
#define COOKIE_HAS_JOLT_HEADERS 1
#else
#define COOKIE_HAS_JOLT_HEADERS 0
#endif

namespace cookie::physics {
namespace {

class JoltPhysicsBackend final : public cookie::core::IPhysicsBackend {
 public:
  bool Initialize() override {
    initialized_ = true;
    simulated_time_seconds_ = 0.0f;
    return true;
  }

  cookie::core::PhysicsStepStats StepSimulation(
      const cookie::core::PhysicsStepConfig& config) override {
    if (!initialized_) {
      return {};
    }

    simulated_time_seconds_ += config.delta_time_seconds;
    return {
        .simulated_time_seconds = simulated_time_seconds_,
        .using_jolt_headers =
            (COOKIE_HAS_JOLT_HEADERS != 0) && (COOKIE_HAS_JOLT_PACKAGE != 0),
    };
  }

  void Shutdown() override {
    initialized_ = false;
    simulated_time_seconds_ = 0.0f;
  }

 private:
  bool initialized_ = false;
  float simulated_time_seconds_ = 0.0f;
};

} // namespace

std::unique_ptr<cookie::core::IPhysicsBackend> CreateJoltPhysicsBackend() {
  return std::make_unique<JoltPhysicsBackend>();
}

} // namespace cookie::physics

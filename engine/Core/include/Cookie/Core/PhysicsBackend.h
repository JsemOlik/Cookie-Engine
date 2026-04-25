#pragma once

#include <memory>

namespace cookie::core {

struct PhysicsStepConfig {
  float delta_time_seconds = 1.0f / 60.0f;
};

struct PhysicsStepStats {
  float simulated_time_seconds = 0.0f;
  bool using_jolt_headers = false;
};

class IPhysicsBackend {
 public:
  virtual ~IPhysicsBackend() = default;

  virtual bool Initialize() = 0;
  virtual PhysicsStepStats StepSimulation(const PhysicsStepConfig& config) = 0;
  virtual void Shutdown() = 0;
};

} // namespace cookie::core

#pragma once

namespace cookie::core {

struct AudioUpdateConfig {
  float delta_time_seconds = 0.0f;
};

class IAudioBackend {
 public:
  virtual ~IAudioBackend() = default;

  virtual bool Initialize() = 0;
  virtual void Update(const AudioUpdateConfig& config) = 0;
  virtual void Shutdown() = 0;
};

} // namespace cookie::core

#include "Cookie/Audio/NullAudioBackend.h"

#include <memory>

namespace cookie::audio {
namespace {

class NullAudioBackend final : public cookie::core::IAudioBackend {
 public:
  bool Initialize() override {
    initialized_ = true;
    return true;
  }

  void Update(const cookie::core::AudioUpdateConfig& config) override {
    (void)config;
  }

  void Shutdown() override {
    initialized_ = false;
  }

 private:
  bool initialized_ = false;
};

} // namespace

std::unique_ptr<cookie::core::IAudioBackend> CreateNullAudioBackend() {
  return std::make_unique<NullAudioBackend>();
}

} // namespace cookie::audio

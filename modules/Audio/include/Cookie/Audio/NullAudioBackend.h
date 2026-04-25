#pragma once

#include <memory>

#include "Cookie/Core/AudioBackend.h"

namespace cookie::audio {

std::unique_ptr<cookie::core::IAudioBackend> CreateNullAudioBackend();

} // namespace cookie::audio

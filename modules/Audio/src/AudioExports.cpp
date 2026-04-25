#include "Cookie/Audio/NullAudioBackend.h"
#include "Cookie/Core/AudioBackend.h"

#if defined(_WIN32)
#define COOKIE_AUDIO_EXPORT __declspec(dllexport)
#else
#define COOKIE_AUDIO_EXPORT
#endif

extern "C" COOKIE_AUDIO_EXPORT cookie::core::IAudioBackend*
CookieCreateAudioBackend() {
  return cookie::audio::CreateNullAudioBackend().release();
}

extern "C" COOKIE_AUDIO_EXPORT void CookieDestroyAudioBackend(
    cookie::core::IAudioBackend* backend) {
  delete backend;
}

#include <atomic>

#if defined(_WIN32)
#define COOKIE_GAME_EXPORT __declspec(dllexport)
#else
#define COOKIE_GAME_EXPORT
#endif

namespace {

std::atomic<bool> g_started{false};
std::atomic<int> g_ticks{0};

} // namespace

extern "C" COOKIE_GAME_EXPORT bool CookieGameStartup() {
  g_ticks.store(0);
  g_started.store(true);
  return true;
}

extern "C" COOKIE_GAME_EXPORT void CookieGameUpdate(float delta_seconds) {
  (void)delta_seconds;
  if (!g_started.load()) {
    return;
  }
  g_ticks.fetch_add(1);
}

extern "C" COOKIE_GAME_EXPORT void CookieGameShutdown() {
  g_started.store(false);
}

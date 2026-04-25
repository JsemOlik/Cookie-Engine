#if defined(_WIN32)
#define COOKIE_CORE_EXPORT __declspec(dllexport)
#else
#define COOKIE_CORE_EXPORT
#endif

extern "C" COOKIE_CORE_EXPORT int CookieCoreModuleApiVersion() {
  return 1;
}

extern "C" COOKIE_CORE_EXPORT const char* CookieCoreModuleName() {
  return "CookieCoreModule";
}

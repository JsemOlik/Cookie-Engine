#include "Cookie/Renderer/RendererBackend.h"
#include "Cookie/RendererDX11/RendererDX11Backend.h"

#if defined(_WIN32)
#define COOKIE_RENDERER_DX11_EXPORT __declspec(dllexport)
#else
#define COOKIE_RENDERER_DX11_EXPORT
#endif

extern "C" COOKIE_RENDERER_DX11_EXPORT cookie::renderer::IRendererBackend*
CookieCreateRendererBackend() {
  return cookie::renderer::dx11::CreateRendererDX11Backend().release();
}

extern "C" COOKIE_RENDERER_DX11_EXPORT void CookieDestroyRendererBackend(
    cookie::renderer::IRendererBackend* backend) {
  delete backend;
}

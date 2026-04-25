#pragma once

#include <memory>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer::dx11 {

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend();

} // namespace cookie::renderer::dx11

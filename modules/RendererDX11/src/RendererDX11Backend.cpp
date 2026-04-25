#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <memory>
#include <string_view>

namespace cookie::renderer::dx11 {
namespace {

class RendererDX11Backend final : public IRendererBackend {
 public:
  bool Initialize() override {
    initialized_ = true;
    return true;
  }

  bool BeginFrame() override {
    return initialized_;
  }

  void Clear(const ClearColor& color) override {
    (void)color;
  }

  void EndFrame() override {}

  void Shutdown() override {
    initialized_ = false;
  }

  std::string_view Name() const override {
    return "dx11";
  }

 private:
  bool initialized_ = false;
};

} // namespace

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend() {
  return std::make_unique<RendererDX11Backend>();
}

} // namespace cookie::renderer::dx11

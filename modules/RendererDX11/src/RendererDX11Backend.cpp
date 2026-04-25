#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string_view>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#endif

namespace cookie::renderer::dx11 {
namespace {

class RendererDX11Backend final : public IRendererBackend {
 public:
#if defined(_WIN32)
  bool Initialize(const RendererInitInfo& init_info) override {
    const auto hwnd = static_cast<HWND>(init_info.native_window_handle);
    if (hwnd == nullptr) {
      return false;
    }

    const UINT width = static_cast<UINT>(std::max(init_info.window_width, 1));
    const UINT height = static_cast<UINT>(std::max(init_info.window_height, 1));

    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc.Width = width;
    swap_chain_desc.BufferDesc.Height = height;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.OutputWindow = hwnd;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_flags = 0;
#if defined(_DEBUG)
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL created_feature_level = D3D_FEATURE_LEVEL_11_0;
    const HRESULT create_result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        create_flags,
        feature_levels,
        static_cast<UINT>(std::size(feature_levels)),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &swap_chain_,
        &device_,
        &created_feature_level,
        &device_context_);
    if (FAILED(create_result)) {
#if defined(_DEBUG)
      create_flags &= ~D3D11_CREATE_DEVICE_DEBUG;
      const HRESULT retry_result = D3D11CreateDeviceAndSwapChain(
          nullptr,
          D3D_DRIVER_TYPE_HARDWARE,
          nullptr,
          create_flags,
          feature_levels,
          static_cast<UINT>(std::size(feature_levels)),
          D3D11_SDK_VERSION,
          &swap_chain_desc,
          &swap_chain_,
          &device_,
          &created_feature_level,
          &device_context_);
      if (FAILED(retry_result)) {
        return false;
      }
#else
      return false;
#endif
    }

    ID3D11Texture2D* back_buffer = nullptr;
    const HRESULT back_buffer_result = swap_chain_->GetBuffer(
        0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    if (FAILED(back_buffer_result) || back_buffer == nullptr) {
      Shutdown();
      return false;
    }

    const HRESULT rtv_result =
        device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_);
    back_buffer->Release();
    if (FAILED(rtv_result) || render_target_view_ == nullptr) {
      Shutdown();
      return false;
    }

    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    device_context_->RSSetViewports(1, &viewport);

    vsync_enabled_ = init_info.enable_vsync;
    initialized_ = true;
    return true;
  }
#else
  bool Initialize(const RendererInitInfo& init_info) override {
    (void)init_info;
    return false;
  }
#endif

  bool BeginFrame() override {
#if defined(_WIN32)
    if (initialized_ && device_context_ != nullptr && render_target_view_ != nullptr) {
      device_context_->OMSetRenderTargets(1, &render_target_view_, nullptr);
    }
#endif
    return initialized_;
  }

  void Clear(const ClearColor& color) override {
#if defined(_WIN32)
    if (!initialized_ || device_context_ == nullptr || render_target_view_ == nullptr) {
      return;
    }
    const float clear_color[] = {color.red, color.green, color.blue, color.alpha};
    device_context_->ClearRenderTargetView(render_target_view_, clear_color);
#else
    (void)color;
#endif
  }

  void EndFrame() override {
#if defined(_WIN32)
    if (!initialized_ || swap_chain_ == nullptr) {
      return;
    }
    const UINT sync_interval = vsync_enabled_ ? 1u : 0u;
    swap_chain_->Present(sync_interval, 0);
#endif
  }

  void Shutdown() override {
#if defined(_WIN32)
    if (render_target_view_ != nullptr) {
      render_target_view_->Release();
      render_target_view_ = nullptr;
    }
    if (swap_chain_ != nullptr) {
      swap_chain_->Release();
      swap_chain_ = nullptr;
    }
    if (device_context_ != nullptr) {
      device_context_->Release();
      device_context_ = nullptr;
    }
    if (device_ != nullptr) {
      device_->Release();
      device_ = nullptr;
    }
#endif
    initialized_ = false;
  }

  std::string_view Name() const override {
    return "dx11";
  }

 private:
  bool initialized_ = false;
#if defined(_WIN32)
  bool vsync_enabled_ = true;
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* device_context_ = nullptr;
  IDXGISwapChain* swap_chain_ = nullptr;
  ID3D11RenderTargetView* render_target_view_ = nullptr;
#endif
};

} // namespace

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend() {
  return std::make_unique<RendererDX11Backend>();
}

} // namespace cookie::renderer::dx11

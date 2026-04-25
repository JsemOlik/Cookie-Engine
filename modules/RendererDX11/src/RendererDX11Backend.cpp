#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <string_view>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#endif

namespace cookie::renderer::dx11 {
namespace {

class RendererDX11Backend final : public IRendererBackend {
 public:
  struct Vertex {
    float position[3];
    float color[4];
  };

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

    if (!CreateTrianglePipeline()) {
      Shutdown();
      return false;
    }

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
    DrawTestTriangle();
    const UINT sync_interval = vsync_enabled_ ? 1u : 0u;
    swap_chain_->Present(sync_interval, 0);
#endif
  }

  void Shutdown() override {
#if defined(_WIN32)
    if (vertex_buffer_ != nullptr) {
      vertex_buffer_->Release();
      vertex_buffer_ = nullptr;
    }
    if (input_layout_ != nullptr) {
      input_layout_->Release();
      input_layout_ = nullptr;
    }
    if (pixel_shader_ != nullptr) {
      pixel_shader_->Release();
      pixel_shader_ = nullptr;
    }
    if (vertex_shader_ != nullptr) {
      vertex_shader_->Release();
      vertex_shader_ = nullptr;
    }
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
    triangle_ready_ = false;
  }

  std::string_view Name() const override {
    return "dx11";
  }

 private:
#if defined(_WIN32)
  bool CompileShader(const char* source, const char* entry_point,
                     const char* target, ID3DBlob** out_blob) {
    if (source == nullptr || entry_point == nullptr || target == nullptr ||
        out_blob == nullptr) {
      return false;
    }

    UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* error_blob = nullptr;
    const HRESULT result = D3DCompile(
        source, strlen(source), nullptr, nullptr, nullptr, entry_point, target,
        compile_flags, 0, out_blob, &error_blob);
    if (error_blob != nullptr) {
      error_blob->Release();
      error_blob = nullptr;
    }
    return SUCCEEDED(result) && *out_blob != nullptr;
  }

  bool CreateTrianglePipeline() {
    if (device_ == nullptr) {
      return false;
    }

    constexpr const char* vertex_shader_source = R"(
struct VSIn {
  float3 position : POSITION;
  float4 color : COLOR;
};

struct PSIn {
  float4 position : SV_Position;
  float4 color : COLOR;
};

PSIn VSMain(VSIn input) {
  PSIn output;
  output.position = float4(input.position, 1.0f);
  output.color = input.color;
  return output;
}
)";

    constexpr const char* pixel_shader_source = R"(
struct PSIn {
  float4 position : SV_Position;
  float4 color : COLOR;
};

float4 PSMain(PSIn input) : SV_Target {
  return input.color;
}
)";

    ID3DBlob* vertex_shader_blob = nullptr;
    ID3DBlob* pixel_shader_blob = nullptr;
    if (!CompileShader(vertex_shader_source, "VSMain", "vs_5_0",
                       &vertex_shader_blob)) {
      return false;
    }

    const HRESULT vs_result = device_->CreateVertexShader(
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(), nullptr, &vertex_shader_);
    if (FAILED(vs_result) || vertex_shader_ == nullptr) {
      vertex_shader_blob->Release();
      return false;
    }

    const D3D11_INPUT_ELEMENT_DESC input_layout_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         static_cast<UINT>(offsetof(Vertex, position)),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         static_cast<UINT>(offsetof(Vertex, color)),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    const HRESULT input_layout_result = device_->CreateInputLayout(
        input_layout_desc, static_cast<UINT>(std::size(input_layout_desc)),
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(), &input_layout_);
    if (FAILED(input_layout_result) || input_layout_ == nullptr) {
      vertex_shader_blob->Release();
      return false;
    }

    if (!CompileShader(pixel_shader_source, "PSMain", "ps_5_0",
                       &pixel_shader_blob)) {
      vertex_shader_blob->Release();
      return false;
    }

    const HRESULT ps_result = device_->CreatePixelShader(
        pixel_shader_blob->GetBufferPointer(),
        pixel_shader_blob->GetBufferSize(), nullptr, &pixel_shader_);
    pixel_shader_blob->Release();
    pixel_shader_blob = nullptr;
    if (FAILED(ps_result) || pixel_shader_ == nullptr) {
      vertex_shader_blob->Release();
      return false;
    }

    const Vertex vertices[] = {
        {{0.0f, 0.55f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f}},
        {{0.55f, -0.45f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f}},
        {{-0.55f, -0.45f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}},
    };

    D3D11_BUFFER_DESC vertex_buffer_desc{};
    vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertex_buffer_desc.ByteWidth = static_cast<UINT>(sizeof(vertices));

    D3D11_SUBRESOURCE_DATA vertex_data{};
    vertex_data.pSysMem = vertices;

    const HRESULT buffer_result = device_->CreateBuffer(
        &vertex_buffer_desc, &vertex_data, &vertex_buffer_);
    vertex_shader_blob->Release();
    vertex_shader_blob = nullptr;
    if (FAILED(buffer_result) || vertex_buffer_ == nullptr) {
      return false;
    }

    triangle_ready_ = true;
    return true;
  }

  void DrawTestTriangle() {
    if (!triangle_ready_ || device_context_ == nullptr || vertex_buffer_ == nullptr ||
        vertex_shader_ == nullptr || pixel_shader_ == nullptr ||
        input_layout_ == nullptr) {
      return;
    }

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    device_context_->IASetInputLayout(input_layout_);
    device_context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
    device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context_->VSSetShader(vertex_shader_, nullptr, 0);
    device_context_->PSSetShader(pixel_shader_, nullptr, 0);
    device_context_->Draw(3, 0);
  }
#endif

  bool initialized_ = false;
#if defined(_WIN32)
  bool vsync_enabled_ = true;
  bool triangle_ready_ = false;
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* device_context_ = nullptr;
  IDXGISwapChain* swap_chain_ = nullptr;
  ID3D11RenderTargetView* render_target_view_ = nullptr;
  ID3D11VertexShader* vertex_shader_ = nullptr;
  ID3D11PixelShader* pixel_shader_ = nullptr;
  ID3D11InputLayout* input_layout_ = nullptr;
  ID3D11Buffer* vertex_buffer_ = nullptr;
#endif
};

} // namespace

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend() {
  return std::make_unique<RendererDX11Backend>();
}

} // namespace cookie::renderer::dx11

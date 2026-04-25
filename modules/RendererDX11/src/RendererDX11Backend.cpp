#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
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

    if (!CreateDepthResources(width, height)) {
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

    if (!CreateScenePipeline()) {
      Shutdown();
      return false;
    }

    vsync_enabled_ = init_info.enable_vsync;
    initialized_ = true;
    submitted_scene_ = {};
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
    if (initialized_ && device_context_ != nullptr && render_target_view_ != nullptr &&
        depth_stencil_view_ != nullptr) {
      device_context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);
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
    if (depth_stencil_view_ != nullptr) {
      device_context_->ClearDepthStencilView(
          depth_stencil_view_, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
#else
    (void)color;
#endif
  }

  void SubmitScene(const RenderScene& scene) override {
#if defined(_WIN32)
    submitted_scene_ = scene;
#else
    (void)scene;
#endif
  }

  void EndFrame() override {
#if defined(_WIN32)
    if (!initialized_ || swap_chain_ == nullptr) {
      return;
    }
    DrawSubmittedScene();
    const UINT sync_interval = vsync_enabled_ ? 1u : 0u;
    swap_chain_->Present(sync_interval, 0);
#endif
  }

  void Shutdown() override {
#if defined(_WIN32)
    submitted_scene_ = {};
    if (index_buffer_ != nullptr) {
      index_buffer_->Release();
      index_buffer_ = nullptr;
    }
    index_buffer_capacity_bytes_ = 0;
    if (vertex_buffer_ != nullptr) {
      vertex_buffer_->Release();
      vertex_buffer_ = nullptr;
    }
    vertex_buffer_capacity_bytes_ = 0;
    if (scene_constant_buffer_ != nullptr) {
      scene_constant_buffer_->Release();
      scene_constant_buffer_ = nullptr;
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
    if (depth_stencil_view_ != nullptr) {
      depth_stencil_view_->Release();
      depth_stencil_view_ = nullptr;
    }
    if (depth_stencil_buffer_ != nullptr) {
      depth_stencil_buffer_->Release();
      depth_stencil_buffer_ = nullptr;
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
#if defined(_WIN32)
  struct SceneConstantData {
    float model[16];
    float view_projection[16];
  };

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
        source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target,
        compile_flags, 0, out_blob, &error_blob);
    if (error_blob != nullptr) {
      error_blob->Release();
      error_blob = nullptr;
    }
    return SUCCEEDED(result) && *out_blob != nullptr;
  }

  bool CreateScenePipeline() {
    if (device_ == nullptr) {
      return false;
    }

    constexpr const char* vertex_shader_source = R"(
cbuffer SceneConstants : register(b0) {
  float4x4 u_model;
  float4x4 u_view_projection;
};

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
  float4 world_position = mul(float4(input.position, 1.0f), u_model);
  output.position = mul(world_position, u_view_projection);
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
         static_cast<UINT>(offsetof(SceneVertex, position)),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         static_cast<UINT>(offsetof(SceneVertex, color)),
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

    ID3DBlob* pixel_shader_blob = nullptr;
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

    D3D11_BUFFER_DESC scene_constant_desc{};
    scene_constant_desc.Usage = D3D11_USAGE_DYNAMIC;
    scene_constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    scene_constant_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    scene_constant_desc.ByteWidth = static_cast<UINT>(sizeof(SceneConstantData));

    const HRESULT constant_result =
        device_->CreateBuffer(&scene_constant_desc, nullptr, &scene_constant_buffer_);
    vertex_shader_blob->Release();
    vertex_shader_blob = nullptr;
    if (FAILED(constant_result) || scene_constant_buffer_ == nullptr) {
      return false;
    }

    return true;
  }

  bool CreateDepthResources(UINT width, UINT height) {
    if (device_ == nullptr || width == 0 || height == 0) {
      return false;
    }

    D3D11_TEXTURE2D_DESC depth_desc{};
    depth_desc.Width = width;
    depth_desc.Height = height;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    const HRESULT depth_buffer_result =
        device_->CreateTexture2D(&depth_desc, nullptr, &depth_stencil_buffer_);
    if (FAILED(depth_buffer_result) || depth_stencil_buffer_ == nullptr) {
      return false;
    }

    const HRESULT depth_view_result = device_->CreateDepthStencilView(
        depth_stencil_buffer_, nullptr, &depth_stencil_view_);
    if (FAILED(depth_view_result) || depth_stencil_view_ == nullptr) {
      return false;
    }

    return true;
  }

  bool EnsureVertexBufferCapacity(std::size_t required_bytes) {
    if (required_bytes == 0 || required_bytes > static_cast<std::size_t>(UINT_MAX)) {
      return false;
    }
    if (vertex_buffer_ != nullptr && required_bytes <= vertex_buffer_capacity_bytes_) {
      return true;
    }

    if (vertex_buffer_ != nullptr) {
      vertex_buffer_->Release();
      vertex_buffer_ = nullptr;
      vertex_buffer_capacity_bytes_ = 0;
    }

    D3D11_BUFFER_DESC vertex_buffer_desc{};
    vertex_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertex_buffer_desc.ByteWidth = static_cast<UINT>(required_bytes);

    const HRESULT result =
        device_->CreateBuffer(&vertex_buffer_desc, nullptr, &vertex_buffer_);
    if (FAILED(result) || vertex_buffer_ == nullptr) {
      return false;
    }

    vertex_buffer_capacity_bytes_ = required_bytes;
    return true;
  }

  bool EnsureIndexBufferCapacity(std::size_t required_bytes) {
    if (required_bytes == 0 || required_bytes > static_cast<std::size_t>(UINT_MAX)) {
      return false;
    }
    if (index_buffer_ != nullptr && required_bytes <= index_buffer_capacity_bytes_) {
      return true;
    }

    if (index_buffer_ != nullptr) {
      index_buffer_->Release();
      index_buffer_ = nullptr;
      index_buffer_capacity_bytes_ = 0;
    }

    D3D11_BUFFER_DESC index_buffer_desc{};
    index_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    index_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    index_buffer_desc.ByteWidth = static_cast<UINT>(required_bytes);

    const HRESULT result =
        device_->CreateBuffer(&index_buffer_desc, nullptr, &index_buffer_);
    if (FAILED(result) || index_buffer_ == nullptr) {
      return false;
    }

    index_buffer_capacity_bytes_ = required_bytes;
    return true;
  }

  void CopyMatrix16(float* destination, const Float4x4& source) {
    std::memcpy(destination, source.m, sizeof(source.m));
  }

  void DrawSubmittedScene() {
    if (!initialized_ || device_context_ == nullptr || input_layout_ == nullptr ||
        vertex_shader_ == nullptr || pixel_shader_ == nullptr ||
        scene_constant_buffer_ == nullptr) {
      return;
    }
    if (submitted_scene_.instances == nullptr || submitted_scene_.instance_count == 0) {
      return;
    }

    for (std::size_t index = 0; index < submitted_scene_.instance_count; ++index) {
      DrawSceneInstance(submitted_scene_.instances[index], submitted_scene_.camera);
    }
  }

  void DrawSceneInstance(const RenderMeshInstance& instance, const RenderCamera& camera) {
    if (instance.vertices == nullptr || instance.vertex_count < 3 ||
        instance.vertex_count > static_cast<std::size_t>(UINT_MAX)) {
      return;
    }

    const std::size_t required_bytes = instance.vertex_count * sizeof(SceneVertex);
    if (!EnsureVertexBufferCapacity(required_bytes)) {
      return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped_vertex_buffer{};
    const HRESULT map_result = device_context_->Map(
        vertex_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_buffer);
    if (FAILED(map_result) || mapped_vertex_buffer.pData == nullptr) {
      return;
    }
    std::memcpy(mapped_vertex_buffer.pData, instance.vertices, required_bytes);
    device_context_->Unmap(vertex_buffer_, 0);

    const bool has_indices =
        instance.indices != nullptr && instance.index_count >= 3 &&
        instance.index_count <= static_cast<std::size_t>(UINT_MAX);
    if (has_indices) {
      const std::size_t required_index_bytes =
          instance.index_count * sizeof(std::uint32_t);
      if (!EnsureIndexBufferCapacity(required_index_bytes)) {
        return;
      }

      D3D11_MAPPED_SUBRESOURCE mapped_index_buffer{};
      const HRESULT map_index_result = device_context_->Map(
          index_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_index_buffer);
      if (FAILED(map_index_result) || mapped_index_buffer.pData == nullptr) {
        return;
      }
      std::memcpy(mapped_index_buffer.pData, instance.indices, required_index_bytes);
      device_context_->Unmap(index_buffer_, 0);
    }

    SceneConstantData constants{};
    CopyMatrix16(constants.model, instance.model_transform);
    CopyMatrix16(constants.view_projection, camera.view_projection);

    D3D11_MAPPED_SUBRESOURCE mapped_constants{};
    const HRESULT constant_map_result = device_context_->Map(
        scene_constant_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_constants);
    if (FAILED(constant_map_result) || mapped_constants.pData == nullptr) {
      return;
    }
    std::memcpy(mapped_constants.pData, &constants, sizeof(constants));
    device_context_->Unmap(scene_constant_buffer_, 0);

    const UINT stride = sizeof(SceneVertex);
    const UINT offset = 0;
    device_context_->IASetInputLayout(input_layout_);
    device_context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
    device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context_->VSSetShader(vertex_shader_, nullptr, 0);
    device_context_->VSSetConstantBuffers(0, 1, &scene_constant_buffer_);
    device_context_->PSSetShader(pixel_shader_, nullptr, 0);
    if (has_indices) {
      device_context_->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
      device_context_->DrawIndexed(static_cast<UINT>(instance.index_count), 0, 0);
    } else {
      device_context_->Draw(static_cast<UINT>(instance.vertex_count), 0);
    }
  }
#endif

  bool initialized_ = false;
#if defined(_WIN32)
  bool vsync_enabled_ = true;
  RenderScene submitted_scene_{};
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* device_context_ = nullptr;
  IDXGISwapChain* swap_chain_ = nullptr;
  ID3D11RenderTargetView* render_target_view_ = nullptr;
  ID3D11Texture2D* depth_stencil_buffer_ = nullptr;
  ID3D11DepthStencilView* depth_stencil_view_ = nullptr;
  ID3D11VertexShader* vertex_shader_ = nullptr;
  ID3D11PixelShader* pixel_shader_ = nullptr;
  ID3D11InputLayout* input_layout_ = nullptr;
  ID3D11Buffer* scene_constant_buffer_ = nullptr;
  ID3D11Buffer* vertex_buffer_ = nullptr;
  ID3D11Buffer* index_buffer_ = nullptr;
  std::size_t vertex_buffer_capacity_bytes_ = 0;
  std::size_t index_buffer_capacity_bytes_ = 0;
#endif
};

} // namespace

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend() {
  return std::make_unique<RendererDX11Backend>();
}

} // namespace cookie::renderer::dx11

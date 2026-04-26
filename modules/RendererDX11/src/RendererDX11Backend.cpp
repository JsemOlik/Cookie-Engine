#include "Cookie/RendererDX11/RendererDX11Backend.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wincodec.h>

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

    const HRESULT co_init_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(co_init_result)) {
      com_initialized_ = true;
    } else if (co_init_result != RPC_E_CHANGED_MODE) {
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

    if (!CreateRasterizerState()) {
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
      device_context_->RSSetState(rasterizer_state_);
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
    ReleaseTextureCache();
    submitted_scene_ = {};
    if (vertex_buffer_ != nullptr) {
      vertex_buffer_->Release();
      vertex_buffer_ = nullptr;
    }
    vertex_buffer_capacity_bytes_ = 0;
    if (index_buffer_ != nullptr) {
      index_buffer_->Release();
      index_buffer_ = nullptr;
    }
    index_buffer_capacity_bytes_ = 0;
    if (rasterizer_state_ != nullptr) {
      rasterizer_state_->Release();
      rasterizer_state_ = nullptr;
    }
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
    if (com_initialized_) {
      CoUninitialize();
      com_initialized_ = false;
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
    float world_view_projection[16];
    float tint[4];
    std::uint32_t use_albedo = 0;
    float padding[3] = {0.0f, 0.0f, 0.0f};
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
  row_major float4x4 u_world_view_projection;
  float4 u_tint;
  uint u_use_albedo;
  float3 u_padding;
};

struct VSIn {
  float3 position : POSITION;
  float4 color : COLOR;
  float2 uv : TEXCOORD0;
};

struct PSIn {
  float4 position : SV_Position;
  float4 color : COLOR;
  float2 uv : TEXCOORD0;
};

PSIn VSMain(VSIn input) {
  PSIn output;
  output.position = mul(float4(input.position, 1.0f), u_world_view_projection);
  output.color = input.color;
  output.uv = input.uv;
  return output;
}
)";

    constexpr const char* pixel_shader_source = R"(
cbuffer SceneConstants : register(b0) {
  row_major float4x4 u_world_view_projection;
  float4 u_tint;
  uint u_use_albedo;
  float3 u_padding;
};

Texture2D u_albedo : register(t0);
SamplerState u_sampler : register(s0);

struct PSIn {
  float4 position : SV_Position;
  float4 color : COLOR;
  float2 uv : TEXCOORD0;
};

float4 PSMain(PSIn input) : SV_Target {
  float4 albedo = (u_use_albedo != 0) ? u_albedo.Sample(u_sampler, input.uv)
                                      : float4(1.0f, 1.0f, 1.0f, 1.0f);
  return input.color * u_tint * albedo;
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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         static_cast<UINT>(offsetof(SceneVertex, uv)),
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

    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    const HRESULT sampler_result =
        device_->CreateSamplerState(&sampler_desc, &texture_sampler_state_);
    if (FAILED(sampler_result) || texture_sampler_state_ == nullptr) {
      return false;
    }

    if (!CreateFallbackTexture()) {
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

    const HRESULT depth_result =
        device_->CreateTexture2D(&depth_desc, nullptr, &depth_stencil_buffer_);
    if (FAILED(depth_result) || depth_stencil_buffer_ == nullptr) {
      return false;
    }

    const HRESULT view_result = device_->CreateDepthStencilView(
        depth_stencil_buffer_, nullptr, &depth_stencil_view_);
    return SUCCEEDED(view_result) && depth_stencil_view_ != nullptr;
  }

  bool CreateRasterizerState() {
    if (device_ == nullptr) {
      return false;
    }

    D3D11_RASTERIZER_DESC rasterizer_desc{};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;

    const HRESULT result =
        device_->CreateRasterizerState(&rasterizer_desc, &rasterizer_state_);
    return SUCCEEDED(result) && rasterizer_state_ != nullptr;
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

  void ReleaseTextureCache() {
    for (auto& entry : texture_cache_) {
      if (entry.second != nullptr) {
        entry.second->Release();
      }
    }
    texture_cache_.clear();
    if (fallback_texture_srv_ != nullptr) {
      fallback_texture_srv_->Release();
      fallback_texture_srv_ = nullptr;
    }
    if (texture_sampler_state_ != nullptr) {
      texture_sampler_state_->Release();
      texture_sampler_state_ = nullptr;
    }
  }

  bool CreateFallbackTexture() {
    if (device_ == nullptr) {
      return false;
    }

    const std::array<std::uint8_t, 16> pixels = {
        255, 0, 255, 255,
        20, 20, 20, 255,
        20, 20, 20, 255,
        255, 0, 255, 255,
    };

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = 2;
    texture_desc.Height = 2;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subresource{};
    subresource.pSysMem = pixels.data();
    subresource.SysMemPitch = 2 * 4;

    ID3D11Texture2D* texture = nullptr;
    const HRESULT texture_result =
        device_->CreateTexture2D(&texture_desc, &subresource, &texture);
    if (FAILED(texture_result) || texture == nullptr) {
      return false;
    }

    const HRESULT view_result =
        device_->CreateShaderResourceView(texture, nullptr, &fallback_texture_srv_);
    texture->Release();
    return SUCCEEDED(view_result) && fallback_texture_srv_ != nullptr;
  }

  bool DecodeTextureRgba8(const std::filesystem::path& texture_path,
                          std::vector<std::uint8_t>& out_pixels,
                          UINT& out_width,
                          UINT& out_height) {
    out_pixels.clear();
    out_width = 0;
    out_height = 0;
    if (texture_path.empty()) {
      return false;
    }

    IWICImagingFactory* factory = nullptr;
    const HRESULT factory_result =
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&factory));
    if (FAILED(factory_result) || factory == nullptr) {
      return false;
    }

    const std::wstring wide_path = texture_path.wstring();
    IWICBitmapDecoder* decoder = nullptr;
    const HRESULT decoder_result = factory->CreateDecoderFromFilename(
        wide_path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(decoder_result) || decoder == nullptr) {
      factory->Release();
      return false;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    const HRESULT frame_result = decoder->GetFrame(0, &frame);
    if (FAILED(frame_result) || frame == nullptr) {
      decoder->Release();
      factory->Release();
      return false;
    }

    IWICFormatConverter* converter = nullptr;
    const HRESULT converter_result = factory->CreateFormatConverter(&converter);
    if (FAILED(converter_result) || converter == nullptr) {
      frame->Release();
      decoder->Release();
      factory->Release();
      return false;
    }

    const HRESULT initialize_result = converter->Initialize(
        frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr,
        0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(initialize_result)) {
      converter->Release();
      frame->Release();
      decoder->Release();
      factory->Release();
      return false;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
      converter->Release();
      frame->Release();
      decoder->Release();
      factory->Release();
      return false;
    }

    std::vector<std::uint8_t> pixels;
    pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                  4);
    const HRESULT copy_result = converter->CopyPixels(
        nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();

    if (FAILED(copy_result)) {
      return false;
    }

    out_width = width;
    out_height = height;
    out_pixels = std::move(pixels);
    return true;
  }

  ID3D11ShaderResourceView* ResolveAlbedoTexture(const RenderMaterial* material) {
    if (fallback_texture_srv_ == nullptr) {
      return nullptr;
    }
    if (material == nullptr || !material->use_albedo ||
        material->albedo_texture_path == nullptr ||
        material->albedo_texture_path[0] == '\0') {
      return fallback_texture_srv_;
    }

    const std::string key(material->albedo_texture_path);
    const auto cache_it = texture_cache_.find(key);
    if (cache_it != texture_cache_.end() && cache_it->second != nullptr) {
      return cache_it->second;
    }

    std::vector<std::uint8_t> pixels;
    UINT width = 0;
    UINT height = 0;
    if (!DecodeTextureRgba8(std::filesystem::path(key), pixels, width, height)) {
      if (warned_texture_paths_.insert(key).second) {
        OutputDebugStringA(
            ("Cookie DX11: failed to load texture '" + key +
             "', using fallback texture.\n")
                .c_str());
      }
      return fallback_texture_srv_;
    }

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subresource{};
    subresource.pSysMem = pixels.data();
    subresource.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    const HRESULT texture_result =
        device_->CreateTexture2D(&texture_desc, &subresource, &texture);
    if (FAILED(texture_result) || texture == nullptr) {
      if (warned_texture_paths_.insert(key).second) {
        OutputDebugStringA(
            ("Cookie DX11: failed to create GPU texture for '" + key +
             "', using fallback texture.\n")
                .c_str());
      }
      return fallback_texture_srv_;
    }

    ID3D11ShaderResourceView* srv = nullptr;
    const HRESULT srv_result =
        device_->CreateShaderResourceView(texture, nullptr, &srv);
    texture->Release();
    if (FAILED(srv_result) || srv == nullptr) {
      if (warned_texture_paths_.insert(key).second) {
        OutputDebugStringA(
            ("Cookie DX11: failed to create SRV for '" + key +
             "', using fallback texture.\n")
                .c_str());
      }
      return fallback_texture_srv_;
    }

    texture_cache_[key] = srv;
    return srv;
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
      DrawSceneInstance(submitted_scene_.instances[index], submitted_scene_);
    }
  }

  void DrawSceneInstance(const RenderMeshInstance& instance,
                         const RenderScene& scene) {
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
      const HRESULT index_map_result = device_context_->Map(
          index_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_index_buffer);
      if (FAILED(index_map_result) || mapped_index_buffer.pData == nullptr) {
        return;
      }
      std::memcpy(mapped_index_buffer.pData, instance.indices, required_index_bytes);
      device_context_->Unmap(index_buffer_, 0);
    }

    const RenderMaterial* material = nullptr;
    if (scene.materials != nullptr && scene.material_count > 0 &&
        instance.material_index < scene.material_count) {
      material = &scene.materials[instance.material_index];
    }

    SceneConstantData constants{};
    CopyMatrix16(constants.world_view_projection, instance.model_transform);
    if (material != nullptr) {
      constants.tint[0] = material->tint[0];
      constants.tint[1] = material->tint[1];
      constants.tint[2] = material->tint[2];
      constants.tint[3] = material->tint[3];
      constants.use_albedo = material->use_albedo ? 1u : 0u;
    } else {
      constants.tint[0] = 1.0f;
      constants.tint[1] = 1.0f;
      constants.tint[2] = 1.0f;
      constants.tint[3] = 1.0f;
      constants.use_albedo = 0u;
    }

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
    device_context_->PSSetConstantBuffers(0, 1, &scene_constant_buffer_);
    ID3D11ShaderResourceView* albedo_texture = ResolveAlbedoTexture(material);
    device_context_->PSSetShaderResources(0, 1, &albedo_texture);
    device_context_->PSSetSamplers(0, 1, &texture_sampler_state_);
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
  ID3D11RasterizerState* rasterizer_state_ = nullptr;
  ID3D11SamplerState* texture_sampler_state_ = nullptr;
  ID3D11ShaderResourceView* fallback_texture_srv_ = nullptr;
  std::size_t vertex_buffer_capacity_bytes_ = 0;
  std::size_t index_buffer_capacity_bytes_ = 0;
  std::unordered_map<std::string, ID3D11ShaderResourceView*> texture_cache_{};
  std::unordered_set<std::string> warned_texture_paths_{};
  bool com_initialized_ = false;
#endif
};

} // namespace

std::unique_ptr<IRendererBackend> CreateRendererDX11Backend() {
  return std::make_unique<RendererDX11Backend>();
}

} // namespace cookie::renderer::dx11

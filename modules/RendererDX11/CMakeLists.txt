add_library(cookie_renderer_dx11 STATIC
  src/RendererDX11Backend.cpp
)

target_include_directories(cookie_renderer_dx11
  PUBLIC
    include
)

target_link_libraries(cookie_renderer_dx11
  PUBLIC
    cookie_renderer
)

target_compile_features(cookie_renderer_dx11 PUBLIC cxx_std_20)

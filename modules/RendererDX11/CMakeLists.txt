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

if(WIN32)
  target_link_libraries(cookie_renderer_dx11
    PUBLIC
      d3d11
      dxgi
      d3dcompiler
  )
endif()

target_compile_features(cookie_renderer_dx11 PUBLIC cxx_std_20)

add_library(cookie_renderer_dx11_module SHARED
  src/RendererDX11Backend.cpp
  src/RendererDX11Exports.cpp
)

target_include_directories(cookie_renderer_dx11_module
  PUBLIC
    include
)

target_link_libraries(cookie_renderer_dx11_module
  PUBLIC
    cookie_renderer
)

if(WIN32)
  target_link_libraries(cookie_renderer_dx11_module
    PUBLIC
      d3d11
      dxgi
      d3dcompiler
  )
endif()

target_compile_features(cookie_renderer_dx11_module PUBLIC cxx_std_20)

set_target_properties(cookie_renderer_dx11_module PROPERTIES
  OUTPUT_NAME "RendererDX11"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/CookieRuntime/bin"
)

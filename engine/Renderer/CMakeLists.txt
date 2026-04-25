add_library(cookie_renderer STATIC
  src/RendererConfig.cpp
)

target_include_directories(cookie_renderer
  PUBLIC
    include
)

target_compile_features(cookie_renderer PUBLIC cxx_std_20)

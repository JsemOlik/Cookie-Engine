add_library(cookie_renderer STATIC
  src/MeshAsset.cpp
  src/Primitives.cpp
  src/SceneBuilder.cpp
  src/RendererConfig.cpp
  src/Transform.cpp
)

target_include_directories(cookie_renderer
  PUBLIC
    include
)

target_compile_features(cookie_renderer PUBLIC cxx_std_20)

add_library(cookie_assets STATIC
  src/AssetPackage.cpp
  src/AssetRegistry.cpp
  src/GlbMeshLoader.cpp
)

target_include_directories(cookie_assets
  PUBLIC
    include
)

target_compile_features(cookie_assets PUBLIC cxx_std_20)

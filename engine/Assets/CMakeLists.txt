add_library(cookie_assets STATIC
  src/AssetId.cpp
  src/AssetMeta.cpp
  src/AssetPackage.cpp
  src/AssetRegistry.cpp
  src/CookedAssetRegistry.cpp
  src/MaterialAsset.cpp
  src/SceneAsset.cpp
)

target_include_directories(cookie_assets
  PUBLIC
    include
)

target_compile_features(cookie_assets PUBLIC cxx_std_20)

add_library(cookie_tools_pipeline STATIC
  src/BuildPipeline.cpp
  src/PackageInspector.cpp
)

target_include_directories(cookie_tools_pipeline
  PUBLIC
    include
)

target_link_libraries(cookie_tools_pipeline
  PUBLIC
    cookie_assets
)

target_compile_features(cookie_tools_pipeline PRIVATE cxx_std_20)

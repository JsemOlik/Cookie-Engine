add_library(cookie_core STATIC
  src/Application.cpp
  src/ConfigPaths.cpp
  src/Logger.cpp
)

target_include_directories(cookie_core
  PUBLIC
    include
)

target_link_libraries(cookie_core
  PUBLIC
    cookie_assets
    cookie_platform
    cookie_renderer
)

target_compile_features(cookie_core PUBLIC cxx_std_20)

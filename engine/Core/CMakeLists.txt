add_library(cookie_core STATIC
  src/Application.cpp
  src/ConfigPaths.cpp
  src/EngineConfig.cpp
  src/GameConfig.cpp
  src/GameLogicModule.cpp
  src/Logger.cpp
)

add_library(cookie_core_module SHARED
  src/CoreModuleExports.cpp
)

target_include_directories(cookie_core
  PUBLIC
    include
)

target_include_directories(cookie_core_module
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
target_compile_features(cookie_core_module PUBLIC cxx_std_20)

set_target_properties(cookie_core_module PROPERTIES
  OUTPUT_NAME "Core"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/CookieRuntime/bin"
)

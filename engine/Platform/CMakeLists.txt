add_library(cookie_platform STATIC
  src/PlatformLibrary.cpp
  src/PlatformPaths.cpp
  src/PlatformWindow.cpp
)

target_include_directories(cookie_platform
  PUBLIC
    include
)

string(REPLACE "\\" "\\\\" COOKIE_ENGINE_SOURCE_ROOT_ESCAPED "${CMAKE_SOURCE_DIR}")
target_compile_definitions(cookie_platform
  PUBLIC
    COOKIE_ENGINE_SOURCE_ROOT="${COOKIE_ENGINE_SOURCE_ROOT_ESCAPED}"
)

target_compile_features(cookie_platform PUBLIC cxx_std_20)

if(WIN32)
  target_link_libraries(cookie_platform PUBLIC user32)
endif()

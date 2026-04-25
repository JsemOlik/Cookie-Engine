add_library(cookie_platform STATIC
  src/PlatformPaths.cpp
  src/PlatformWindow.cpp
)

target_include_directories(cookie_platform
  PUBLIC
    include
)

target_compile_features(cookie_platform PUBLIC cxx_std_20)

if(WIN32)
  target_link_libraries(cookie_platform PUBLIC user32)
endif()

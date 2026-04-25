add_library(cookie_platform STATIC
  src/PlatformPaths.cpp
)

target_include_directories(cookie_platform
  PUBLIC
    include
)

target_compile_features(cookie_platform PUBLIC cxx_std_20)

add_executable(CookieRuntime
  src/main.cpp
)

target_link_libraries(CookieRuntime
  PRIVATE
    cookie_core
)

target_compile_features(CookieRuntime PRIVATE cxx_std_20)

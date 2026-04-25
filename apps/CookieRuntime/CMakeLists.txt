add_executable(CookieRuntime
  src/main.cpp
)

target_link_libraries(CookieRuntime
  PRIVATE
    cookie_core
    cookie_audio
    cookie_physics
    cookie_renderer
    cookie_renderer_dx11
)

target_compile_features(CookieRuntime PRIVATE cxx_std_20)

add_dependencies(CookieRuntime
  cookie_renderer_dx11_module
  cookie_audio_module
  cookie_physics_module
)

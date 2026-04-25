add_library(cookie_physics STATIC
  src/JoltPhysicsBackend.cpp
)

target_include_directories(cookie_physics
  PUBLIC
    include
)

target_link_libraries(cookie_physics
  PUBLIC
    cookie_core
)

target_compile_features(cookie_physics PUBLIC cxx_std_20)

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

find_package(Jolt CONFIG QUIET)
if(NOT Jolt_FOUND)
  find_package(unofficial-joltphysics CONFIG QUIET)
endif()

set(COOKIE_HAS_JOLT_PACKAGE 0)
if(TARGET Jolt::Jolt)
  target_link_libraries(cookie_physics PUBLIC Jolt::Jolt)
  set(COOKIE_HAS_JOLT_PACKAGE 1)
elseif(TARGET Jolt)
  target_link_libraries(cookie_physics PUBLIC Jolt)
  set(COOKIE_HAS_JOLT_PACKAGE 1)
elseif(TARGET unofficial::joltphysics::joltphysics)
  target_link_libraries(cookie_physics PUBLIC unofficial::joltphysics::joltphysics)
  set(COOKIE_HAS_JOLT_PACKAGE 1)
endif()

target_compile_definitions(cookie_physics
  PRIVATE
    COOKIE_HAS_JOLT_PACKAGE=${COOKIE_HAS_JOLT_PACKAGE}
)

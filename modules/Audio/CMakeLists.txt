add_library(cookie_audio STATIC
  src/NullAudioBackend.cpp
)

add_library(cookie_audio_module SHARED
  src/NullAudioBackend.cpp
  src/AudioExports.cpp
)

target_include_directories(cookie_audio
  PUBLIC
    include
)

target_include_directories(cookie_audio_module
  PUBLIC
    include
)

target_link_libraries(cookie_audio
  PUBLIC
    cookie_core
)

target_link_libraries(cookie_audio_module
  PUBLIC
    cookie_core
)

target_compile_features(cookie_audio PUBLIC cxx_std_20)
target_compile_features(cookie_audio_module PUBLIC cxx_std_20)

set_target_properties(cookie_audio_module PROPERTIES
  OUTPUT_NAME "Audio"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/CookieRuntime/bin"
)

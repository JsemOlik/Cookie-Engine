add_library(GameLogic SHARED
  src/GameLogic.cpp
)

target_compile_features(GameLogic PRIVATE cxx_std_20)

set_target_properties(GameLogic PROPERTIES
  OUTPUT_NAME "GameLogic"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/CookieRuntime"
)

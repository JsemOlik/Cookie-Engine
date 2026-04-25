add_executable(CookiePakTool
  src/main.cpp
)

target_link_libraries(CookiePakTool
  PRIVATE
    cookie_assets
)

target_compile_features(CookiePakTool PRIVATE cxx_std_20)

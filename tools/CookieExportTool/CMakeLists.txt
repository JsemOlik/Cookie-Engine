add_executable(CookieExportTool
  src/main.cpp
)

target_link_libraries(CookieExportTool
  PRIVATE
    cookie_tools_pipeline
)

target_compile_features(CookieExportTool PRIVATE cxx_std_20)

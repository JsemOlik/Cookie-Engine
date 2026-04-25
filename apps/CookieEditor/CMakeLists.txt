find_package(Qt6 COMPONENTS Widgets QUIET)

if(NOT Qt6Widgets_FOUND)
  message(STATUS "Qt6 Widgets not found; skipping CookieEditor target.")
  return()
endif()

qt_standard_project_setup()

qt_add_executable(CookieEditor
  src/main.cpp
  src/EditorMainWindow.cpp
)

target_include_directories(CookieEditor
  PRIVATE
    include
)

target_link_libraries(CookieEditor
  PRIVATE
    Qt6::Widgets
    cookie_core
    cookie_renderer
    cookie_assets
)

target_compile_features(CookieEditor PRIVATE cxx_std_20)

if(WIN32)
  add_custom_command(TARGET CookieEditor POST_BUILD
    COMMAND "$<TARGET_FILE:Qt6::windeployqt>"
            "$<TARGET_FILE:CookieEditor>"
    COMMENT "Deploying Qt runtime dependencies for CookieEditor"
    VERBATIM
  )
endif()

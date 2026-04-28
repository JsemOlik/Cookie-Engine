find_package(Qt6 COMPONENTS Widgets QUIET)

if(NOT Qt6Widgets_FOUND)
  message(STATUS "Qt6 Widgets not found; skipping CookieEditor target.")
  return()
endif()

qt_standard_project_setup()

qt_add_executable(CookieEditor
  include/Cookie/Editor/EditorMainWindow.h
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
    cookie_tools_pipeline
)

target_compile_features(CookieEditor PRIVATE cxx_std_20)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" COOKIE_ENGINE_SOURCE_ROOT_PATH)
target_compile_definitions(CookieEditor
  PRIVATE
    COOKIE_ENGINE_SOURCE_ROOT="${COOKIE_ENGINE_SOURCE_ROOT_PATH}"
)

set_target_properties(CookieEditor PROPERTIES
  AUTOMOC ON
)

if(WIN32)
  add_custom_command(TARGET CookieEditor PRE_LINK
    COMMAND "${CMAKE_COMMAND}"
            -DCOOKIE_EDITOR_TARGET="$<TARGET_FILE:CookieEditor>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TerminateRunningEditor.cmake"
    COMMENT "Ensuring stale CookieEditor process is not locking target executable"
    VERBATIM
  )

  add_custom_command(TARGET CookieEditor POST_BUILD
    COMMAND "$<TARGET_FILE:Qt6::windeployqt>"
            "$<TARGET_FILE:CookieEditor>"
    COMMENT "Deploying Qt runtime dependencies for CookieEditor"
    VERBATIM
  )
endif()

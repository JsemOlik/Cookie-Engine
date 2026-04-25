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
  set(VCPKG_QT_ROOT "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows")

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(QT_PLUGIN_DIR "${VCPKG_QT_ROOT}/debug/plugins")
  else()
    set(QT_PLUGIN_DIR "${VCPKG_QT_ROOT}/plugins")
  endif()

  add_custom_command(TARGET CookieEditor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:CookieEditor>/platforms"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:CookieEditor>/styles"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QT_PLUGIN_DIR}/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:CookieEditor>/platforms/"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QT_PLUGIN_DIR}/styles/qmodernwindowsstyle$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:CookieEditor>/styles/"
    COMMENT "Copying Qt platform/style plugins for CookieEditor"
    VERBATIM
  )
endif()

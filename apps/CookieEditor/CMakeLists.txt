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
  get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(QT6_BIN_DIR "${QT6_QMAKE_EXECUTABLE}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_BIN_DIR}")

  if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET CookieEditor POST_BUILD
      COMMAND "${WINDEPLOYQT_EXECUTABLE}"
              --dir "$<TARGET_FILE_DIR:CookieEditor>"
              --no-compiler-runtime
              "$<TARGET_FILE:CookieEditor>"
      COMMENT "Deploying Qt runtime for CookieEditor"
      VERBATIM
    )
  else()
    message(WARNING "windeployqt not found; CookieEditor may fail to launch outside IDE.")
  endif()
endif()

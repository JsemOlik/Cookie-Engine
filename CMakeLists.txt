cmake_minimum_required(VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(CookieEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Phase 1 adds the first runtime/core/platform skeleton.
add_subdirectory(engine/Platform)
add_subdirectory(engine/Assets)
add_subdirectory(engine/Core)
add_subdirectory(engine/Renderer)
add_subdirectory(modules/RendererDX11)
add_subdirectory(modules/Physics)
add_subdirectory(modules/Audio)
add_subdirectory(game)
add_subdirectory(tools)
add_subdirectory(apps/CookieRuntime)
add_subdirectory(apps/CookieEditor)

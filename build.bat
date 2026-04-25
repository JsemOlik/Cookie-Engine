@echo off
setlocal EnableExtensions
set "EXIT_CODE=0"

cd /d "%~dp0"

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=x64-debug-vcpkg"

if /I "%PRESET%"=="x64-debug-vcpkg" (
  set "TRIPLET=x64-windows"
  set "BUILD_DIR=C:\ce-build\x64-debug-vcpkg"
  set "INSTALL_DIR=C:\ce-install\x64-debug-vcpkg"
) else if /I "%PRESET%"=="x64-release-vcpkg" (
  set "TRIPLET=x64-windows"
  set "BUILD_DIR=C:\ce-build\x64-release-vcpkg"
  set "INSTALL_DIR=C:\ce-install\x64-release-vcpkg"
) else (
  echo [ERROR] Unsupported preset "%PRESET%".
  echo [INFO] Supported presets: x64-debug-vcpkg, x64-release-vcpkg
  set "EXIT_CODE=1"
  goto :end
)

if not defined VCPKG_ROOT (
  set "VCPKG_ROOT=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\vcpkg"
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
  echo [ERROR] VCPKG_ROOT is not valid: "%VCPKG_ROOT%"
  echo [INFO] Set VCPKG_ROOT to your vcpkg root and run again.
  set "EXIT_CODE=1"
  goto :end
)

where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cmake is not available on PATH.
  set "EXIT_CODE=1"
  goto :end
)

set "NINJA_EXE="
for /f "delims=" %%I in ('where ninja 2^>nul') do (
  if not defined NINJA_EXE set "NINJA_EXE=%%I"
)

if not defined NINJA_EXE (
  if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
    set "NINJA_EXE=%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
  )
)

if not defined NINJA_EXE (
  if exist "%ProgramFiles%\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
    set "NINJA_EXE=%ProgramFiles%\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
  )
)

if not defined NINJA_EXE (
  echo [ERROR] Ninja was not found.
  echo [INFO] Install Ninja or use a Visual Studio Developer Command Prompt.
  set "EXIT_CODE=1"
  goto :end
)

echo [INFO] Project root: "%cd%"
echo [INFO] Using preset: %PRESET%
echo [INFO] Using VCPKG_ROOT: "%VCPKG_ROOT%"
echo [INFO] Using Ninja: "%NINJA_EXE%"
echo [INFO] Cleaning previous build artifacts...

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%INSTALL_DIR%" rmdir /s /q "%INSTALL_DIR%"

echo [INFO] Configuring...
cmake --preset "%PRESET%" --fresh -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"
if errorlevel 1 (
  echo [ERROR] Configure failed.
  set "EXIT_CODE=1"
  goto :end
)

echo [INFO] Building...
cmake --build --preset "%PRESET%" --parallel
if errorlevel 1 (
  echo [ERROR] Build failed.
  set "EXIT_CODE=1"
  goto :end
)

echo [SUCCESS] Build completed for preset %PRESET%.
echo [INFO] Build output: "%BUILD_DIR%"

:end
echo.
set /p _COOKIE_ENGINE_CLOSE_PROMPT=Press Enter to close...
exit /b %EXIT_CODE%

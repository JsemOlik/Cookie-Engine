@echo off
setlocal EnableExtensions

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
  exit /b 1
)

if not defined VCPKG_ROOT (
  set "VCPKG_ROOT=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\vcpkg"
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
  echo [ERROR] VCPKG_ROOT is not valid: "%VCPKG_ROOT%"
  echo [INFO] Set VCPKG_ROOT to your vcpkg root and run again.
  exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cmake is not available on PATH.
  exit /b 1
)

echo [INFO] Project root: "%cd%"
echo [INFO] Using preset: %PRESET%
echo [INFO] Using VCPKG_ROOT: "%VCPKG_ROOT%"
echo [INFO] Cleaning previous build artifacts...

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%INSTALL_DIR%" rmdir /s /q "%INSTALL_DIR%"

echo [INFO] Configuring...
cmake --preset "%PRESET%" --fresh
if errorlevel 1 (
  echo [ERROR] Configure failed.
  exit /b 1
)

echo [INFO] Building...
cmake --build --preset "%PRESET%" --parallel
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo [SUCCESS] Build completed for preset %PRESET%.
echo [INFO] Build output: "%BUILD_DIR%"
exit /b 0

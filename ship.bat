@echo off
setlocal EnableExtensions
set "EXIT_CODE=0"

cd /d "%~dp0"

set "PRESET=x64-release-vcpkg"
set "GAME_NAME=%~1"
if "%GAME_NAME%"=="" set "GAME_NAME=MyGame"

set "BUILD_DIR=C:\ce-build\x64-release-vcpkg"
set "RUNTIME_DIR=%BUILD_DIR%\apps\CookieRuntime"
set "EXPORT_TOOL=%BUILD_DIR%\tools\CookieExportTool\CookieExportTool.exe"
set "EXPORT_PARENT=%cd%\out\ship"
set "EXPORT_ROOT=%EXPORT_PARENT%\%GAME_NAME%"
set "EXPORT_EXE=%EXPORT_ROOT%\%GAME_NAME%.exe"
set "EXPORT_LOG=%EXPORT_ROOT%\logs\latest.log"

echo [INFO] Ship candidate flow started.
echo [INFO] Game name: %GAME_NAME%
echo [INFO] Preset: %PRESET%

set "COOKIE_ENGINE_NO_PAUSE=1"
call "%cd%\build.bat" %PRESET%
if errorlevel 1 (
  echo [ERROR] Build step failed.
  set "EXIT_CODE=1"
  goto :end
)
set "COOKIE_ENGINE_NO_PAUSE="

if not exist "%EXPORT_TOOL%" (
  echo [ERROR] Export tool not found: "%EXPORT_TOOL%"
  set "EXIT_CODE=1"
  goto :end
)

echo [INFO] Exporting with release profile...
"%EXPORT_TOOL%" "%cd%" "%RUNTIME_DIR%" "%EXPORT_PARENT%" "%GAME_NAME%" release
if errorlevel 1 (
  echo [ERROR] Export step failed.
  set "EXIT_CODE=1"
  goto :end
)

echo [INFO] Validating exported folder integrity...
if not exist "%EXPORT_ROOT%" (
  echo [ERROR] Export root missing: "%EXPORT_ROOT%"
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_EXE%" (
  echo [ERROR] Exported executable missing: "%EXPORT_EXE%"
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\bin\Core.dll" (
  echo [ERROR] Missing module: Core.dll
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\bin\RendererDX11.dll" (
  echo [ERROR] Missing module: RendererDX11.dll
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\bin\Physics.dll" (
  echo [ERROR] Missing module: Physics.dll
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\bin\Audio.dll" (
  echo [ERROR] Missing module: Audio.dll
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\bin\GameLogic.dll" (
  echo [ERROR] Missing module: GameLogic.dll
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\content\base.pak" (
  echo [ERROR] Missing asset package: content\base.pak
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\content\textures\debug.png" (
  echo [ERROR] Missing texture asset: content\textures\debug.png
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\content\models\test_mesh.glb" (
  echo [ERROR] Missing mesh asset: content\models\test_mesh.glb
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\config\engine.json" (
  echo [ERROR] Missing config: config\engine.json
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\config\graphics.json" (
  echo [ERROR] Missing config: config\graphics.json
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\config\input.cfg" (
  echo [ERROR] Missing config: config\input.cfg
  set "EXIT_CODE=1"
  goto :end
)
if not exist "%EXPORT_ROOT%\config\game.json" (
  echo [ERROR] Missing config: config\game.json
  set "EXIT_CODE=1"
  goto :end
)
if exist "%EXPORT_ROOT%\config\engine.dev.json" (
  echo [ERROR] Unexpected profile file in export: config\engine.dev.json
  set "EXIT_CODE=1"
  goto :end
)
if exist "%EXPORT_ROOT%\config\engine.release.json" (
  echo [ERROR] Unexpected profile file in export: config\engine.release.json
  set "EXIT_CODE=1"
  goto :end
)

echo [INFO] Running strict-mode validation from exported game...
if exist "%EXPORT_LOG%" del /q "%EXPORT_LOG%"
pushd "%EXPORT_ROOT%"
"%EXPORT_EXE%" --engine-config config\engine.json
set "RUN_EXIT=%ERRORLEVEL%"
popd

if not "%RUN_EXIT%"=="0" (
  echo [ERROR] Exported runtime exited with code %RUN_EXIT%.
  set "EXIT_CODE=1"
  goto :end
)

if not exist "%EXPORT_LOG%" (
  echo [ERROR] Exported runtime log not found: "%EXPORT_LOG%"
  set "EXIT_CODE=1"
  goto :end
)

findstr /C:"Strict module mode: true" "%EXPORT_LOG%" >nul || (
  echo [ERROR] Strict module mode was not enabled in exported runtime.
  set "EXIT_CODE=1"
  goto :end
)
findstr /C:"Core runtime source: module" "%EXPORT_LOG%" >nul || (
  echo [ERROR] Core module was not loaded in module mode.
  set "EXIT_CODE=1"
  goto :end
)
findstr /C:"Renderer runtime source: module" "%EXPORT_LOG%" >nul || (
  echo [ERROR] Renderer module was not loaded in module mode.
  set "EXIT_CODE=1"
  goto :end
)
findstr /C:"Physics runtime source: module" "%EXPORT_LOG%" >nul || (
  echo [ERROR] Physics module was not loaded in module mode.
  set "EXIT_CODE=1"
  goto :end
)
findstr /C:"Audio runtime source: module" "%EXPORT_LOG%" >nul || (
  echo [ERROR] Audio module was not loaded in module mode.
  set "EXIT_CODE=1"
  goto :end
)
findstr /C:"static-fallback" "%EXPORT_LOG%" >nul && (
  echo [ERROR] Strict mode run reported fallback usage.
  set "EXIT_CODE=1"
  goto :end
)

echo [SUCCESS] Ship candidate is valid.
echo [INFO] Export root: "%EXPORT_ROOT%"
echo [INFO] Validation log: "%EXPORT_LOG%"

:end
if not defined COOKIE_ENGINE_NO_PAUSE (
  echo.
  set /p _COOKIE_ENGINE_CLOSE_PROMPT=Press Enter to close...
)
exit /b %EXIT_CODE%

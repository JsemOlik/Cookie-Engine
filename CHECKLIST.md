# Cookie Engine Checklist

## Current Phase: Phase 8 - Packaging And Export

Status: completed (skeleton scope)

## Completed

- [x] Analyzed the default CMake scaffold.
- [x] Decided to replace the hello-world `Cookie-Engine/` target with a clean architecture.
- [x] Added `PLAN.md`.
- [x] Added `CHECKLIST.md`.
- [x] Added `vcpkg.json` with no early dependencies.
- [x] Added initial repository architecture folders with tracked placeholders.
- [x] Removed the default hello-world source target.
- [x] Updated the root CMake project to represent the architecture-only phase.
- [x] Removed the unused legacy `Cookie-Engine/` folder from the repository.
- [x] Added `engine/Platform` skeleton with path helper interfaces.
- [x] Added `engine/Core` skeleton with startup path discovery and file logger.
- [x] Added `apps/CookieRuntime` executable skeleton wired through core/platform modules.
- [x] Updated root CMake to build the new Phase 1 skeleton targets.
- [x] Fixed project root discovery by walking up directories to repository markers.
- [x] Added API-neutral renderer interfaces in `engine/Renderer`.
- [x] Added renderer config loading from `config/graphics.json`.
- [x] Added `modules/RendererDX11` backend skeleton (initialize/shutdown/name only, no DirectX calls).
- [x] Wired `CookieRuntime` to select backend by config and run initialize/shutdown lifecycle.
- [x] Added placeholder config files (`engine.json`, `graphics.json`, `input.cfg`, `game.json`).
- [x] Added platform window abstraction (`IPlatformWindow`) in `engine/Platform`.
- [x] Added Win32 window creation and message polling implementation behind platform interfaces.
- [x] Extended renderer abstraction with frame lifecycle methods (`BeginFrame`, `Clear`, `EndFrame`).
- [x] Updated DX11 backend skeleton to implement frame lifecycle as non-rendering stubs.
- [x] Added runtime frame loop that drives renderer methods through interfaces.
- [x] Extended `graphics.json` support with window settings, clear color, and optional `max_frames`.
- [x] Fixed Win32 window API usage to explicit Unicode (`W`) calls for MSVC builds.
- [x] Fixed Win32 cursor resource id for `LoadCursorW` in non-UNICODE builds using `MAKEINTRESOURCEW(32512)`.
- [x] Added `engine/Assets` module with package loader and asset registry interfaces.
- [x] Added text `.pak` skeleton loader for mounting and listing asset IDs.
- [x] Added runtime asset package mount attempt for `content/base.pak` with log output.
- [x] Added `CookiePakTool` CLI under `tools/` for quick package inspection.
- [x] Added sample `content/base.pak` package manifest.
- [x] Added physics backend interface in core (`IPhysicsBackend`).
- [x] Added `modules/Physics` with `CreateJoltPhysicsBackend()` factory and simulation step skeleton.
- [x] Added compile-time Jolt header detection (`__has_include(<Jolt/Jolt.h>)`) in physics backend.
- [x] Wired runtime to initialize physics, run simulation steps in frame loop, and shut physics down.
- [x] Added `joltphysics` dependency to `vcpkg.json` for this phase.
- [x] Added optional `x64-debug-vcpkg` and `x64-release-vcpkg` presets for manifest/toolchain builds.
- [x] Added physics-side CMake auto-link for common Jolt package targets when available.
- [x] Added runtime hint log when Jolt headers are not active.
- [x] Added `apps/CookieEditor` Qt Widgets skeleton with scene viewport, game viewport, hierarchy, inspector, asset browser, and console placeholders.
- [x] Added `qtbase` dependency to `vcpkg.json`.
- [x] Added CMake gating so CookieEditor target is skipped cleanly when Qt6 Widgets is unavailable.
- [x] Added `builtin-baseline` to `vcpkg.json` for compatibility with newer manifest-mode vcpkg.
- [x] Repinned `builtin-baseline` to a real public microsoft/vcpkg commit (`52f5569...`) after rejecting bundled-tool hash.
- [x] Added root `build.bat` to clean and rebuild via vcpkg presets.
- [x] Shortened vcpkg preset build/install directories to avoid deep-path Qt build failures on Windows.
- [x] Added compile-time source-root fallback so runtime resolves `config/` and `content/` correctly from short build directories.
- [x] Added Windows post-build Qt plugin copy for `CookieEditor` (platform/style) from vcpkg tree.
- [x] Switched editor plugin deployment to copy plugin directories (handles debug/release Qt plugin naming differences).
- [x] Updated `build.bat` to wait for Enter before closing so build output remains visible.
- [x] Updated `build.bat` to auto-detect Ninja (PATH or Visual Studio bundled) and pass `CMAKE_MAKE_PROGRAM`.
- [x] Updated `build.bat` to auto-load Visual Studio `VsDevCmd` when `cl.exe` is missing.
- [x] Added `engine/Platform` dynamic library abstraction (`PlatformLibrary`) for module loading.
- [x] Added `engine/Core` game logic module contract loader (`GameLogicModule`).
- [x] Added `game/GameLogic` shared library target with exported startup/update/shutdown functions.
- [x] Wired runtime to attempt loading `GameLogic.dll` and call lifecycle hooks during frame loop.
- [x] Updated runtime phase completion log to Phase 7 milestone text.
- [x] Added `tools/CookieExportTool` CLI skeleton for exporting game folder layout.
- [x] Wired `CookieExportTool` into `tools/CMakeLists.txt`.
- [x] Added export flow to create `bin/`, `content/`, `config/`, and `logs/` folders.
- [x] Added export copy behavior for runtime executable (`CookieRuntime.exe` to `<GameName>.exe`).
- [x] Added export copy behavior for `GameLogic.dll` when present.
- [x] Added export report output (`export_report.txt`) documenting warnings and future module placeholders.
- [x] Updated startup path resolution to prefer executable directory when it matches exported game layout.
- [x] Added platform helper for executable directory lookup (`GetExecutableDirectory`).

## Not Started

- [x] Runtime executable skeleton.
- [x] Cookie Editor Qt Widgets shell.
- [x] Renderer abstraction.
- [x] DirectX 11 renderer module skeleton.
- [x] Platform window abstraction skeleton.
- [x] Runtime frame loop skeleton.
- [x] Jolt physics integration (skeleton boundary + header detection).
- [x] Asset package format and `.pak` tooling (skeleton text format).
- [x] Game logic module loading.
- [x] Exported game packaging (skeleton).
- [ ] OpenGL backend.
- [ ] Save support.
- [ ] Mod support.

## What Should Be Tested

- [x] Confirm the requested architecture folders exist.
- [x] Confirm `vcpkg.json` is valid JSON and has an empty dependency list.
- [ ] Configure and build `CookieRuntime` with Visual Studio/MSVC (`cl.exe`) using `x64-debug` preset.
- [ ] Run `CookieRuntime` and confirm it creates `logs/latest.log`.
- [ ] Confirm `logs/latest.log` contains repository-root `Project root` (not `out/build/...`) and config file paths under repository `config/`.
- [ ] Confirm `logs/latest.log` contains renderer lifecycle lines:
  - `Selected renderer backend: dx11`
  - `Initializing renderer backend: dx11`
  - `Renderer backend initialized successfully.`
  - `Renderer backend shut down successfully.`
- [ ] Confirm `CookieRuntime` opens a window with title from `graphics.json`.
- [ ] Confirm window closes after `max_frames` is reached (currently `300`) or via manual close.
- [ ] Confirm `logs/latest.log` includes frame loop start/end lines and total frame count.
- [ ] Confirm `cmake --build out/build/x64-debug` succeeds after the Unicode Win32 API fix.
- [ ] Confirm runtime log includes asset mount lines for `content/base.pak` and discovered asset count.
- [ ] Build and run `CookiePakTool` against `content/base.pak` and confirm listed asset IDs.
- [ ] Confirm runtime log includes:
  - `Initializing physics backend.`
  - `Physics backend initialized successfully.`
  - `Physics backend using Jolt headers: true` (expected when building with a vcpkg preset and Jolt target resolves)
  - `Physics backend shut down successfully.`
- [ ] Confirm runtime log hint appears when Jolt is inactive:
  - `Hint: configure/build with x64-debug-vcpkg preset to enable vcpkg Jolt integration.`
- [ ] Configure/build with `x64-debug-vcpkg` and verify `Physics backend using Jolt headers: true`.
- [ ] Configure/build with `x64-debug-vcpkg` and verify `CookieEditor` target is generated.
- [ ] Run `CookieEditor` and confirm dock panels open:
  - `Hierarchy`, `Inspector`, `Asset Browser`, `Console`, `Game Viewport`
- [ ] Confirm `CookieEditor.exe` launches directly without Qt platform plugin error.
- [ ] Confirm `CookieEditor.exe` launches directly without Qt platform plugin error after plugin copy step.
- [ ] Confirm runtime log `Project root` points to repo root (not `C:\ce-build\...`) and `content/base.pak` mounts successfully.
- [ ] Run `build.bat` (default) and confirm it cleans previous outputs then configures/builds with `x64-debug-vcpkg`.
- [ ] Confirm runtime logs either:
  - `Loaded game logic module: ...GameLogic.dll`, or
  - `Game logic module not loaded. Looked in repo/bin and runtime directory.`
- [ ] Confirm runtime logs include:
  - `Game logic startup completed.`
  - `Game logic shutdown completed.`
  - `Phase 7 skeleton complete. Game logic module contract wired.`
- [ ] Build `CookieExportTool` and run:
  - `CookieExportTool <project-root> <runtime-build-dir> <export-parent-dir> <game-name>`
- [ ] Confirm export output contains:
  - `<game-name>/<game-name>.exe`
  - `<game-name>/bin/` (with `GameLogic.dll` when available)
  - `<game-name>/content/` copied from project `content/`
  - `<game-name>/config/` copied from project `config/`
  - `<game-name>/logs/`
  - `<game-name>/export_report.txt`
- [ ] Run exported `<game-name>.exe` from export root and confirm `logs/latest.log` is created under exported `<game-name>/logs/` (not repository root logs).
- [x] Confirm no real DirectX rendering code, physics, editor UI, full binary packer/export pipeline, OpenGL, save, or mod implementation was added.

## Verification Notes

- `rg --files --hidden` shows the tracked `.gitkeep` placeholders for the requested folder architecture.
- `vcpkg.json` parses successfully and reports `dependencies=0`.
- `cmake --preset x64-debug` was attempted in this terminal, but `cl.exe` is not on `PATH` here, so compiler detection failed before generation.
- The leftover empty `Cookie-Engine/` directory was removed.
- Phase 1 skeleton code was added without introducing renderer/editor/physics implementations.
- Phase 2 added renderer abstraction and DX11 backend skeleton only (no DirectX API usage yet).
- Runtime backend selection currently reads `renderer` from `config/graphics.json` and defaults to `dx11` if the file is missing.
- Phase 3 added platform windowing and frame loop contracts while keeping renderer calls API-neutral.
- Win32 implementation is contained in `engine/Platform`; non-Windows currently returns no window instance.
- Build error cause: mixed narrow/wide Win32 APIs in `PlatformWindow.cpp` under MSVC.
- Fix applied: switched to `RegisterClassW`, `CreateWindowExW`, `DefWindowProcW`, `LoadCursorW`, and `GetModuleHandleW`.
- Follow-up fix: switched cursor id to `MAKEINTRESOURCEW(32512)` for `LoadCursorW` compatibility across SDK setups.
- Phase 4 introduces a temporary text `.pak` skeleton for asset mount/list flow only.
- Runtime now attempts to mount `content/base.pak` and logs package/asset counts.
- Phase 5 keeps physics API-neutral at core level and isolates backend implementation in `modules/Physics`.
- Current physics step is deterministic skeleton logic, not full rigid-body simulation yet.
- If runtime shows `Physics backend using Jolt headers: false`, build likely used non-vcpkg preset/toolchain.
- Phase 6 adds editor shell scaffolding only; no scene editing, serialization, or gameplay tooling yet.
- If `cmake --preset x64-debug-vcpkg` fails with baseline error, ensure `builtin-baseline` remains pinned in `vcpkg.json`.
- `build.bat` defaults to `x64-debug-vcpkg`; pass `x64-release-vcpkg` as argument for release builds.
- vcpkg presets now use `C:\ce-build\...` and `C:\ce-install\...` for shorter Windows paths.
- Runtime now uses compile-time repo root as first path-resolution candidate when working directory is outside source tree.
- CookieEditor now copies `qwindows*.dll` and `qmodernwindowsstyle*.dll` from vcpkg plugin folders during post-build.
- CookieEditor plugin deployment now copies full `platforms/` and `styles/` directories to avoid hardcoded DLL-name mismatches.
- `build.bat` now pauses with `Press Enter to close...` on both success and failure paths.
- `build.bat` now injects `-DCMAKE_MAKE_PROGRAM=<ninja-path>` during configure to avoid `CMAKE_MAKE_PROGRAM is not set`.
- `build.bat` now attempts to bootstrap MSVC environment automatically via `VsDevCmd.bat`.
- Phase 7 adds dynamic game module loading shape without introducing scripting/runtime reflection yet.
- Runtime now probes for `GameLogic.dll` in `project_root/bin` and runtime output directory fallback.
- Phase 8 adds an export CLI skeleton that assembles the game folder layout and copies available runtime artifacts.
- Export currently reports future module placeholders (`Core.dll`, `RendererDX11.dll`, `Physics.dll`, `Audio.dll`) rather than producing those DLLs yet.
- Runtime startup path detection now treats executable directory as project root when it contains exported `config/`, `content/`, and `logs/`.

## Best Next Step

Start next phase planning for runtime module packaging details (shared engine DLL targets), then iterate export tool integration with editor workflows.

## Suggested Commit Message

```text
fix: resolve exported runtime logs and config paths from executable directory
```

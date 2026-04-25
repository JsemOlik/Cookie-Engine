# Cookie Engine Checklist

## Current Phase: Phase 6 - Cookie Editor Qt Widgets Skeleton

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
- [x] Added `windeployqt` post-build deployment for `CookieEditor` on Windows.
- [x] Updated `build.bat` to wait for Enter before closing so build output remains visible.
- [x] Updated `build.bat` to auto-detect Ninja (PATH or Visual Studio bundled) and pass `CMAKE_MAKE_PROGRAM`.

## Not Started

- [x] Runtime executable skeleton.
- [x] Cookie Editor Qt Widgets shell.
- [x] Renderer abstraction.
- [x] DirectX 11 renderer module skeleton.
- [x] Platform window abstraction skeleton.
- [x] Runtime frame loop skeleton.
- [x] Jolt physics integration (skeleton boundary + header detection).
- [x] Asset package format and `.pak` tooling (skeleton text format).
- [ ] Game logic module loading.
- [ ] Exported game packaging.
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
- [ ] Confirm runtime log `Project root` points to repo root (not `C:\ce-build\...`) and `content/base.pak` mounts successfully.
- [ ] Run `build.bat` (default) and confirm it cleans previous outputs then configures/builds with `x64-debug-vcpkg`.
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
- `build.bat` now pauses with `Press Enter to close...` on both success and failure paths.
- `build.bat` now injects `-DCMAKE_MAKE_PROGRAM=<ninja-path>` during configure to avoid `CMAKE_MAKE_PROGRAM is not set`.

## Best Next Step

Start Phase 7 by adding game logic module loading contract (`GameLogic.dll`-style boundary) without full gameplay scripting yet.

## Suggested Commit Message

```text
chore: add vcpkg clean-build batch script
```

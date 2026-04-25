# Cookie Engine Checklist

## Current Phase: Phase 1 - Core And Runtime Skeleton

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

## Not Started

- [x] Runtime executable skeleton.
- [ ] Cookie Editor Qt Widgets shell.
- [ ] Renderer abstraction.
- [ ] DirectX 11 renderer module skeleton.
- [ ] Jolt physics integration.
- [ ] Asset package format and `.pak` tooling.
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
- [ ] Confirm `logs/latest.log` contains startup lines for project root and config file paths.
- [x] Confirm no renderer, physics, editor UI, asset packer, runtime loading modules, OpenGL, save, or mod implementation was added.

## Verification Notes

- `rg --files --hidden` shows the tracked `.gitkeep` placeholders for the requested folder architecture.
- `vcpkg.json` parses successfully and reports `dependencies=0`.
- `cmake --preset x64-debug` was attempted in this terminal, but `cl.exe` is not on `PATH` here, so compiler detection failed before generation.
- The leftover empty `Cookie-Engine/` directory was removed.
- Phase 1 skeleton code was added without introducing renderer/editor/physics implementations.

## Best Next Step

Start Phase 2 by defining API-neutral renderer interfaces in `engine/Renderer` and creating a DX11 module skeleton in `modules/RendererDX11` without implementing full rendering yet.

## Suggested Commit Message

```text
feat: add core/platform/runtime phase-1 skeleton
```

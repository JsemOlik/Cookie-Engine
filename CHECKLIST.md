# Cookie Engine Checklist

## Current Phase: Phase 0 - Repository Foundation

Status: completed

## Completed

- [x] Analyzed the default CMake scaffold.
- [x] Decided to replace the hello-world `Cookie-Engine/` target with a clean architecture.
- [x] Added `PLAN.md`.
- [x] Added `CHECKLIST.md`.
- [x] Added `vcpkg.json` with no early dependencies.
- [x] Added initial repository architecture folders with tracked placeholders.
- [x] Removed the default hello-world source target.
- [x] Updated the root CMake project to represent the architecture-only phase.

## Not Started

- [ ] Runtime executable skeleton.
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
- [ ] Configure CMake with the existing Windows preset from a shell where `cl.exe` is available.
- [x] Confirm no renderer, physics, editor UI, asset packer, runtime loading, OpenGL, save, or mod implementation was added.

## Verification Notes

- `rg --files --hidden` shows the tracked `.gitkeep` placeholders for the requested folder architecture.
- `vcpkg.json` parses successfully and reports `dependencies=0`.
- `cmake --preset x64-debug` was attempted, but this shell does not have `cl.exe` on `PATH`, so compiler detection failed before generation.

## Best Next Step

Start Phase 1 by adding a minimal `CookieRuntime` target, core startup/shutdown interfaces, logging contract, config path discovery shape, and platform abstraction placeholders.

## Suggested Commit Message

```text
chore: initialize Cookie Engine repository architecture
```

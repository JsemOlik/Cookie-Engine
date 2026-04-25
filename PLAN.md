# Cookie Engine Plan

## Engine Overview

Cookie Engine is a custom C++ 3D game engine built with CMake. The first production renderer backend is DirectX 11, targeting Windows first. OpenGL is planned for a future phase, but it must not be implemented until explicitly requested.

The engine is organized around clean module boundaries. Core engine systems own shared abstractions and orchestration. Platform, rendering, physics, audio, asset, and game-code systems are separated so exported games can ship as a structured folder with executable, DLL modules, configs, logs, and asset packages.

## Runtime And Editor Split

Cookie Engine has two main products:

- Cookie Engine Runtime: the executable that ships with exported games. It loads config files, engine modules, game code modules, and `.pak` asset packages, then runs the final game.
- Cookie Editor: the developer-facing editor built with Qt Widgets. It will eventually provide a scene viewport, game viewport, inspector, hierarchy, asset browser, console/log panel, and packaging/export tools.

The runtime and editor may share engine modules, but they should remain separate applications. Editor-only code must not become a runtime dependency unless the runtime genuinely needs it.

## Exported Game Folder Architecture

Exported games should use a structured layout instead of one giant executable:

```text
MyGame/
|-- MyGame.exe
|-- bin/
|   |-- Core.dll
|   |-- RendererDX11.dll
|   |-- Physics.dll
|   |-- Audio.dll
|   `-- GameLogic.dll
|-- content/
|   |-- base.pak
|   |-- maps.pak
|   |-- audio.pak
|   |-- shaders_dx11.pak
|   `-- patch_001.pak
|-- config/
|   |-- engine.json
|   |-- graphics.json
|   |-- input.cfg
|   `-- game.json
|-- saves/
|-- logs/
|   `-- latest.log
`-- mods/
```

`RendererOpenGL.dll` is a future backend and should not be shipped or implemented in the DirectX 11 phase. `saves/` and `mods/` are future folders and should not receive implementation until their phases.

## Repository Folder Architecture

Initial repository layout:

```text
apps/
|-- CookieRuntime/
`-- CookieEditor/
engine/
|-- Core/
|-- Platform/
|-- Assets/
`-- Renderer/
modules/
|-- RendererDX11/
|-- Physics/
`-- Audio/
game/
tools/
content/
config/
docs/
tests/
third_party/
```

Responsibilities:

- `apps/CookieRuntime`: runtime executable entry point and bootstrapping.
- `apps/CookieEditor`: Qt Widgets editor executable entry point and editor-only UI code.
- `engine/Core`: shared engine lifecycle, logging contracts, module interfaces, config abstractions, math/types that are not backend-specific, and common utilities.
- `engine/Platform`: OS-windowing, filesystem paths, dynamic library loading, timing, and platform-specific services behind stable interfaces.
- `engine/Assets`: asset database, asset references, asset loading interfaces, and `.pak` mounting abstractions.
- `engine/Renderer`: renderer-facing interfaces and API-neutral data types.
- `modules/RendererDX11`: DirectX 11 renderer backend implementation only.
- `modules/Physics`: physics integration module, using Jolt when that phase begins.
- `modules/Audio`: audio integration module, implementation chosen in a later phase.
- `game`: sample or test game logic module when game-code loading begins.
- `tools`: build-time and developer tools such as the future asset packer.
- `content` and `config`: development-time sample assets and config files.
- `docs`: extra design notes beyond the main plan.
- `tests`: automated tests and test fixtures.
- `third_party`: reserved for small vendored code only when vcpkg is not appropriate.

## Renderer Backend Architecture

Core engine systems must not include DirectX 11 types. Rendering should be accessed through API-neutral interfaces in `engine/Renderer`. The DirectX 11 backend lives in `modules/RendererDX11` and adapts those interfaces to D3D11.

The backend boundary should make these future changes possible:

- Loading a renderer module by name from config.
- Keeping backend-specific shader packages separate, such as `shaders_dx11.pak`.
- Adding OpenGL later as a separate module without rewriting core engine systems.
- Sharing editor and runtime viewport rendering through the same renderer abstraction.

OpenGL should be documented as a future backend only. No OpenGL source folder, target, or dependency should be added until requested.

## Asset Packaging With .pak Files

`.pak` files are for assets and data only. Game code must remain separate and eventually ship as a module such as `GameLogic.dll`.

The asset system should eventually support mounting multiple packages, resolving assets through stable IDs or paths, and loading patch packages after base packages. Package contents may include meshes, textures, materials, scenes, audio, scripts/data, and backend-specific shader packages.

The asset packer is a future tool. This phase only reserves repository space and documents the intended package role.

## Config Files

Runtime configuration should be stored in the exported game's `config/` folder. Planned files include:

- `engine.json`: engine startup, module loading, logging, and general runtime settings.
- `graphics.json`: renderer backend selection, display mode, resolution, vsync, and quality settings.
- `input.cfg`: input mappings and control settings.
- `game.json`: project-specific metadata and startup scene information.

Config loading should be owned by runtime/core systems, not renderer backends. Backend-specific settings can be represented in API-neutral config structures and interpreted by the selected backend.

## Dependency Management

Dependencies are managed through vcpkg manifest mode using `vcpkg.json`. Dependencies should be added only when the current implementation phase actually needs them.

Initial phase dependencies: none.

Planned later dependencies:

- Qt Widgets for Cookie Editor.
- Jolt Physics for the physics module.
- Any audio, image, model, or packaging libraries only when those systems are being implemented.

DirectX 11 is provided by the Windows SDK and should remain isolated to the DX11 renderer module.

## Windows And macOS Strategy

Windows is the main target while DirectX 11 is the only renderer. The architecture should still avoid unnecessary Windows-only assumptions in core systems so macOS development can become possible after a portable renderer backend is added.

Platform-specific code should live behind `engine/Platform` interfaces. Windows implementations may be created first. macOS implementations should wait until there is a supported renderer backend for macOS.

## Coding Standards And Module Boundaries

- Use modern C++20 with clear ownership and RAII.
- Keep public interfaces small and API-neutral.
- Do not expose DirectX 11 headers or types from core engine systems.
- Keep runtime, editor, tools, engine modules, and game code separate.
- Prefer explicit module boundaries over global state.
- Keep `.pak` assets/data separate from game code modules.
- Add comments only when they clarify non-obvious behavior.
- Keep CMake targets focused and named by product or module.
- Add dependencies deliberately through `vcpkg.json` when the phase needs them.

## Development Phases And Milestones

### [x] Phase 0: Repository Foundation

Create planning docs, checklist, vcpkg manifest, and the initial folder architecture. Remove the default hello-world CMake scaffold.

Milestone: CMake configures, `vcpkg.json` is valid, and the repository structure is ready for real targets.

### [x] Phase 1: Core And Runtime Skeleton

Add initial core interfaces, logging, config path discovery, platform abstractions, module loading shape, and a runtime executable skeleton.

Milestone: `CookieRuntime` starts, writes a log, reads placeholder config paths, and exits cleanly.

### [x] Phase 2: Renderer Abstraction And DX11 Module Skeleton

Add API-neutral renderer interfaces and a DX11 renderer module skeleton without building full rendering features.

Milestone: runtime/editor test harness can choose the DX11 backend by config and initialize/shutdown it cleanly on Windows.

### [x] Phase 3: Windowing And Basic Viewport Rendering

Add platform window creation and a minimal DX11 render loop.

Milestone: a window opens and clears to a color through the renderer interface.

### [x] Phase 4: Asset System And .pak Tooling

Create the asset interfaces and first asset packer/mounter implementation.

Milestone: runtime mounts a `.pak`, lists known assets, and loads a simple test asset.

### [x] Phase 5: Jolt Physics Module

Add Jolt through vcpkg and implement the first physics module boundary.

Milestone: a simple simulation step runs through the physics abstraction.

### [x] Phase 6: Cookie Editor Skeleton

Add Qt Widgets through vcpkg and create the editor shell.

Milestone: editor opens with dockable viewport, hierarchy, inspector, asset browser, and log panels as placeholders.

### [x] Phase 7: Game Logic Module

Add a separate game-code module shape.

Milestone: runtime loads `GameLogic.dll`, calls a defined startup/update/shutdown contract, and keeps assets in `.pak` files.

### [x] Phase 8: Packaging And Export

Add export tooling for the structured game folder layout.

Milestone: editor or tool exports a runnable game folder with executable, modules, content packages, configs, and logs folder.

### [x] Phase 9: Runtime Module Loader Skeleton

Add manifest-driven module path configuration and migrate the first runtime subsystem (renderer) toward a loadable DLL boundary with a safe fallback.

Milestone: runtime can read renderer module path from `engine.json`, load `RendererDX11.dll` through exported factory symbols, and still fall back to static wiring if DLL loading fails.

### [x] Phase 10: Physics Module Loader Skeleton

Migrate physics runtime creation to the same manifest-driven DLL pattern as renderer, with a static fallback during transition.

Milestone: runtime can read physics module path from `engine.json`, load `Physics.dll` through exported factory symbols, and still fall back to static `CreateJoltPhysicsBackend()` when DLL loading fails.

### [x] Phase 11: Audio Module Loader Skeleton

Migrate audio runtime creation to the same manifest-driven DLL pattern as renderer and physics, with a static null-audio fallback during transition.

Milestone: runtime can read audio module path from `engine.json`, load `Audio.dll` through exported factory symbols, and still fall back to static `CreateNullAudioBackend()` when DLL loading fails.

### [x] Phase 12: Core Module Boundary Skeleton

Add a `Core.dll` module artifact with exported identity/version symbols, and probe it from runtime startup using manifest-driven path resolution with static fallback logging.

Milestone: runtime reports `Core runtime source` as `module` when `Core.dll` is available and falls back cleanly when unavailable, while preserving current static-link behavior.

### [x] Phase 13: Optional Strict Module Mode

Add config-driven strict mode controls that can require module loading for core, renderer, physics, and audio at runtime.

Milestone: runtime keeps fallback behavior by default, but can fail fast with clear log errors when required modules are unavailable.

### [x] Phase 14: Runtime Config Profiles

Add explicit development and release engine config profiles, runtime argument-based profile/config selection, and export-time profile application.

Milestone: runtime can load `--engine-config` overrides, and export tooling can produce a shipped `config/engine.json` from `engine.release.json` while preserving a dev-friendly profile in source configs.

### [x] Phase 15: Ship Candidate Automation

Add a release-oriented one-command build/export/validation flow that enforces strict module behavior and exported folder integrity.

Milestone: a single script can rebuild in release mode, export with release profile, validate required modules/config/content, and run exported strict-mode startup checks.

### [x] Phase 16: DX11 Real Clear/Present Bootstrap

Replace the DX11 placeholder renderer behavior with a real D3D11 startup path (device, swap chain, render-target view) behind existing API-neutral renderer interfaces.

Milestone: runtime window is cleared/presented by DX11 backend each frame, with no DirectX types leaked into core engine interfaces.

### [x] Phase 17: DX11 Minimal Triangle Pipeline

Build the first end-to-end draw path in the DX11 backend: create shaders, input layout, vertex buffer, and issue a basic triangle draw call each frame.

Milestone: runtime displays a colored test triangle over the clear color using the renderer module path and existing runtime/editor boundaries.

### [x] Phase 18: Engine-Driven Scene Submission

Introduce API-neutral scene submission contracts in renderer interfaces and drive DX11 drawing from runtime-provided scene data (camera + mesh instances), instead of backend hardcoded triangle data.

Milestone: runtime submits scene/camera/instance data each frame through renderer abstraction, and DX11 renders that submitted scene with per-instance transforms.

### [x] Phase 19: Renderer Scene Builder Utilities

Add reusable renderer-side utilities for scene composition (transform helpers, primitive mesh generation, and scene builder assembly) so runtime/editor paths can share render-scene construction logic.

Milestone: runtime triangle scene is built through `engine/Renderer` scene-builder APIs rather than inline hardcoded scene assembly in `Application`.

### [x] Phase 20: Camera Projection Utilities

Add reusable renderer-side camera/projection matrix helpers and use them in runtime scene submission to validate world-space layout through camera transforms.

Milestone: runtime scene uses renderer projection utilities (orthographic camera) and renders multiple world-space instances with expected placement/animation.

### [x] Phase 21: Config-Driven Camera Mode Selection

Expose camera mode/settings in `graphics.json` and propagate them through renderer config into runtime scene projection selection.

Milestone: runtime can switch between orthographic and perspective projection via config only, without code edits.

### [x] Phase 22: View Matrix And Orbit Camera Skeleton

Add renderer-side view matrix helper (`LookAt`) and use it in runtime camera update flow with a simple orbit behavior around the scene target.

Milestone: runtime composes `view * projection` from config-driven camera settings, supports orbit on/off, and validates camera motion independent of object transforms.

### [ ] Future Phases

OpenGL, save support, mod support, in-game UI/HUD, advanced editor workflows, scripting, and platform expansion are future phases and should be planned separately before implementation.

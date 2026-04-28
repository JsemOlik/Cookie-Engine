# Cookie Engine Asset System v0

## Purpose

Define the first stable asset contract used by editor/dev workflows and runtime cooking. This locks identity, references, and cook outputs before deeper editor features are built.

## Scope

- Editor/dev uses Unity-style sidecar `.meta` files for imported/source assets.
- Engine authores references by `AssetId` (GUID), not raw disk path.
- Runtime loads cooked data from `.pak` files only.
- Scene system supports nested scenes and component-based GameObjects.
- Prefabs are explicitly out of scope for v0.

## Asset Identity

### AssetId format

- `AssetId` is a 32-character lowercase hexadecimal GUID string (no braces, no dashes).
- Example: `7f3a9d1c2e9b4a7f8d1234567890abcd`

### Lifecycle rules

- Creation:
  - On first import/discovery of a source asset, generate a new `AssetId` and write sidecar `.meta`.
- Move/rename:
  - Keep the same `AssetId` when file path changes.
- Duplication:
  - Duplicating an asset creates a new `.meta` with a new `AssetId`.
- Delete/recreate:
  - Deleting the source and later recreating a file at the same path creates a new `AssetId` unless the old `.meta` is intentionally restored.
- Merge/conflict:
  - `AssetId` uniqueness is authoritative. Conflicting IDs must be treated as an error and resolved by regenerating one side.

## Sidecar Meta Schema (`*.meta`)

Editor/dev source example:

```text
Assets/
  Textures/wood.png
  Textures/wood.png.meta
  Materials/Wood.cookieasset
  Materials/Wood.cookieasset.meta
```

v0 schema fields:

- `asset_id` (or `guid`): stable `AssetId` string.
- `type`: logical asset type (`Texture2D`, `Material`, `Scene`, `Mesh`, ...).
- `importer`:
  - `name`: importer identity (`texture_importer`, `mesh_importer`, ...).
  - `version`: importer schema version for reimport/cook invalidation.
  - `settings`: type-specific import settings (compression, mipmaps, sRGB, etc.).
- `source_fingerprint`: hash/fingerprint of source + relevant importer settings.
- `dependencies`: list of `AssetId` values referenced by this asset.
- `labels`: optional tags for editor organization/search.

Notes:

- Keep schema text-based for v0 (`json` preferred for parser simplicity in current codebase).
- Unknown fields should be preserved when possible to allow forward-compatible tooling.

## Reference Rule

- Engine-authored assets and scenes must store references by `AssetId`.
- Raw paths are allowed only in importer/editor transitional tooling, not in authoritative runtime asset references.
- Runtime lookups resolve `AssetId` through cooked registry data.

## Scene v0 Direction

- Scene assets are first-class assets with their own `AssetId`.
- Nested scenes are supported by scene references (`AssetId` list).
- GameObjects use component-based data blobs (Transform, MeshRenderer, Rigidbody, Collider, Script, etc.).
- Scene-to-scene references are by `AssetId`, not source path.
- Prefabs are not part of v0.

## Cooked Runtime Outputs

- Runtime reads from `.pak` files only.
- Source/editor data stays separate from cooked/runtime data.
- Cook step generates a cooked asset registry used by runtime:
  - `AssetId -> cooked payload location`
  - `AssetId -> type`
  - `AssetId -> dependency list`
  - optional version/hash fields for validation

Suggested v0 packaged shape:

- `content/base.pak` contains cooked payload blobs.
- `content/asset_registry.pak` (or equivalent entry inside `base.pak`) contains cooked registry table.

## Migration From Current State

Current baseline:

- `demo_albedo_asset_id` exists as a config-level bridge.
- Runtime still has a hardcoded mesh source path for `content/models/test_mesh.glb`.

Migration steps:

1. Introduce `AssetId` utilities and meta read/write support in `engine/Assets`.
2. Convert texture and mesh references from path-based runtime constants to asset references resolved through registry.
3. Introduce first `Material.cookieasset` using `AssetId` texture references.
4. Introduce first `Scene.cookieasset` referencing mesh/material/nested scenes by `AssetId`.
5. Add cook step that emits runtime registry + cooked payload mapping into `.pak`.
6. Remove remaining hardcoded runtime source paths from app bootstrap.

## Thin Implementation Milestones

### Milestone A: Contract + parsing primitives

- Add `AssetId` validation/normalization helpers.
- Add v0 `.meta` parser/writer for required fields.

### Milestone B: First authoritative references

- Add material asset format that references textures by `AssetId`.
- Load material by `AssetId` in runtime path (no raw texture path authority).

### Milestone C: Scene v0 bootstrap

- Add minimal scene asset with:
  - GameObjects + Transform
  - MeshRenderer references
  - nested scene references by `AssetId`

### Milestone D: Cooked registry handoff

- Emit cooked registry mapping (`AssetId -> cooked payload/type/deps`) into shipped content.
- Runtime resolves through cooked registry from `.pak` data only.

### Milestone E: Ship parity gate

- Add ship validation checks for cooked registry presence and required referenced assets.
- Ensure dev runtime and shipped runtime render identical scene/material output for the same startup scene.

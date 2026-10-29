# GOD model data boundary

Historical prefix checkpoint. See [complete root trees](GOD_MODEL_TREE_PROGRESS.md)
for the subsequent full-data reader and GLES diagnostic.

The native model reader can now load and validate the root geometry, material
references, vertex weights, bind matrices and initial animation keys in the
original GOD assets. This is preparation for the shell's buildings and units.
It does not yet create a complete MeshRoot or render/animate these models.
The released v0.3 APK and its working folder import/audio remain unchanged.

## Source and format

`read_god_mesh_prefix` uses the original BlockFile container implementation and
a new bounded, little-endian compatibility reader for the serialized root prefix.
It follows `MeshRoot::Load` through the initial `states` keys in the pinned
upstream snapshot `9ce9da5f21604310a997f0c41e9cd383f5e292c3`. It is not a staged
copy of the original MeshRoot implementation. CMake verifies the reviewed source
hashes in `cmake/god-format-source-hashes.json` before building the reader.

Format references are `graphics/meshload.cpp`, `godfile.cpp/h`, `material.cpp`,
`bitmap_manager.cpp`, `mesh.h`, `Vertex.h`, `family.h`, and the math/type headers.
Original implementation code and game assets remain outside this repository.

Supported logical versions are 1, 2, 3 and 5–12, matching original dispatch.
The block named **MeshRoot 4.0 means logical version 1**; there is no logical v4
branch. Legacy Sphere bounds include a full 64-byte Matrix; newer Bounds use
seven floats. Shadow fields, material flags, texture markers, and animation
scale fields vary by version. The reader consumes these differences explicitly.

The returned snapshot contains:

- Original vertices, normals, UVs, colors, and faces with independent indices.
- Raw bucket flags, optional material fields and texture names/type/mip counts.
- Vertex influences, retaining the original first-two-weight policy without
  renormalizing; serialized extra influences are consumed safely.
- Planes, bind matrices and initial animation keys.
- The exact prefix byte count and total root-block size.

Raw values do not imply runtime rendering policy. In particular, the original
material manager suppresses some serialized material properties, and runtime
bitmap settings override stored mip counts. No material manager or GPU object
is initialized by this reader. Other root header fields are consumed to reach
the geometry but are not yet exposed as runtime model settings.

## Safety boundary

Unaligned host pointer casts are replaced by explicit byte reads; 32-bit floats
are reconstructed with memcpy and must be finite. Reads are bounded before
allocation, with a 64 MiB block cap, 65,535 geometry elements, 16 buckets and
64 states. Names must be bounded and NUL-terminated. Face bucket/vertex/normal/UV
indices, retained bone indices and applicable color/weight counts are checked.

This is intentionally a **prefix reader**, not whole-file validation. Animation
cycles, legacy environment data, MRM updates, hierarchy nodes, chunks and root
end markers follow the reported boundary and remain opaque. A file truncated
inside that later tail can still yield a valid prefix. Callers must not treat
this API's success as permission to animate or render a complete model.

## Validation

- `god_mesh_smoke`: 6,516 assertions across all 11 supported logical versions.
  Covers fields, two-weight behavior, every fixture prefix truncation, malformed
  names/counts, nonfinite positions, invalid face/bone references, unknown version,
  and the explicit opaque-tail contract.
- Real supplied assets: all 338 GOD files in `packs/base.zwp` and
  `worlds/sprawl.zwp` pass: 24,883 vertices and 40,927 faces.
- Observed versions: v1=50, v2=19, v3=19, v6=20, v7=47, v9=60,
  v10=57, v11=12, v12=54. v5/v8 are fixture-tested, not present in these packs.
- `god_mesh.cpp` cross-compiles to an AArch64 ELF object with NDK r27c/API 26.
  CI also builds `reign_god_mesh` with the other native shell components.
- All 62 host tests pass under ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`).
  Two cached test binaries needed mode 755 restored after reporting permission
  denied; the full suite then passed. Results are saved alongside the probe logs.
  This is not an on-device model-rendering test.

```sh
build/god_mesh_probe '/path/to/Dark Reign 2/packs/base.zwp' \
  '/path/to/Dark Reign 2/worlds/sprawl.zwp'
```

The probe requires the user's supplied assets and writes only metadata. Logs
live in `docs/logs/god-mesh/`; no models, textures or original game data are
included. The next boundary is the remaining animation/hierarchy/MRM/chunk data,
then original node transforms/skinning and material draw policy feeding the
existing homogeneous GLES path. The complete native shell still also needs
lighting, environment/effects and cinematic integration.

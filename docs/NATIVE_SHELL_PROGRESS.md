# Native shell camera and terrain checkpoint

Subsequent projection/GLES integration is documented in [WORLD_RENDER_PROGRESS.md](WORLD_RENDER_PROGRESS.md).

The target is the original real-time 3D menu scene from the game's data, with
its original controls, music, effects and camera/cinematic behavior. A recorded
background is not part of this port. v0.3 remains the released APK and still
uses its static backdrop. The owner confirmed its folder import and audio on
Samsung SM-F946U/API 36/Adreno 740.

## Original camera runtime

The hash-checked source staging now includes BookmarkObj::CurveSrc::AddNode,
Curve::Build and Curve::Step plus the original NList implementation. The
arithmetic and list/loop stepping bodies are unchanged, in an isolated namespace.
Only the relevant original declarations are staged, outside this repository;
GameObj ownership and the viewer/cinematic scheduler are not claimed ported.

ShellCamera owns the lists, reads the original parser's configuration snapshot,
validates segment inputs and returns a position and normalized forward vector.
Movement and focus share time in the original BookmarkObj::Step order. A
non-looping path returns inactive at its end; the future viewer must preserve
its last valid pose rather than snapping to the zero-initialized inactive result.
Looping paths use the original modulo behavior. Main::Init is a valid single-node
loop; it must not be rejected by a generic two-node minimum.

Real shell validation: 18 bookmarks, 2,160 interior samples, plus completion
checks. Main::Enter lasts 6 seconds; menu event timing remains separate from
curve duration. Synthetic checks exercise linear interpolation, wrapping,
completion, focus, single-node loops, invalid durations and invalid sample times.

## Original terrain data boundary

`prepare_shell_terrain.py` stages the original Terrain::Load body from the pinned
source. The data-only adapter supplies bounded block reads, owned arrays and
captured rendering variables. The original cell and environment assignments
are retained. Terrain::Recalc and SetupPerf are explicitly excluded because
their renderer/LOD/material services are not yet ported.

The adapter supports the supplied v17 format only, checks counts before fixed
arrays, checks texture-name length/termination, checks cell indices and finite
values, bounds dimensions, and rejects truncated/trailing data. Terrain bytes
are retrieved through the already-ported original BlockFile implementation.
No original implementation files or game assets are committed to Git.

The snapshot preserves serialized texture/overlay table identities. Bitmap
allocation/deduplication, overlay GPU resources and material residency checks
belong to the future renderer. The adapter allows all 256 serialized overlay
texture slots; that is not a claim that those textures have been resolved.
Only serialized height rows are exposed: original HeightField::Setup's extra
bottom border and terrain Recalc/LOD setup still need their rendering lifecycle.

Actual shell results:

- Map: 60×60, pitch 61, 3,660 stored height samples; heights -10 through 30.
- 73 normals, 225 clusters, seven overlay descriptions.
- 20 terrain textures decoded by the original Bitmap manager: 19 Sprawl PICs
  at 128×128 and engine_terrain_grid.tga at 64×64.
- Camera and terrain probes operate on the supplied original assets.
- Terrain probe also succeeds with the Android import directory layout.

## Folder import

The importer now copies available `worlds/sprawl.zwp`, `sides/client/jda.zwp`
and `sides/client/sprawler.zwp` into the same paths under its private snapshot.
They are validated and included in the existing transactional storage limits;
corruption cancels the entire replacement and keeps the previous data.
Missing optional packs keep compact menu-only installations usable. Diagnostics
report the number of scene packs imported without claiming world rendering.

The previously delivered compact mobile menu ZIP lacks these three archives.
A future native-scene APK will need the expanded data set or an import of the
full installation. The complete installation is already available in recovery
storage; no new desktop files or recording are needed from the owner.

## Validation

- 60/60 host tests under ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`), including
  19 camera checks and 461 terrain checks covering every truncated fixture.
- Four older cached host binaries lacked executable bits; these were restored
  and the complete test suite rerun successfully.
- 34 Java importer checks using the full installation, including exact byte
  comparison of all three imported scene packs and optional-pack corruption.
- Android Java compilation against API 35 passes.
- Original spline, bookmark curves, camera wrapper, terrain loader and terrain
  wrapper each compile into AArch64 ELF objects with NDK r27c/API 26.
- CI builds both native component libraries for ARM64. Its menu artifact path
  is corrected to the existing v0.3 package name.
- No on-device world-rendering test or new APK release is claimed.

Logs are in `docs/logs/native-shell/`.

## Next integration boundary

Restore the original terrain world-space projection/clipping, cluster drawing,
LOD/border setup, texture/overlay and water passes behind the existing GLES
boundary. Load the scene's GOD meshes, transformations, animation and lighting.
Then bind the original viewer and objective/cinematic events to menu transitions
and Android frame/lifecycle scheduling. The original UI and audio remain in
place while these scene services are connected. Do not substitute a generated
scene, video loop or invented camera orbit for the original shell.

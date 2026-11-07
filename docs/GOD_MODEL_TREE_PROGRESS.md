> Historical checkpoint. See [model poses and shell scene validation](GOD_POSE_PROGRESS.md) for the current boundary.

# Complete GOD root trees and model GLES diagnostic

The model path now reads the remainder of the original serialized MeshRoot:
animation cycles/tracks, MRM updates, node hierarchy and nested chunks. The
Sprawler HQ and JDA Guardian root geometry also render with their original base
textures through the homogeneous GLES path. This is still an isolated diagnostic,
not the animated original shell or a new APK. The working v0.3 release is unchanged.

## Data boundary

`read_god_mesh` builds on the bounded prefix reader and consumes a complete root
block, verifying node/root end markers and rejecting trailing root data. It
supports the same original version dispatch (including logical v1 under the
MeshRoot 4.0 block name). Original BlockFile remains the container implementation;
the field reader is new compatibility code checked against pinned original
meshload/godfile/material/bitmap sources. The source manifest now includes
`graphics/anim.h` and `graphics/meshmrm.h`.

The snapshot retains:

- Cycle names, timing/bounds fields, initial keys, per-state animation tracks and
  serialized key values. Legacy meters-per-frame converts to frames-per-meter.
- Flat hierarchy nodes with explicit parent indices, object matrices and local
  geometry/groups. Child and legacy sibling markers preserve parent relationships.
- MRM vertex additions and face replacement tokens/indices, checked against the
  original geometry arrays and declared update totals.
- Nested model chunks, their indices and exact serialized byte spans.

Root/chunk recursion is capped at 32 levels and 1,024 roots per block; node
recursion at 128 levels and 256 nodes per root. Counts are bounded before reads
and allocations. Node and animation state references must address root states.
The legacy `do_airport45.god` local group contains state ID 2 with only two root
states. Unlike actual node/track references, these local group IDs are retained
as bounded metadata and are **not** used to index bone arrays. Their historical
origin has not been established; no runtime interpretation is inferred.

This is a serialized snapshot, not the original runtime SetupRoot procedure.
Original end-key normalization, quaternion sign adjustment, root-state reset,
state/world matrix evaluation, skinning and LOD policy are not applied. Local
geometry/group data are preserved, not made render-ready. The earlier
`read_god_mesh_prefix` API keeps its explicit prefix-only contract.

## Real assets

All 338 base/Sprawl GOD files pass the full-root reader. Root geometry totals
remain 24,883 vertices and 40,927 faces. Including nested chunks, the reader
loads 1,794 nodes, 234 cycles, 1,432 tracks, 43,535 animation keys, 111 chunks and
223 MRM sections. These totals describe decoded data, not running animations.

## GLES diagnostic

`god_mesh_gpu_probe INSTALLATION_ROOT MODEL.god OUTPUT.ppm` submits serialized
root vertices with independent face UV indices and the original base textures
through WorldProjection and LegacyGles. It frames the model using calculated
bounds and an explicit diagnostic camera. Two fresh EGL contexts must produce
identical nonempty frames.

Observed results:

| Model | Triangles | Nonblack pixels in each context |
| --- | ---: | ---: |
| spr_hq.god | 129 | 21,805 |
| jda_guardian.god | 394 | 8,883 |

Both internal snapshots were visually inspected. The guardian's body/equipment
and the HQ geometry/textures are visible. No original screenshots, textures or
models are committed. This test omits node transforms, animation, lighting,
overlays, team color, original material/blend policy, shadows and scene placement.
It does not imply that every serialized model can render correctly without those
services. The production Android menu does not invoke this diagnostic.

## Verification

- Expanded synthetic checks: 32,717 assertions, covering complete roots across
  all 11 supported logical versions, every fixture tail truncation, child/sibling
  parenting, nested chunks, invalid track/node states, MRM tokens, end markers,
  trailing bytes and excessive chunk nesting. Existing prefix coverage remains.
- Full suite: 62 host tests under ASan/UBSan with leak detection disabled.
- `god_mesh.cpp` cross-compiles with NDK r27c/API 26 to AArch64 ELF.
- Real-file census and two-model GLES context-recreation checks pass.
- One cached frontend-quad executable required mode 755 restored after reporting
  permission denied; this was an environment issue, not a product-code fix.

Logs are in `docs/logs/god-model-tree/`. The next integration is original pose
and skinning behavior, shell object placement and animation timing. Lighting,
water/effects and the original cinematic runtime remain necessary for the complete
native moving menu.

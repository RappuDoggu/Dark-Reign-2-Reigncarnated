# Original model poses and shell scene

Checkpoint: 2026-10-28, branch `dev/native-arm64-primitives`.
The released v0.3 APK is unchanged. This checkpoint is host scene integration
and Android compile validation, not a completed animated-menu APK.

## Implementation

`prepare_mesh_pose.py` verifies eleven pinned upstream files against
`cmake/mesh-pose-source-hashes.json` and stages reviewed arithmetic outside the
repository. It retains original scalar Matrix construction/multiplication,
Quaternion conversion, Animation::SetFrame/Set and MeshRoot::SetVert bodies.
The adapter supplies checked containers and portable types; source hashes fail
closed if the reviewed upstream changes.

`GodPoseEvaluator` owns its model data, validates hierarchy, indices, finite
values and track ordering, and samples an explicit cycle/frame from initial
cycle state. It uses original row-vector transforms and inverse-bind matrices,
control-node initialization, and one/two-weight skinning. It reproduces the
original loader's redundant quaternion end-key removal and sign normalization
branch. Missing key intervals retain initial state; no hold-last behavior is
invented. This does not implement the stateful MeshEnt animation clock,
transitions, overlays or aiming.

The shell probe resolves original type configuration includes through PTree,
loads both object and world types, and reads saved MapObj transforms. All 95
placements share 28 model definitions. Four HQ child objects resolve their
parent IDs and named attachment nodes before composing their transforms.
The probe samples default cycles using an explicit modulo frame mapping; this
is diagnostic timing, not the original cinematic scheduler or object simulation.

The mesh probe accepts optional `cycle frame` arguments, allowing a fixed-camera
comparison of sampled animation through GLES. Existing static invocation remains
supported.

## Fresh verification

- Sanitized host build: all 63 CTest tests pass; the new pose test has 31 checks.
- Original asset scan: 338 top-level models, 449 roots including chunks,
  234 cycles, 936 additional frame samples, 179 cycles with moving vertices,
  and 28,179 vertices. No pose-validation failures.
- Shell scene: 7,200 terrain and 8,499 model triangles per frame. Samples at
  0, 3 and 5.5 seconds contain 219,589 / 246,988 / 225,246 nonblack pixels.
  Each sample is byte-identical after EGL context recreation.
- Guardian MOVE frames 0 and 5: 7,710 and 7,771 nonblack pixels; 11,043 pixels
  differ. Both poses reproduce after context recreation. Visual inspection
  confirms coherent model geometry and changing limb/rifle pose.
- Android NDK r27c/API 26: evaluator and staged math compile to AArch64 objects.
  This is compile-only evidence, not execution on the Fold 5.

Logs are in [logs/god-pose](logs/god-pose). Host rendering uses software EGL/GLES;
ASan leak detection is disabled because this environment cannot inspect threads.
Address/undefined-behavior instrumentation remains enabled by the build option.

## Reproduce

Configure/build using the README, with GLES and staged Khronos headers enabled.
Then run:

```sh
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build --output-on-failure
ASAN_OPTIONS=detect_leaks=0 build/god_pose_probe /installation/packs/base.zwp /installation/worlds/sprawl.zwp
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 build/shell_world_gpu_probe /installation shell
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 build/god_mesh_gpu_probe /installation jda_guardian.god guardian.ppm MOVE 5
```

## Remaining boundary

Rendering currently uses unlit base textures. Original material passes, blending,
team colors, normal transformation/lighting, terrain overlays, sky/water and
particles remain. LOD/chunk switching and continuous original animation/cinematic
state remain. The original menu controls/audio must be composed with a continuous
Android scene renderer and tested through pause/resume and context loss before
calling the moving-menu APK complete. No gameplay availability is implied.

The prior session ended before its pose work reached GitHub. This checkpoint
reconstructs that work and reruns the verification above; it does not claim that
unsaved historical logs were recovered. Proprietary assets, original source,
generated overlays and rendered snapshots remain outside Git.

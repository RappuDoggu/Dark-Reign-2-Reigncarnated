# World projection and terrain GLES integration

The native renderer can now accept original-style homogeneous world coordinates,
retain geometry on both sides of the camera, and clip it safely before perspective
division. The existing transformed/lit menu path remains available and tested.
This checkpoint does not release a new APK or activate a partial 3D backdrop in
the main menu.

## Original camera math

`prepare_world_math.py` hash-checks and stages the original scalar Matrix
ClearData, Zero, SetFromFront, SetInverse, SetProjection and Transform_Vectors,
plus Vector normalization/dot/cross/magnitude helpers. The C++ transform path
is used; the x86 assembly alternative is excluded. Original arithmetic bodies
remain unchanged and outside the repository.

WorldProjection owns the view/projection matrices, validates the input frustum
and camera direction, and returns homogeneous x/y/z/w without early perspective
division. It applies the screen-y inversion from original Vid::SetProjTransform.
A direction parallel to the initial up vector is explicitly rejected rather than
inventing an alternate camera orientation. The source math tests cover translated
and rotated cameras, handedness, aspect ratio, near/far depth and invalid inputs.

The wrapper's raw w can be zero or negative so GPU clipping remains possible.
This intentionally stops before the original Vid::W epsilon/division path, and
is not an implementation of the original CPU Vid::Clip algorithm.

## GLES boundary

LegacyClip is an explicit 32-byte vertex layout with homogeneous position,
diffuse/specular colors and UVs. LegacyGles::draw_clip shares state/buffers with
the existing TL path while selecting homogeneous coordinates in the vertex
shader. It converts Direct3D clip depth [0,w] to GLES [-w,w], preserves the
logical target's orientation and half-pixel convention, and delegates canonical
frustum clipping to GLES. The existing perspective-correct texture and affine
color/fog conventions are retained. The final logical-target flip is unchanged.

All coordinates and indices are checked before submission. Negative/zero w is
allowed only on the homogeneous path; the original TL API still requires finite,
positive reciprocal w. No reinterpret-cast between the two C++ layouts is used.

GPU checks cover:

- TL/homogeneous pixel parity with varying w, textures and vertex colors.
- Near-plane crossing, geometry entirely behind the camera, and a triangle
  crossing w=0 without screen-filling artifacts.
- Shared world/TL depth and restoration of the menu path after world drawing.
- The original camera matrix pipeline feeding GLES, including screen orientation
  and camera translation.
- Existing UI, bitmap/font, blend/fog, state and context-recreation checks.

## Real-data integration probe

`shell_world_gpu_probe INSTALLATION_ROOT OUTPUT_PREFIX` reads the actual shell
configuration and terrain, loads its 20 textures through the original bitmap
manager and samples Main::Enter at 0, 3 and 5.5 seconds. It uses the original
cluster triangle and cell-UV tables, staged separately from pinned source. The
probe submits 7,200 terrain triangles for each frame. The original setup's
unserialized extra bottom row is initialized to height 10 for this diagnostic.

Two recreated EGL/GLES contexts produce byte-identical corresponding frames;
advancing the camera produces distinct frames. The observed nonblack pixel
counts are 213,978; 196,477; and 156,406. Developer snapshots were visually
inspected for orientation and texture mapping. No scene assets or screenshots
are committed to Git.

This tool deliberately renders **unlit base terrain only**. It is a GPU
integration test, not a port of Terrain::RenderClusterVtl or the complete scene.
It has no models, overlays, water, sky, original lighting, fog policy, shroud,
LOD/culling or cinematic scheduler. Its explicit diagnostic frustum is a 60-degree
vertical FOV with near/far 1/1000; final viewer and performance settings are not
bound. Do not use these diagnostic defaults as the final original shell behavior.

## Validation

- 61/61 host tests pass under ASan/UBSan with ASAN_OPTIONS=detect_leaks=0.
- Original scalar projection checks: 14 assertions.
- GPU tests pass in two fresh software EGL contexts, including the direct
  original-matrix-to-GLES check and all previous UI checks.
- Actual shell terrain integration passes at three camera times in two contexts.
- World projection, staged original matrix source, GLES implementation and
  renderer checks compile to AArch64 ELF objects with NDK r27c/API 26.
- Two older cached test binaries needed their execute bits restored; the full
  suite passed afterward. No product code change was needed for that issue.
- CI now explicitly builds the new ARM64 world-projection library alongside
  the prior shell components. On-device world rendering is not yet tested.

Logs: `docs/logs/world-render/`.

## Remaining native scene work

Use this 3D backend boundary while connecting the original terrain drawing,
material/overlay/water passes and visibility/LOD services. Port GOD mesh loading,
object transforms, model animation and lighting for the shell's buildings and
units. Bind the original viewer, environment and objective/cinematic events to
scene transitions and frame scheduling. Preserve the current original UI and
working music/effects while completing those integrations.

# Original Vid submission boundary — 2026-10-24

The reviewed original `graphics/vidrend.cpp` stage-zero texture setters,
`SetTextureDX` and `DrawIndexedPrimitive` now compile on Linux and Android ARM64.
Host tests execute these original functions through the new numeric D3D7 state
adapter into the existing GLES3 renderer and validate actual GPU readback.
This is an isolated original rendering dependency slice, not engine startup.

## Implementation and provenance

`tools/prepare_vid_slice.py` verifies SHA-256 pins for `graphics/vidrend.cpp` and
`graphics/Vertex.h` before extracting selected implementation and flag definitions
into an external directory. The original checkout stays unchanged. No original
implementation, generated overlay, game texture or compiled engine binary is
committed or published.

The original state-setter bodies remain unchanged except const-correcting their
unused diagnostic blend-name array. `SetTextureDX` takes an original porting
`TextureView` instead of the original Bitmap class and uses integer zero for its
native texture handle. That view only exposes the real GLES texture handle; it
does not replace or claim to initialize the original Bitmap manager. The original
function still binds stage zero, applies its original blend/address flags and
explicitly disables stage one.

The selected original indexed-draw body preserves its topology assertions,
source/destination state application, clipping-state call, DP flag masking,
device submission, return result and index counter. Its Windows debug range/log
block is replaced by unconditional bounded native submission validation. The
original DODXLEANANDGRUMPY build path is retained (software transformed/lit
vertices, no D3D lighting). Cull setup is excluded until winding conventions are
validated; unsupported device state calls throw rather than claim success.

`VidDevice` implements only the inspected state ABI and validates it. It needs an
explicit, synchronous draw sink. There is no successful no-renderer fallback.
Thread-local Vid globals are bound/restored by a noncopyable scope. This is not a
COM/D3D compatibility library or a replacement simulation.

The GLES adapter now additionally supports stage-zero diffuse-RGB/texture-alpha,
texture-only output, saturated 2x/4x RGB modulation, wrap/mirror/clamp addressing,
and independent valid source/destination blend factors. GLES continues to clip
primitives; original clipping flags are recorded, not used to disable hardware
clipping. Unsupported multitexturing, border addressing, W buffering, lighting,
other FVF layouts and non-triangle-list primitives are rejected.

## Verification

- Twelve host CTest targets pass with ASan/UBSan. The new original-Vid target
  verifies previous state values, all seven original texture flag modes, null
  texture unbinding, nested bindings, real draw dispatch and counters, and
  rejection of unsupported stages/states/topology/FVF, absent sink and bad data.
- Real software-EGL readback: the nine baseline checks plus 13 original-Vid
  checks pass in two independently created GLES contexts. The latter submit via
  original `Vid::DrawIndexedPrimitive`, not a direct test call to GLES draw.
  Checks cover all seven texture modes, destination-color/additive framebuffer
  blend, depth test/write, and all three supported texture address modes.
- All targets, including original Vid state code and its smoke executable,
  compile for Android `arm64-v8a`, NDK r27c/API26. These new original-source
  binaries have not been executed on the owner's device.
- The owner's prior v0.3 Fold screenshot remains the actual device evidence for
  the original porting GLES backend and cursor preview. It does not verify this
  new original-source slice or pause/resume.

Raw configure/build/test logs are in `docs/logs/vid-state/`. An initial staging
marker mismatch is retained in `staging-failure.log`; selecting the original
function's closing brace fixed it. ARM64 Clang also reported a mismatched scalar
`delete` for `new char[]` in original BlockFile::ReadBlockAlloc. The external
config overlay now uses `unique_ptr<char[]>`, transfers ownership only on a
successful read and cleans up on false/exception paths. The pre-fix warning and
post-fix build logs are retained. No complete Windows build was available.

## Remaining engine work

1. Bring over original Bitmap lifetime/format conversion and texture upload/cache
   semantics; integrate actual resource streams. The TextureView used here is
   only a bounded test boundary.
2. Port original bucket allocation/sorting/flush and connect it to these original
   Vid calls; inspect 32-bit pointer arithmetic before compiling it.
3. Preserve stage-one terrain/detail/shroud combinations and add sampler/mipmap,
   alpha/fog state and required primitive support with pixel fixtures.
4. Continue original Main/GameGod prerequisites (VarSys, fonts/interface,
   timing/input/audio, camera/terrain/mesh services), then original frontend.

M3–M5 remain unachieved. No main menu or mission APK is delivered by this slice.
Touch/pen and Thor dual-screen ideas remain deferred. Existing v0.3 device test
requires no repeat for this host-only original-code work.

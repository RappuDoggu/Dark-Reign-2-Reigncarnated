# v0.7 performance test

The Fold 5 v0.6 device test confirmed main-menu startup, direct mission loading,
and construction of a command center and solar panel. The user reported poor
performance and a nonfunctional original Instant Action button.

## Changes

- Retain DX7 surface GPU textures across texture changes. The device owns an LRU
  cache of up to 512 surfaces / 64 MiB of RGBA storage (one oversize surface may
  occupy the cache alone). Retained surface references prevent pointer reuse.
  Revision checks still detect writes, including mutations while unbound.
- Reuse GPU texture storage for updated surfaces and menu/intro movie frames.
- Cache shader uniform locations, draw state, and six sampler objects. Enable
  vertex attributes once and adjust offsets per batch. Reuse streaming buffer
  arenas, orphaning at frame boundaries or when full, instead of reallocating
  vertex/index storage on every draw. Validate GLES errors at frame/readback
  boundaries; REIGN_GLES_VALIDATE_DRAWS restores per-draw driver checks.
- Reuse aligned index, topology conversion, culling, and sequential-index arrays.
  Preserve the alignment fix and index/vertex validation.
- Compile all 603 original source objects, mission adapters, menu slices, and
  foundation libraries with -O2, -fno-strict-aliasing and -fwrapv. Keep
  DEVELOPMENT=1 consistently because it affects original class layout. No
  fast-math flags or gameplay timing/balance changes.
- Rebuild both native libraries. Limit their exports to their JNI entry points
  and use symbolic internal binding. Both retain 16 KiB ELF LOAD alignment.
- Avoid the JNI mission loop's second presentation when the original engine has
  already presented through its callback. Keep presentation when loading or a
  simulation frame does not draw.
- Enable the original main-menu Instant Action button and route its existing
  event/command to a native test-map chooser for 2crazed. Full original lobby,
  map options and AI opponent configuration remain pending. Keep the explicit
  test-launch button available.
- Add once-per-second menu/mission FPS and native-call duration overlays. Tap
  the overlay to share the current reading. Mission details include draws/frame,
  texture uploads/second, upload MiB/second, engine time and presentation command
  submission time. These are CPU wall timings, not GPU execution timers. FPS
  uses the interval between GLSurfaceView callbacks and includes swap pacing.
  Remove the mission's per-frame UI visibility runnable after startup.

## Validation

- All 10 host foundation tests pass with ASan/UBSan in foundation/new module
  code. Leak detection is disabled for Mesa. Original 603 objects are optimized
  but are not sanitizer-instrumented.
- Real GLES surface tests across two contexts cover two hundred A/B texture
  switches with only one additional upload, revision refresh while unbound,
  stable texture ID on same-size update, raster state, winding, topology,
  alpha/depth behavior, and perspective versus affine interpolation.
- Original menu/profile lifecycle: 26 checks pass. Original menu GPU rendering,
  animated scene/effects, context recreation, and the real owner's Instant
  Action control producing @instant all pass. Full GLES pixel regression passes.
- Shared mission application loads original 2crazed, selects an original rig,
  issues a primary-click move command, pauses/resumes, shuts down, and reopens
  the same profile successfully. Two lifecycle passes: 76 and 21 simulation
  cycles. Movement displacement: approximately (0.205, 7.607) world units.
- Java compiles against SDK 35. APK signature, ZIP structure, ARM64-only payload,
  zipalign 16 KiB, JNI exports, and native-library byte equality pass. The v0.7
  signer certificate matches v0.6, enabling an in-place candidate update.
- No v0.7 Android device execution was available here. Device speed, audio,
  lifecycle, and the Android chooser UI still need Fold 5 testing.

## Build and recovery

Use tools/build_performance_candidate.py with external pinned upstream source,
SDK 35 / NDK r27c, minimal Android FFmpeg n6.1.1 install, the exact private v05
base APK, and the existing candidate test signing key. It rebuilds the original
units, foundation, menu, mission module and APK. The source/relink archive also
contains native objects and static dependencies for relinking without a full
engine rebuild. Keep the test signing key private and reuse it for candidate
updates. No proprietary game data is included in the APK or source archive.

## Fold 5 test

Install over Reigncarnated IA Test v0.6. Reuse its existing imported v0.6 data;
no new game-data ZIP is required. Let the menu settle, note its FPS, then use
the original Instant Action button or the explicit test button, choose 2crazed,
and Launch. Move the camera and rigs, construct buildings, and note performance
after a few minutes. Tap the bottom-right overlay to share readings. Full match
settings, AI opponents and complete campaign/network navigation are not added.

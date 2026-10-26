# Console dispatch increment — 2026-10-26 UTC

The original console parser/VarSys command dispatch now builds on host. Focused
checks pass (21); the full host suite passes 45/45 with local LeakSanitizer off.
A missing execute bit again prevented one initial test process from starting;
restoring that bit allowed the complete suite to pass. CI remains the independent
clean build check. The full staged-control link has three unresolved services:
IFace::MsgBox, IFace::Sound::Play and movie deactivation. No menu runtime yet.
[Implementation and limits](CONSOLE_DISPATCH.md), [host test log](logs/console-host-tests.log).

The preceding localization/Babel commit b4f2b1e passed both host (44/44 tests)
and ARM64/GLES/package CI: run 40265745495, host job 121387664105,
Android job 121387664003. Console ARM64 verification is pending.

# Localization lookup and Babel increment — 2026-10-26 UTC

Original MultiLanguage lookup and Babel substitution now compile on the restored
host workspace. Native checked formatting preserves UTF-16 menu strings and
variadic integer/float/string arguments. Host ASan/UBSan tests: **44/44 PASS**
with local LeakSanitizer disabled. Focused tests: 20 lookup and 39 Babel checks.
The first run could not execute two files whose execute bits were absent;
restoring those bits allowed the full suite to pass. Both logs are retained.
ARM64 CI for this increment is pending; no new engine/menu APK is claimed.

The eight-object full control link now has four unresolved services:
Console::ProcessCmd, IFace::MsgBox, IFace::Sound::Play and movie deactivation.
This is a staged-control link boundary, not a count of all menu startup work.
See [implementation, compatibility limits and reproduction](LOCALIZATION_LOOKUP.md),
[host tests](logs/localization-babel-host-tests.log), and
[link evidence](logs/localization-babel-full-link.json).

# Current next gate — original menu startup

The next device deliverable remains an APK displaying the original menu. No
replacement diagnostic is requested from the owner at this checkpoint.

1. Port the remaining compiled-control services: localization, command parser,
   actual dialog creation, sound and texture/movie lifecycle.
2. Register and compile the additional widget/game control types required by the
   supplied Login/Shell configurations; construct live original roots and execute
   their configuration/action paths.
3. Connect those roots to native input, font/texture submission, Android lifecycle,
   frame presentation and the original shell-scene requirements.
4. Build and verify the APK, then request a Fold test with actionable diagnostics.
   Touch/stylus refinement and dual-screen support follow basic startup.

The supplied installation has been recovered and its executable hash matches the
prior analysis. Real configuration theme loading succeeds (21 groups, 14 skins,
2 bitmaps), while 183 other statements remain deliberately unexecuted by that
probe. This is not engine initialization or a main-menu runtime result. No reliable
completion percentage follows from the five missing symbols in one UI slice.

Earlier milestone notes below are historical checkpoints.

# Native Android engineering sequence

## Implemented baseline

ARM64-only CMake builds of original CRC/RNG, original PACK header layout validation, original platform timer/file/lifecycle services, read-only pack metadata tooling, and an Android NativeActivity diagnostic shell. The shell links original primitives and GLES3, runs self-tests at onCreate, and clears a green/red surface. It does not call Main::CoreSystemInit, GameGod::Start, load a menu, or simulate a mission.

Provisional platform floor: API 26 (Android 8). The actual dependencies compiled at this API: C++17 filesystem/chrono, NativeActivity, EGL and GLES3. This is a tested build configuration, not a proven minimum; API 21 may be viable after library/API review. No SDL3 dependency is currently introduced. The NDK builds 16 KiB-compatible ELF LOAD alignment and the packaging script checks APK ZIP alignment. No device/page-size runtime test has occurred.

## Next implementation units, in order

1. **Acquire runtime evidence:** run the local diagnostic on a controlled Android ARM64 device after the binary handling decision. Capture `adb logcat -s Reigncarnated`, app launch, pause/resume, rotate/window recreation, process recreation and EGL errors. Check green diagnostic output plus original primitive PASS log. A green screen is not a game scene or M3.
2. **Expand portable system dependency slice:** separate defines/std prelude, establish fixed-width signed/unsigned types and pointer types; port minimal assertions, memory, strings and file services required by PTree/FScope/BlockFile. Compile original config parser with real startup configuration. Add serialization fixtures and preserve exact original configuration/search behavior. Current bounded read_asset helper is not wired into FileSys; it does not implement streams, mapping or archive precedence.
3. **Determinism and utility gate:** replace x86 rotate/FPU/math/clock operations individually; test original data layout, RNG, float rounding, math, saved game data, order/simulation tick behavior. Do not treat CRC/RNG compilation as simulation portability. Port debug allocator pointer arithmetic or isolate that allocator for bring-up; never silently alter serialized record sizes.
4. **FileSys and assets:** preserve .zwp NORK loading and CRC lookup. Use the existing inspector to collect metadata from a user's installation without copying assets to git. Implement app-private copied asset roots or a Storage Access Framework import; do not request broad storage permissions as a default. Current helper expects an existing local root and does not implement SAF. Concurrent hostile filesystem mutation is outside its current threat model.
5. **Video backend:** map Vid/Bitmap state/surfaces to GLES3; implement deterministic fixture textures and geometry, then original terrain/mesh draw paths. Compare Windows capture and output for coordinate handedness, depth range, winding, fixed-function texture stages, fog, alpha testing, blend modes and color packing. Keep Vulkan/translation-layer alternatives open until this boundary is prototyped.
6. **Input and audio:** port Input's event contract to gamepad/touch/pointer events; handle focus loss/held-state reset. Replace Sound internals behind the existing voice/reserved/stream interfaces, preserving priorities, looping, fades and timing. Audit Miles MP3 and Bink formats/rights before codec selection. Do not bundle x86 DLLs.
7. **Engine boot:** wire actual generic initialization and game run codes to Android lifecycle, removing Windows process-launch/display-mode requirements at the boundary. Account for Multiplayer::PreInit and MultiPlayer::Init even for single-player. Reach original main menu, then original mission and victory/defeat/save/load paths. Gather performance and parity traces before optimizing.
8. **Thor after single-screen gameplay:** inspect actual display topology/APIs with a device; test secondary Presentation/native surfaces or supported multi-window design. Keep one authoritative simulation and render/tactical views sharing state. Upper battlefield and lower minimap/unit/build/orders panel are desired UX, not yet implemented or hardware-verified. Keep single-screen mode fully playable.

## Build commands

Requires CMake >=3.22, C++17 compiler, Python3, git; Android requires JDK17, NDK r27c, Android SDK platform35/build-tools35.0.0 and unzip/zipalign/apksigner. Tools are installed outside the repository.

```bash
bash tools/fetch_upstream.sh /tmp/dr2-reviewed
cmake -S . -B build-host -DREIGN_UPSTREAM=/tmp/dr2-reviewed -DREIGN_SANITIZE=ON
cmake --build build-host --verbose
ctest --test-dir build-host -V
# Only in sandboxes where LeakSanitizer cannot inspect /proc:
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host -V

bash tools/install_android_sdk.sh /tmp/reign-sdk
export ANDROID_SDK_ROOT=/tmp/reign-sdk
export ANDROID_NDK_HOME="$ANDROID_SDK_ROOT/ndk/27.2.12479018"
export REIGN_UPSTREAM=/tmp/dr2-reviewed
bash tools/package_android.sh
```

For a standalone platform-services build, omit REIGN_UPSTREAM. For ARM64 compilation without APK packaging:

```bash
cmake -S . -B build-android \
 -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
 -DREIGN_UPSTREAM="$REIGN_UPSTREAM" -DREIGN_ANDROID_SHELL=ON
cmake --build build-android --verbose
```

Do not run ARM64 CTest binaries on an x86 host. Their linking is verified, their Android execution is not. Diagnostic package and debug key remain in ignored build directories; never commit the key or distribute source-derived binaries pending rights review. CI currently uploads evidence logs only.

## Current next gate after installation audit

Use the original-only Android asset-test APK and ANDROID_ASSET_TEST.md for native file/pack runtime evidence. Host validation of the supplied installation passes (88 packs, 3,763 entries, 1,181 total non-player text configs). Original TBuf now compiles for ARM64; original PTree/FScope/FileSys integration and real Vid/Bitmap rendering remain unimplemented. This diagnostic does not advance M3 to achieved. SDK/JDK/build commands for the distributable asset test are separate from the upstream-derived NativeActivity shell.

## Updated next engineering units

Original configuration/runtime compilation and real J01 parse tests now pass; inspect CONFIG_RENDER_PROGRESS.md. The initial GLES TL batch boundary is implemented and pixel-tested on software EGL. Device gate: ANDROID_RENDERER_TEST.md. Next connect the original Vid/Bitmap surface, texture-cache and render-state paths to this backend; preserve original visibility/camera/mesh code, then continue Main::CoreSystemInit/GameGod::Start. No new gameplay implementation or simplified replacement is introduced. Touch/pen and dual-screen work remains deferred in CONTROLS_BACKLOG.md.


## Completed original Vid dependency slice

The original state setters, texture binding and indexed submission now build for ARM64 and drive tested software-GLES pixels on the host. Reproduce with REIGN_UPSTREAM plus REIGN_GLES_BACKEND=ON and the external Khronos headers; gles_smoke automatically adds the original-function pixel checks when that upstream is configured. CI now configures both options together. The current source overlay remains outside the repository and engine-derived binaries remain excluded from artifacts. Next: original Bitmap lifecycle/conversion, BucketMan flush/sort/allocation, missing texture stages/samplers, then remaining Main/GameGod services. See VID_STATE_PROGRESS.md. This does not advance M3 until actual engine initialization and an original scene run in an APK.


## Completed bitmap conversion and bucket dependency slice

Original opaque/translucent bucket management and Pix/CopyLine conversion now build for ARM64 and run through original Vid calls into host GLES pixel checks. CMake automatically includes the new targets/tests when REIGN_UPSTREAM is configured; staging recipe and hash-file changes trigger reconfiguration. There are 14 host CTest targets and 31 real GPU checks per recreated context. Follow BITMAP_BUCKET_PROGRESS.md for the exact scope. Next connect original Bitmap loading/lifetime/context restoration, InitBuckets and frontend helpers, then remaining Main/GameGod/VarSys/font/interface/input/audio services. Avoid substituting a diagnostic image or a recreated menu for original engine startup. M3 still requires an engine-initializing APK and original scene runtime evidence.


## Completed original font/IFace dependency slice

Original FFF atlas construction, measurement, glyph draw and interface batching now compile for ARM64 and render through original Vid calls in host GLES tests. Retained CPU atlas data can be uploaded again after context recreation. This revision's font draw path uses indexed rectangles; the earlier triangle-fan prerequisite was inferred from a stale define and is corrected.

Renderer consumers must call LegacyGles::present() once after frame draws, before EGL swap. The JNI diagnostic does this. Renderer Test v0.4 exposes 12 own-adapter GPU checks and surface/context metadata for Fold validation. It contains no upstream implementation or game assets. Next: device verification of the changed presentation path, original Bitmap loading/registry and missing formats, FontSys alias/configuration, IFace frame/control/input integration and remaining Main/GameGod services. See FONT_PROGRESS.md. No substitute menu or playable mission is claimed.



## Bitmap loading and v0.4 device checkpoint

Owner Fold5/API36/Adreno740 passes all12 v0.4 adapter checks at context generation2. Native TGA/PIC framing plus original pixel conversion now loads111 TGA and453 PIC images from the supplied base archive. Reproduce with `bitmap_conversion_probe /path/to/base.zwp`; no image assets are committed. See BITMAP_LOADING_PROGRESS.md for the scoped semantics and synthetic/runtime evidence. Next implement original Bitmap registry/lifetime/reload/mipmap integration and FontSys/IFace registration/frame orchestration. M3 remains blocked on actual engine startup and scene execution, not adapter-only diagnostics.


## Original font registration integration

The original FontSys registration dependency is now exercised with the actual standard-font config (nine aliases/six files), and configured/reloaded aliases draw through original Font/IFace/Vid in host GPU tests. CMake automatically builds dr2_font_registry, font_registry_smoke and font_registry_probe when REIGN_UPSTREAM is set. Next preserve Bitmap manager cache/fallback/animation/lifetime behavior, then IFace Metrics/control/frame/input orchestration and Main/GameGod services. No substitute menu or mission is introduced.


## Original interface metrics integration

CMake now stages original IFace metric functions into an external overlay, builds dr2_iface_metrics and includes original_interface_metrics in host tests/ARM64 compilation. FontRegistry::configure optionally dispatches Metrics during the same original parse. Metric-driven font/shadow drawing passes host GPU checks. Next integrate original bitmap manager ownership/reload/animation and IFace colors/skins/control/event/frame startup before attempting an engine-initializing menu APK. No new adapter-only device test is required for this change.


## Original bitmap manager integration

CMake now builds dr2_bitmap_manager, bitmap_manager_smoke and bitmap_manager_probe from an external original manager overlay plus native ownership adapters. CPU-only and actual GLES callbacks are explicit; lost-context abandonment must precede restoration in a new owning context. Host tests and two-context GPU replay pass. Next complete BMP, mipmap/transparency policies and full Bitmap integration, then original interface colors/skins/control/frame/event startup and Main/GameGod services. The public v0.4 diagnostic is unchanged; no new owner testing is required for this source-side checkpoint.


## BMP corpus and control dependency checkpoint

BMP is now connected to original pixel conversion and original manager fallback, with private corpus checks and host GPU tests. CMake adds bitmap_bmp_smoke and bmp_rgba_probe; the local verify_bmp_archive.py checks a legal installation in memory against Pillow without writing assets. Next port original EventSys/TrackSys/color/control startup prerequisites and complete Bitmap allocation/UV/mipmap/transparency policies before constructing ICRoot. The public v0.4 adapter remains unchanged; there is no new gameplay APK or new device-testing gate.


## Runtime recovery and requested menu gate — 2026-10-25

Next device deliverable must show the original main menu at minimum; a refreshed diagnostic does not satisfy it. Original tracking/event/variable prerequisites and a synthetic handoff fixture are implemented, but IControl compilation probes remain failed and full original frontend/startup integration is missing. The interactive workspace became unavailable before upload; implementation recovery uses GitHub and requires fresh CI. Continue at IControl/ICRoot graphics/math/header integration, then original root configuration and User/Login/Shell/frame/input initialization.


## Restored workspace, decompiler verification and control types — 2026-10-25

Current next gate remains a linked original main menu. Native control declarations and selected coordinate/border methods now compile on host and ARM64, with original geometry/color checks. Next integrate the full IControl source against shared native bitmap/font/TL/Input/Event/VarSys declarations, then original root/button/window classes and IFace Process/PaintAll. Continue through real user/login and Vid resources before Main/GameGod/Shell startup. REA is installed but blocked by PID/procfs ownership diagnostics; standalone Ghidra can now inspect the owner executable without touching prior projects. See CONTROL_PROGRESS.md and REA_SETUP.md.


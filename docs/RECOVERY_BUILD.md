# Reproducible ARM64 recovery build

This is a functional reconstruction from checkpoint
`75e34b99c312238ca9367cb4abdcf579665f40f6` and the supplied conversation handoff,
not the lost v0.12 C++ working tree. Source is preserved on
`recovery/native-arm64-v012`. Each delivered APK includes its exact Git revision
and native-library hashes in `assets/build/source.json`, and this document in
`assets/build/BUILD.md`. Commit all source changes before packaging.

## Linux x86_64 prerequisites

Use the existing checkout; cloud tasks are isolated and do not require a new
worktree. Keep all outputs, signing keys, owned assets and upstream source
outside the checkout. Install Java/Javac (17 or 21, targeting Java 8), Python 3,
CMake >=3.22, Ninja, make, GCC/G++, curl, unzip and Mesa EGL/GLES runtime.
On Debian the relevant native packages include `libegl1-mesa-dev`,
`libgles2-mesa-dev`, `libgl1-mesa-dri`; use distribution equivalents where needed.
Current cloud tooling is initialized with `source /workspace/reign-recovery/env.sh`.

```sh
# From the repository root. Choose a new external directory.
work=/absolute/private/reign-build
mkdir -p "$work"
bash tools/fetch_upstream.sh "$work/upstream"
bash tools/install_android_sdk.sh "$work/sdk"
export ANDROID_SDK_ROOT="$work/sdk"
export ANDROID_NDK_HOME="$work/sdk/ndk/27.2.12479018"
bash tools/build_bink_decoder.sh "$work/ffmpeg" host
bash tools/build_bink_decoder.sh "$work/ffmpeg" android
python3 tools/stage_khronos_headers.py "$ANDROID_NDK_HOME" "$work/khronos"
```

These scripts retain upstream source hashes, FFmpeg n6.1.1 SHA-256 verification,
and SDK/NDK artifact checksum verification. Do not disable TLS/signature/checksum
checks to work around download errors. SDK/build-tools 35, NDK r27c, native API
26, `arm64-v8a` and static libc++ are intentional. Preserve `DEVELOPMENT=1`,
`NO_ASM_MATH`, `MONO_DISABLED`, `-O2`, `-fno-strict-aliasing` and `-fwrapv`
across engine boundaries; no fast math. Simulation remains the original 10 Hz,
independent of display refresh.

## Tests and full source build

```sh
cmake -S . -B "$work/host" -G Ninja -DREIGN_UPSTREAM="$work/upstream" \
  -DREIGN_SANITIZE=ON -DREIGN_GLES_BACKEND=ON \
  -DREIGN_KHRONOS_HEADERS="$work/khronos" \
  -DREIGN_FFMPEG_ROOT="$work/ffmpeg/host/install"
cmake --build "$work/host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  ctest --test-dir "$work/host" --output-on-failure
cmake -S engine -B "$work/engine-host" -G Ninja \
  -DREIGN_UPSTREAM="$work/upstream" -DREIGN_SANITIZE=ON \
  -DREIGN_KHRONOS_HEADERS="$work/khronos"
cmake --build "$work/engine-host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  ctest --test-dir "$work/engine-host" --output-on-failure
mkdir -p "$work/java"
javac --release 8 -d "$work/java" \
  android/menu/org/reigncarnated/menu/{DataFolderImporter,ZipGameSource,FrameTiming}.java \
  tests/{DataFolderImporterTest,ZipGameSourceTest,MusicImportTest}.java
for test in DataFolderImporterTest ZipGameSourceTest MusicImportTest; do
  java -cp "$work/java" org.reigncarnated.menu.$test || exit 1
done
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk /private/path/to/verified-v07-or-v12.apk \
  --output "$work/candidate" --jobs 4 --new-recovery-key
```

The full builder verifies and stages pinned upstream, compiles the selected 603
original engine units, rebuilds foundations and both native libraries, then
recompiles Java, packages, aligns and signs. Base APKs are accepted only by known
SHA-256; both native libraries are replaced, not silently borrowed from the base.
The base provides bundled notices; proprietary game assets are never bundled.
For relink-only recovery, `tools/link_mission_android.py --original-archive`
accepts the recovered engine archive; this is distinct from a full source build.

## Signing and device validation

`--new-recovery-key` explicitly creates the separate application
`org.reigncarnated.recovery`, keeping the installed candidate and its data intact.
Keep the generated keystore private and reuse it in the same external output
directory. For compatible updates of the original app, provide the original
private candidate key with `--signing-key /private/key.keystore` instead; never
substitute a key silently. Neither keys nor owned game data belong in Git.

Import owned game ZIPs/folders through the launcher. Supplemental music ZIP
import publishes a separate transactional snapshot and leaves game data intact.
Debug tools provide explicit frame PNG/report sharing, fast/synchronized stream
comparison, one-frame geometry audit, GPU checks, intro replay and mission layer
toggles. Normal rendering performs no frame readback or per-draw GL error query.

The historically reported 110–120 FPS on Adreno 740 is a device target, not a
host-test result. Compare fast/synchronized modes on the same scene, retain
source receipts, share diagnostic reports and explicit captures, and listen to
music/intro playback. Desktop fidelity, other missions, complete shell/AI/save
flow, and on-device performance require separate validation. Resolution/mod/
cheat controls are not advertised as working features in this recovery.

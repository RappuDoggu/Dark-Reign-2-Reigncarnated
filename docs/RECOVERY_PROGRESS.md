# Native ARM64 recovery progress

Recovery started 2026-11-02 (America/Los_Angeles).

## Provenance and scope

- Starting checkpoint: `75e34b99c312238ca9367cb4abdcf579665f40f6`, fetched from
  `dev/native-arm64-primitives`.
- Recovery branch: `recovery/native-arm64-v012`. The branch name describes the
  intended recovery; it does **not** claim v0.12 has been reconstructed.
- `builds/v0.7/recovery.zip.part1` through `part5` reconstruct an archive with
  SHA-256 `f3122fc8a81a24fb53824d21c21d9d23691ba974ccb5a76a9099e5c294d43abe`.
  All 589 ZIP members pass CRC verification. All archived source-snapshot files
  match the starting checkpoint byte for byte.
- The archive identifies its source as
  `4bdab5af0e9c58c62f6d033db1f17190d5b19db6`. Its README mentions a signing key,
  but the checked-in recovery archive contains no keystore. Do not assume one
  is present or publish credentials or owned game data.

## Verified baseline

- Upstream checkout: `9ce9da5f21604310a997f0c41e9cd383f5e292c3`, clean.
  `tools/prepare_engine_full.py` passed the repository's full source-tree check:
  1,791 files, aggregate SHA-256
  `13bc6a7028f64297118b8f1238261873aa0fe1f7cc74f28eca24c31a6b362724`.
- SDK platform 35 revision 2, build-tools 35.0.0, NDK 27.2.12479018/r27c:
  downloaded and verified with `tools/install_android_sdk.sh` unchanged.
  SDK 35 is the compile SDK; the existing native minimum API remains 26.
- FFmpeg n6.1.1 source passed the pinned SHA-256 check in
  `tools/build_bink_decoder.sh`; host and Android static decoder builds passed.
- Java/Javac 21.0.12.1, Python 3.12.14, CMake 4.4.4, Ninja 1.13.2;
  host software EGL/Mesa and extracted Debian GLES runtime available.
- Java import tests: 28 folder-import checks and 12 ZIP-import checks passed.
  All Android menu Java sources compile against SDK 35 using `--release 8`.
- Engine foundation host build and all 9 tests passed under ASan/UBSan.
  Engine ARM64 foundation build passed.
- Main host build and all 65 tests passed with ASan/UBSan and software GLES,
  including `gles_legacy_pixels`. This does not test owned-data mission draws.
- Both native libraries retain 16 KiB ELF LOAD alignment. Repeating both
  relinks produced identical SHA-256 hashes.
- Both archived relink scripts passed with NDK r27c. After `llvm-strip
  --strip-unneeded`, both libraries match the original v0.7 APK byte for byte:
  mission `de4d63522736b6af1351fa0d0561815afd19a67b2e3b7e66383fe4a37701d1da`;
  menu `b952d9a01c04b6ae928dbae024ce85b2d8d5145aaf338966ba2a912596d677d1`.
- Original v0.7 APK SHA-256 matches its existing receipt:
  `9527160c7d594b6b36a99e71cb0ead8303ed58340d49e56e6c85f916d0fccc7b`.
  APK signature verification passed. No new APK is delivered by this checkpoint.

## Reproduce the baseline

Use the existing checkout; no extra worktree is required. Keep build outputs,
toolchains, upstream sources, signing keys and owned assets outside the checkout.
On a Linux x86_64 host, install a JDK, Python, CMake, Ninja, make, GCC/G++,
unzip, EGL/Mesa and GLES runtime dependencies. This cloud machine lacks root;
missing Java/GLES packages were downloaded through signature-verified Debian
APT metadata and extracted into `/workspace/reign-recovery/host`. CMake/Ninja
are isolated in `/workspace/reign-recovery/tools-venv`. Current-machine paths
are initialized by `source /workspace/reign-recovery/env.sh`.

From the repository root, with an external `$work` directory:

```sh
bash tools/fetch_upstream.sh "$work/upstream"
bash tools/install_android_sdk.sh "$work/sdk"
export ANDROID_SDK_ROOT="$work/sdk"
export ANDROID_NDK_HOME="$work/sdk/ndk/27.2.12479018"
bash tools/build_bink_decoder.sh "$work/ffmpeg" host
bash tools/build_bink_decoder.sh "$work/ffmpeg" android
python3 tools/prepare_engine_full.py "$work/upstream" "$work/staged"
python3 tools/stage_khronos_headers.py "$ANDROID_NDK_HOME" "$work/khronos"
cmake -S engine -B "$work/build-engine-host" -G Ninja \
  -DREIGN_UPSTREAM="$work/upstream" -DREIGN_SANITIZE=ON
cmake --build "$work/build-engine-host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$work/build-engine-host" --output-on-failure
cmake -S . -B "$work/build-host" -G Ninja \
  -DREIGN_UPSTREAM="$work/upstream" -DREIGN_SANITIZE=ON \
  -DREIGN_GLES_BACKEND=ON -DREIGN_KHRONOS_HEADERS="$work/khronos" \
  -DREIGN_FFMPEG_ROOT="$work/ffmpeg/host/install"
cmake --build "$work/build-host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  ctest --test-dir "$work/build-host" --output-on-failure
cmake -S engine -B "$work/build-engine-android" -G Ninja \
  -DREIGN_UPSTREAM="$work/upstream" \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -g -fno-strict-aliasing -fwrapv'
cmake --build "$work/build-engine-android" --parallel 4
```

Concatenate the five recovery ZIP parts in numerical order into an external
file, check the SHA-256 above, check ZIP integrity, and extract externally.
Then run `NDK="$ANDROID_NDK_HOME" bash /path/to/extracted/mission/relink.sh`
and the corresponding `menu/relink.sh`. Strip copies of the resulting libraries
with the NDK's `llvm-strip --strip-unneeded`; compare those bytes against
`lib/arm64-v8a/libreign_mission.so` and `libreign_menu.so` in the checked-in
v0.7 APK. This verifies object-based recovery, not a full 603-source rebuild.

The original baseline packager required its exact external v0.5 base APK (SHA-256
`1d0fa253b52d5d7200caa3d356a1f6b58f9d2fa7084f1cd61940581ef04bc598`).
Existing compatible-update signing also requires the private candidate key.
The reconstructed packager now accepts checksum-pinned v0.7/v0.12 bases too, always replaces both native libraries and recompiles Java. Do not silently substitute an unverified base or signing identity.

## Required next evidence

## Conversation-guided reconstruction

The directly attached conversation handoff has now been read in full. Its
SHA-256 is `64161808f199e316b3c061d3517dca6c1ff95cdf87769a1d6bec2c9e881755b6`.
It describes itself as a reconstructed summary, not a complete verbatim export.
It confirms that later C++ source was lost, and that no additional user-maintained
source repository exists. The second attached v0.12 APK matches the SHA-256 below.
The missing-history blocker described in the initial baseline is resolved.

Reconstruction now restores append-only mapped vertex/index arenas with checked
mapping/unmapping and jointly orphaned storage, fast/synchronized comparison,
bounded weak-owner bitmap/font texture caches, adjacent ordered sprite batching,
frame/native stage timings, bounded native/site and Java session journals,
explicit geometry audit and PNG/report sharing, layer toggles, intro replay/skip
logging, and transactional supplemental music import with menu/mission playback.
These are new equivalent implementations, not exact recovered v0.12 source.
The simulation clock and engine ABI defines are unchanged. GPU/host tests cover
arena rollover with pending colored geometry and retained font atlas uploads.

See [RECOVERY_BUILD.md](RECOVERY_BUILD.md) for reproducible full-source commands,
signing and APK/source receipts. The original signing key is unavailable through
the file-transfer limit; an explicitly separate recovery package/key preserves
the existing installed candidate. No game assets or private keys are committed.

The 110–120 FPS result remains historical Adreno 740 evidence from the handoff.
This cloud host cannot certify that device rate or audible media/fidelity.
Current reconstruction checks: 603/603 original ARM64 source units compiled;
65/65 host tests, 10/10 engine foundation/GLES tests and 55 Java importer/timing
checks passed. Both Android libraries link, including the mission library linked against all
603 newly compiled objects and freshly built FFmpeg-enabled foundations.
Packaging is recorded with each artifact receipt.

Draw rejection diagnostics and referenced-vertex validation are the next phase,
after preserving and packaging the reconstructed streaming baseline.

### Supplied v0.12 binary inspection

The directly attached `Reigncarnated-Diagnostics-v12.apk` was downloaded and
passed ZIP CRC and Android v2/v3 signature verification. SHA-256:
`1cd5e963306dc17d0869eda528fb6f02970164052374ce78d1fda1459f92cbe5`.
Its manifest identifies `org.reigncarnated.missioncandidate`, versionCode 12,
versionName `0.12-diagnostics`, compile/target SDK 35, minimum API 26 and an
ARM64-only native payload. Both native libraries differ from v0.7.

Static strings in both native libraries identify `Stream mode append-only
unsynchronized` and a synchronized diagnostic mode. The DEX contains music ZIP
import, diagnostic sharing, `Diagnostics` and `DiagnosticsProvider`; the
manifest declares a non-exported diagnostics provider with URI grants. These
are binary observations, not executed behavior or recovered source. No precise
source revision is established by this inspection. Native strings also include
invalid-index rejection and behind-camera GPU clipping diagnostics; their
runtime incidence and correctness are unverified.

The accompanying `Reigncarnated-v07-Source-Relink.zip` and
`Reigncarnated-Instant-Action-Data-v06.zip` exceed this session's 32 MiB file
transfer limit and were not downloaded. The supplied source archive has not
been compared with the v0.7 archive already recovered from Git. No game data
has entered the checkout, APKs, or remote backup. The uploaded APK and extracted
inspection files remain outside Git under `/workspace/attachments` and
`/workspace/reign-recovery/v12-inspection`.

At the initial baseline, the chat/Library URLs were inaccessible. The directly
attached reconstructed conversation handoff subsequently resolved that blocker.
The lost v0.8–v0.12 working tree cannot be fetched; this branch reconstructs its
reported behavior from the surviving inputs. The v0.7 record reports 10–15 FPS;
the later 110–120 FPS report is device evidence supplied through the handoff.

No device or owned mission data is available in this session. FPS, rejected
mission draws and invalid live TL vertices have not been reproduced. Future
delivered APKs must embed their exact source revision, ship build instructions
and update this record. Keep signing material and proprietary assets private.

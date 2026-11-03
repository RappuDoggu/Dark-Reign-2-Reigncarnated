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

The reconstructed baseline was packaged first from source
`ccb309fd4560c64b1b119c1b714c6659f247b2a2`, with remote backup verified.
`Reigncarnated-Recovery-Baseline.apk` SHA-256:
`7bf124c34a16c1fffb0e5b7a471ff6842afbb4169c9cf394b1b1d722f3e58543`.
Signature, receipt/library hash equality, ARM64 payload, 16 KiB ELF/ZIP alignment,
notice/build-only assets, and absence of duplicate ZIP entries passed.
It is versionCode 13, `0.12-recovery.1`, separate app `org.reigncarnated.recovery`.

### Subsequent draw investigation

After preserving the baseline, regression tests confirmed that validation of an
unused poisoned TL vertex rejected an otherwise valid indexed batch. The GLES
path now checks all index bounds first, then only referenced TL/clip vertices.
Invalid referenced coordinates or rhw still fail. Tests cover both cases, bounds
precedence, real rendered output, and DX7 dispatch across two EGL contexts.
This is a confirmed code defect, not proof that it caused every historical
device exception.

DX7 draw rejections now retain HRESULT, source line, primitive, FVF, flags,
submitted and converted index counts, vertex count, texture operations/arguments,
filters, cull mode and exception reason in the bounded journal. Unsupported
generated device methods report their names; state setters report type/value.
Unknown formats, primitives, flags and unsupported operations still return errors.
No unsupported draw flags or vertex formats have been silently accepted.

The second package is versionCode 14, `0.12-recovery.2`, using the same private
recovery signer/application ID for update compatibility. Post-change checks:
65 host tests and 10 engine foundation tests pass; focused GLES/TL and DX7
rejection tests pass. The original 603 compiled units are reused for this
adapter-only phase. The simulation and engine ABI remain unchanged.

The historical 555 E_NOTIMPL rejections cannot yet be assigned a cause: their
old logs omitted primitive/FVF/flags. A device mission run with the new report
and explicit PNG is required before extending draw support. No owned-data mission
or Android device is available on this host, and no device FPS/fidelity/audio
success is claimed.

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

## Delivered recovery artifacts

- Streaming baseline: source `ccb309fd4560c64b1b119c1b714c6659f247b2a2`,
  `Reigncarnated-Recovery-Baseline.apk`, SHA-256
  `7bf124c34a16c1fffb0e5b7a471ff6842afbb4169c9cf394b1b1d722f3e58543`.
- Draw diagnostics/validation: source `7ef3fc093ca3710a80573e3a19a7bdff2c338a7b`,
  `Reigncarnated-Recovery-DrawDiagnostics.apk`, SHA-256
  `8121616b20cbc37b653066ea6eca1cd5cdf371e2b9acc3e015c1c3a03bba2dd6`.
  This also resets telemetry snapshots when the menu renderer is recreated,
  preventing counter underflow after context loss.

Both APKs pass signature and ZIP alignment checks, source/native hash receipt
verification, ARM64-only ELF/16 KiB alignment checks, restricted JNI export checks,
and notice/build-only asset checks. They use the same private recovery signer;
version 14 updates version 13 under the separate recovery app ID. Signing keys
remain outside Git. Machine-readable receipts are in `docs/builds/`.

On the device, import owned data, use Fast stream first, play the mission and
share a full report plus explicit frame capture from Debug tools. Compare the
baseline only if needed to isolate a regression. Use synchronized mode as a
controlled diagnostic comparison, not as an assumed performance fix. A fresh
report must include the rejected draw arguments before implementing new draw
semantics. FPS, audible music/intro and desktop-fidelity claims remain pending
that device validation.

## Device feedback and diagnostic ZIP follow-up

Owner reports that the recovery APK imports successfully and loads the menu,
but pillar artifacts and substantial Instant Action flickering persist. Passing
GPU/import checks has not established visual fidelity. Rendering stability and
performance remain the immediate priority. After that gate, focus the main-menu
workflow on campaign missions, cutscenes and options; these are queued work,
not features added by this diagnostic release.

Version 15 adds one automatic diagnostic sweep from either screen, a ZIP with
six requested captures across fast/synchronized modes, per-mode timing/native
reports, GPU checks, build receipt and bounded session history. It restores the
original mode and offers Save/Share. A previous other-screen collection is included
within a stated size cap. Native rendering/simulation code and ABI are unchanged.
The collection state machine and ZIP publisher passed 21 host checks covering
both modes, captured-frame count, restoration, cancellation, failure, ZIP content,
path rejection, size limits, and preservation of a prior ZIP on failure. All Java
sources compile against SDK 35. Android interaction and the reported flickering
still require the owner's next diagnostic ZIP; this release does not claim a fix.

## Device ZIP findings and fenced-stream candidate (version 16)

The two supplied v15 collections passed ZIP integrity checks and identify source
`2a3098a189d5b9271e0f865c2720848f00b792ce`. Current top-level collections cover
the menu and mission; nested prior captures/session tails are historical context,
not additional independent current measurements. Raw logs and screenshots remain
outside Git; a bounded textual measurement summary is saved with the test logs.

Both current GPU reports show three fast-mode rollover pixel failures followed
by three synchronized-mode passes. This is a real device correctness failure
despite the previous software-GLES passes. The fast menu averaged 7.875 ms native
wall time and approximately 120 FPS; synchronized averaged 149.666 ms (6.6 FPS).
Mission fast averaged 7.632 ms (approximately 120 FPS); synchronized averaged
63.236 ms (about 15 FPS). Readback-affected callback intervals are labeled in the
reports; none of these timings is GPU elapsed time.

Version 16 replaces reliance on same-object orphaning with a pool of at most
eight physically separate vertex/index buffer pairs. A retired pair receives a
GPU fence and is reused only after completion. Unsynchronized append mapping
remains enabled in fast mode; offsets never overwrite queued ranges. Pool
exhaustion has an explicit completion wait and journal counter, rather than unsafe
reuse or unbounded allocation. Diagnostics report bank count/waits. Context loss
abandons old fence objects before destroying the mission renderer.

The previously unidentified E_NOTIMPL draws are primitive 2 (DX7 line lists),
FVF 0x1c4 (TL), flags 0, with even counts. That specific path now dispatches
GL_LINES with existing texture/blend/depth state, ignores triangle-only culling,
and checks pair topology and referenced vertices. Unknown primitives/FVF/flags
still fail. The remaining invalid referenced triangle vertex 13 is not bypassed;
future rejections include x/y/z/rhw/uv values for diagnosis.

Host validation: all 65 host and 10 engine foundation tests pass. Targeted checks
cover rollover pixels in both modes across contexts, a forced-unsignaled fence
pool reaching its eight-bank cap and safe wait/reuse, DX7 line-list pixels and
odd-count rejection, and continued rejection of unsupported point lists. Both
Android native libraries rebuild/link against the pinned original engine objects.
The original simulation and ABI defines are unchanged. The candidate's device
flicker, pillar appearance and 120 FPS retention are not yet verified; collect
fresh v16 ZIPs before advancing to campaign/cutscene/options implementation.


Diagnostic ZIP release: `Reigncarnated-Diagnostics-Zip-v15.apk`, source
`2a3098a189d5b9271e0f865c2720848f00b792ce`, SHA-256
`d43b30d6b2b2752f9dc5d1fed1dc7221c68c35a584e6260612b73dad1364afc6`.
APK signature/ZIP alignment and receipt checks passed. It uses the same recovery
signer/application ID and byte-identical native libraries as version 14, preserving
imported data on update. Diagnostic collection code is present in the DEX; only
notice/build assets are bundled. On-device ZIP collection/save/share remains to
be exercised by the owner. Upload menu and mission collections for investigation.


Version 16 artifact: `Reigncarnated-FencedStreaming-v16.apk`, source
`4c843a4f6ecb6ab03fdd2b116d3bb2dc6c5dc4e1`, SHA-256
`00d0b809df67835f682ef051ec6b35e4a36b3ace38df80f84c7f5174b62e00d5`.
Source/native hash receipts, ZIP integrity/alignment, ARM64/16 KiB ELF alignment
and matching v15 recovery signer checks pass. It updates the recovery app with
imported data retained. The all-in-one diagnostic collector remains available;
its next fast rollover result on Adreno is the acceptance gate, alongside visible
flicker and normal fast-mode performance. Host tests cannot close that gate.

## v16 mission device result

Received `Reigncarnated-diagnostics11.zip`, SHA-256
`75edf2e2ee9c8eae46ae330f9fe547b2a41d0f98c68cd188856fbd9f079de93f`.
ZIP integrity passed. Current mission source and native-library hashes match
the v16 receipt (`4c843a4f6ecb6ab03fdd2b116d3bb2dc6c5dc4e1`).

- All three fast and three synchronized rollover checks now pass on the device.
  The fast-mode device pixel-test failure observed in v15 is resolved in this run.
- Fast mission: 120.1 FPS, 7.541 ms mean native wall time, 8.681 ms p95,
  134 draws/frame; four physical stream banks and zero pool exhaustion waits.
- Synchronized mission: 13.7 FPS, 71.610 ms mean native wall time. Keep fast mode
  as the normal path; synchronized mode remains a slower diagnostic comparison.
- Current per-mode native journals contain no rejection/error sites. This does
  not prove the earlier referenced-invalid-vertex scenario is eliminated.
- Simulation cycles progress from 1174 to 1195 across the comparison, consistent
  with the original 10 Hz clock. All three fast PNGs show consistent major scene
  geometry; animated units/effects and resources change between captures. Static
  images cannot establish absence of flicker during continuous play or desktop
  fidelity. Await the owner's visual observation.

The nested prior menu archive identifies v15 (`2a3098a...`), not v16. Its old
fast rollover failures are historical, rather than failures in this mission run.
A new v16 menu collection is still needed to verify pillar appearance and menu
stability before moving to campaigns, cutscenes and options. No additional
renderer changes or new APK are justified by this successful mission collection
alone. Current testing APK remains v16. Raw device logs/images stay outside Git;
a source-verified measurement summary is retained under `docs/logs/fenced-stream`.

## Match menus, controls and pillar address state (version 17)

The owner confirms units move and selection/building work, but pillar textures
still look stretched. The shell model renderer was clamping all material UVs;
pinned `graphics/Vertex.h` specifies wrap by default, with separate mirror/clamp
flags at bit 14. Model and sky buckets now apply those original flags. Reserved
address mode 3 throws an explicit error. Software GLES pixel checks cover tiled
UVs beyond one for wrap, mirror and clamp. This addresses a concrete discrepancy;
the pillar appearance still needs a fresh device screenshot/collection.

The agreed sequence is controls/rendering, Instant Action options and original
AI, then the campaign menu. Version 17 adds two-finger pan/pinch, retains tap and
drag selection, uses the original deselect event, and exposes zoom and placement
direction/rotation buttons through a Controls toggle. Pinch events use complete
120-unit wheel steps, as the original viewer divides wheel input by 120.

Instant Action options enumerate installed maps from original group metadata,
excluding system groups. Choices include practice/one AI, imported difficulty
settings and a reproducible seed. A requested opponent uses original
`Strategic::Create` and an actual randomizable personality mod on the second
playable team; teams become mutual enemies and unused starting teams are purged
after retaining the AI. Missing teams/mods fail explicitly. Reports identify
requested options and actual playable teams, AI status and personality. This is
an initial options menu, not the full original multiplayer lobby.

The Campaigns button and explicit Campaign missions entry now open a selector.
It reads original mission order and the Commander runtime profile's win records,
offering completed missions plus the next mission. Campaign launch retains
original default-client/scripted-AI setup and does not purge scripted teams.
Shutdown saves the user before clearing campaign globals. Full campaign story
flow, cutscenes, completion navigation and shell/runtime profile unification
remain separate work. No campaign mission or AI combat was executed in this
cloud workspace, which has no accessible owned installation or Android device.

Validation: all 66 ASan/UBSan host tests and the 10 foundation tests pass. The new
catalog test covers map filtering, campaign ordering/unlocks, missing maps and
duplicate rejection; the original menu event fixture verifies Campaigns routes
to the selector while unavailable actions remain guarded. A separately linked
host session test passes invalid-option transaction checks and native AI/seed/
difficulty storage; its reused original objects are not sanitizer-instrumented.
Both Android native libraries link without unresolved symbols, with the changed
original session unit recompiled. All Java compiles for SDK 35. The pinned
1,791-file source check passes through `prepare_engine_full.py` into fresh staging.

The v16 fence-protected append stream, original 10 Hz simulation and engine ABI
flags are unchanged. The previously measured 120.1 FPS belongs to the supplied
v16 device run; v17 performance, gesture feel, pillar appearance, AI behavior and
campaign launch/save remain device acceptance gates. Keep the same recovery
signer and application ID; never include signing keys or owned game assets in
source backups or APKs.

Version 17 bundles this progress record alongside source/native hash receipts
and build instructions. Device execution is pending. Test practice controls,
one AI, then the first unlocked campaign mission, and collect fresh menu/mission
diagnostic ZIPs.

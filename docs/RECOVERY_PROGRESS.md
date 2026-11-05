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

Version 17 artifact: `Reigncarnated-MatchMenus-v17.apk`, source
`982a929511d7ab7608c7217fabd2d0b888c7b86e`, SHA-256
`33b34a7e77e86d7dd4e6e7308c256ff92aaec00016acf9683b56841842122ffb`.
ZIP/signature/alignment checks pass, including ARM64/16 KiB ELF load alignment,
matching v16 signer and embedded source, instructions, progress and native hashes.
The APK retains imported data on update. Remote recovery source and a complete
source ZIP are preserved separately from private keys and owned game assets.

## v17 owner feedback and next menu/control work

The owner reports that the pillar artifacts are removed and the AI starts
correctly. These are device observations, not new cloud measurements; no new FPS
measurement or campaign result accompanies this report. Some unit attack effects
are inaudible. The source ZIP download still fails for the owner; they have
deferred that issue. Preserve the remote recovery branch and avoid relying on
the ZIP as the sole backup.

The requested next work is the full original Instant Action selection flow,
including transitions from the main menu, and more intuitive phone/controller
controls informed by RTS selection and command patterns. The v17 Android dialog
is an interim chooser and does not satisfy the original full setup flow.
Control preferences have been asked explicitly and are pending. See
`INSTANT_ACTION_MENU_RESTORATION.md` for source boundaries, option coverage and
the audio investigation; no new menu implementation or audio fix is claimed by
this feedback checkpoint.

## Requested touch controls (version 18)

The owner narrowed immediate controls to map rotation, building rotation during
selection/placement, left click and quick deselect, with controller work deferred.
Two-finger twisting now generates an original `camera.rotateby` command in the
opposite camera-yaw direction so the map follows the screen twist. The original
Standard camera's Circle path retains pitch/height and does not invoke Swoop.
Other camera types fail visibly instead of ignoring rotation. Two-finger input
classifies rotation or pan, locks its choice until lifted, and never sends wheel
events. Zoom is available only through separate buttons.

The fixed toolbar exposes Left click and Deselect; its scrollable portion exposes
building direction arrows, Camera and Debug. A JNI query detects the original
CM_CONSTRUCT placement mode. In that mode, touch aims the preview, building arrows
change the original placement direction, and Left click confirms. Deselect exits
forced order/placement mode and clears selection. Ordinary taps and box selection
are retained. Controller changes have not been made. The full original Instant
Action menu and attack-sound investigation remain open work.

Validation: 30 host gesture checks cover tap/drag, preview aim/explicit click,
both twist directions, angle wrapping, pinch without zoom, gesture lock, pointer
replacement, cancellation and placement-mode transitions. All 10 foundation tests
pass, including original input/frame clock/PCM checks. Java compiles against SDK
35 and the ARM64 mission library links with zero unresolved symbols. The menu
target requires no rebuild and retains the v17 native library. The pinned source
check and ABI/simulation settings are unchanged. The 66 host frontend tests were
not repeated for this touch/mission-only change. Device behavior and performance
of v18 still need validation. The all-in-one diagnostic ZIP remains available;
mission reports now identify active camera and placement state.

Version 18 artifact: `Reigncarnated-TouchRotation-v18.apk`, source
`a65a0c82a03f9cefaae552d0db6efcb3fd59c1ee`, SHA-256
`dd24b64fa9f668a4e60ebabc4fd69627dcecdb1c597b062839a0c7b6ea5906d8`.
APK ZIP, signing, alignment, ARM64/16 KiB ELF loads, new Java/native placement
entry point and embedded source/build/progress receipts pass verification. The
signer matches v17 for compatible updates. A fresh download copy is provided
separately from private signing keys and owned game data. Device checks remain
pending; no new on-device FPS result is claimed.

## v18 device feedback and version 19 corrections

The owner reports continued broken finger interactions, two fingers driving the
camera to maximum zoom, an outdated main-menu version and missing Instant Action
transition. The requested replacement is two-finger pan only, three-finger
building rotation and single-finger double taps for click actions. Controller
work remains deferred.

Source inspection found a concrete pan height-contract error: the adapter called
Standard::Set with world position, while SplitMatrix interpreted its Y coordinate
as terrain-relative height and the original simulation added the floor again.
Version 19 adds a non-virtual PanNative method that moves only targetPos X/Z, using
the original scroll path's height/pitch handling. Touch wheel/zoom and map-rotate
actions are removed. ARM64 ABI probes give Base size/alignment 256/8 and Standard
496/8 both before and after this method addition; no fields or vtable slots change.
The SHA-verified stager and reproducible full build include this source adapter.
The original viewer_standard and multiplayer units are recompiled for this APK.

Single touches aim only. A double tap issues one original left click, including
building selection, movement and placement. Two fingers emit pan only. Horizontal
three-finger swipes step building direction while construction mode is active.
Finger count/ID changes rebase without jumps; trailing fingers/cancel do not issue
clicks. One-finger box selection is replaced by aiming. Existing click, building
rotation and quick deselect buttons remain. The all-in-one ZIP collector remains.

The old original `sys.buildver` string was hardcoded to 0.5 Native Launch. The
menu now binds PackageManager's installed version name/code there and displays
the same version in a persistent corner label. Both Instant Action entry points
play the imported Main::Exit curve, hide main controls and wait for completion
before opening selection. Cancel/Back plays Main::Enter and restores controls at
completion. A bounded timeline loads imported bookmarks, preserves their final
valid pose and rejects missing/duplicate names. Activity pause/GPU restoration
return navigation to Main; stale dialog callbacks are guarded. Full source setup
UI/objective scheduling and attack audio remain tracked separately.

Validation: all 66 sanitizer host tests pass; camera tests now cover 29 checks
including entrance/exit/return timing and held poses. The revised touch test has
31 checks for double-tap-only clicks, aiming, two-finger pan without zoom/rotation,
three-finger building steps, pointer changes and cancellation. Both Android
libraries build/link and all Java compiles with SDK 35. The pinned 1,791-file
source check passes. The original simulation, ABI defines and fenced streaming
are unchanged. No v19 device test/FPS measurement has occurred; zoom invariance,
physical gesture feel, building placement and menu navigation need phone testing.

Version 19 artifact: `Reigncarnated-PanDoubleTap-v19.apk`, source
`69040f46848c13ec6d9c6106614a733bcd3ca204`, SHA-256
`23cc0b939ef1c83fa0a813f15a6cabe41c0827ef90e1f35a0cf2e1feefdf252d`.
APK ZIP, signatures, alignment, ARM64/16 KiB ELF loads and source/build/progress
receipts pass. Both native hashes match the rebuilt libraries; the new navigation
JNI methods are present and the stale native 0.5 version label is absent. Manifest
version is 0.15-pan-double-tap (code 19). The recovery signer matches v18. This
artifact needs phone validation; the source branch remains the primary backup.

## v20 startup regression fix

The owner supplied a v19 screenshot showing `Native startup failed: Expected one
run code`. The failure is reproduced on the host by calling set_build_version
immediately after constructing MenuFrontend, matching native State startup.
The previous host suite omitted this newly added version-label call.

Original VarSys inherits a scope's NOTIFY flag into direct child variables.
Registering the entire sys scope with NOTIFY caused sys.buildver's SetStr to
invoke runCodeCommand without console arguments. The updated frontend registers
sys and gamegod handlers without broad scope notification, retains NOTIFY on the
actual commands, and checks sys.runcode's identity before parsing arguments.
No command argument validation is bypassed.

The before-fix test fails with the exact device message. The repaired menu runtime
test passes 34 checks, including version assignment, opening the original shell
preview, repeated version changes, invalid run-code argument rejection, existing
navigation, rendering and profile reopen. All 66 sanitizer host tests pass and
the ARM64 menu library rebuilds. The mission library and Java controls retain
their v19 source/behavior. SDK 35/NDK 27.2.12479018, original ABI flags, fenced
streaming and 10 Hz simulation are unchanged. Phone startup and the v19 gesture/
transition changes still require device validation. No reimport is required for
this code fix; the APK retains the recovery identity and signer.

Version 20 artifact: `Reigncarnated-StartupFix-v20.apk`, source
`9c09e6419a0aa7036a30a012e287740d4422af77`, SHA-256
`2e2aa3362a0e5a525c103a7ee873590566520bd4c4cc1fbe722dedb0355c6017`.
ZIP, signature, alignment, ARM64/16 KiB ELF loads and embedded source/build/
progress receipts pass. Manifest code is 20. The mission native hash is identical
to v19 and the signer matches for an update retaining imported data. Phone startup
is pending; this release fixes the reproduced notification regression.

## v21 single game-data backup and restore

The owner confirmed v20 is functional for testing, then requested one ZIP with
all required game data because separate imports lose music or Sprawl. No owned
installation is materialized in this workspace: the earlier attached v06 game
archive exceeds the executor download tool's 32 MiB transfer limit. A new game
ZIP cannot honestly be supplied here from the source/relink archive. Instead,
v21 exports the installation and selected music already present on the phone.
The game data stays private and is not put in Git or the APK.

Report / data now has Export complete game-data ZIP and Save ZIP. The exported
canonical paths round-trip through the actual ZIP importer, preserving Sprawl,
base/shell, side packs, maps, mods, library files and selected separate music.
Required packs/entries and music are checked; missing assets stop export by
name. Available campaign/movie/world files are included, but unavailable assets
are not invented. Each file has a SHA-256/length manifest, and the APK source
receipt is included. ZIP CRC verification precedes atomic private publication.
Cancellation/failure keeps a prior output. Profiles/saves/signing keys are absent.
The Android document picker lets the owner save the ZIP without a cloud upload.

A source inspection found two related re-import bugs. Selecting a new data
snapshot could leave music preferences referencing a removed old snapshot;
imports now select new music or retain valid existing music and its directory.
Mission resource links previously kept the first imported path indefinitely;
they now refresh to the current snapshot, remove stale optional pack links and
repair dangling links. Real runtime files/directories are protected by preflight,
and profile/saves are never removed. The resource-link test deletes the first
snapshot and verifies the second's world/base/map bytes remain readable.

All 67 sanitizer host tests pass, including the new filesystem restore test.
Leak detection is disabled under the workspace ptrace sandbox as documented;
ASan/UBSan remain enabled. Java import/export tests pass: data folders 28 checks,
ZIP sources 12, music 15,
diagnostics 21, controls 31, and the new bundle round-trip/error tests 26. All
Android Java compiles with SDK 35. The ARM64 mission library relinks without
unresolved symbols using NDK 27.2.12479018 and unchanged original-engine object
selection/ABI flags. The menu native library is byte-identical to v20; 10 Hz
simulation, renderer/streaming and controls are unchanged. Export Save and actual
game-data restore need phone validation; no new FPS claim is made.

Version 21 artifact: `Reigncarnated-DataBundle-v21.apk`, source
`2f56830ae1e30a1c2e31076ffb3212181eb8ea73`, SHA-256
`9cdeb2d7f1c67ff4d1cc40a8e637e7c34460551acf42b93ff3fd52530083ff7e`.
APK ZIP checks, signature, recovery package/version 21, 16 KiB alignment and
ARM64 ELF loads pass. Embedded source/build/progress receipts match the committed
source. Menu native hash and recovery signer match v20. The mission library
contains only the resource-reference changes described above. Export on an
actual Android document provider still needs device validation.

## GitHub game-data release investigated after workspace recovery

The failed cloud workspace was replaced/restored to the published Phase 0
snapshot. Re-fetched the verified recovery branch from GitHub; no engine changes
were reapplied. Investigated the upstream `game` release requested by the owner.
Both available ZIPs download successfully and pass CRC checks. R4SOURCE contains
source code only. microDR2 wraps UHA 0.6, decoded using a source-built Linux
extractor with a checksum-verified Rust toolchain; the Windows EXE was not run.
All 37 extracted files pass CRC. The 15 game packs pass the existing repository
metadata inspector. Sprawl matches the owner's tested pack, but base and shell
are reduced and differ. Only one multiplayer map is present; campaign maps,
movies and all loose music tracks are absent. No OGG/MP3 tracks occur inside the
base/shell tables either.

The actual APK importer code, executed on the host, imports all three scene
packs and reports zero music. The complete-export guard refuses this snapshot
and leaves no output. No incomplete game ZIP was delivered as a complete one.
A full original installation or complete phone export is still required; code
cannot recreate the missing content. Evidence and hashes are recorded in
`github-game-release-audit.json`. No APK, engine behavior, ABI, simulation, key
or game asset was changed or committed during this investigation.

## Instant Action menu and campaign availability review

The owner paused archive-import work while another chat prepares GitHub file
updates. The current GitHub heads were checked: main 0bc1fe7, development 75e34b9,
and recovery 37e5202. No new file/data commits were present at review time. None
of those tracked trees contains campaign ZWP/game data or music. The v0.7 archive
parts are the previously verified source/relink kit, not an installation.
The repository's GitHub visibility is public; full game archives must remain in
a separate private source accessible to the environment, and signing keys must
remain outside Git.

The owner confirms the animated transition works, but the Instant Action menus
do not display and the animation should show a different city area. Source review
confirms the implementation still chooses Main::Exit and stops drawing original
menu controls in Choose. Only an interim Android chooser is wired. Decoding the
reference shell configuration establishes the actual Instant::Enter ->
Cineractive::Multi::Enter -> Multi::Enter -> CineractiveCompleted ->
GameSetup::Instant activation sequence. Back uses Multi::Exit. The objective
sends completion at 2 seconds, before either full camera path ends. Main::Exit
belongs to Quit/Logout. Details and return-from-mission distinctions are recorded
in INSTANT_ACTION_MENU_RESTORATION.md. This review does not claim a fix or APK.

Campaign enumeration, original mission ordering, progress-based unlocks and
campaign launch plumbing exist in the recovery source. The earlier full owned
installation audit found two campaigns and 60 missions across all groups; the
current GitHub source does not supply those playable campaign files. The decoded
micro release also has no campaign mission packs. Once the owner pushes the
private data update, verify missions/jda and missions/sprawler group.cfg files,
their Campaign/Missions/Add references, each corresponding mission pack and
required worlds/sides/mods before claiming campaign availability. Campaign story
flow, cutscenes, completion navigation and full menu integration are unfinished.
No engine, renderer, simulation, ABI or APK changes were made during this review.

## v22 Instant Action city and in-engine recovery setup

The owner authorized an Instant Action fix APK after review of PR #2. Its three
data-packaging/receipt files are incorporated from 170fb80 without committing
owned game bytes. The native source now selects the camera and UI activation
schedule from Cineractive::Multi::Enter and Cineractive::Multi::Exit. Controls
appear at the imported CineractiveCompleted event (2 seconds in the reference),
while the longer city path continues. Quit/Logout's Main::Exit is no longer used
for Instant Action. Back/Android Back return to Main on the source event schedule.
Invalid/unsupported objective schedules and actions fail explicitly.

The original control renderer now draws GameSetup::Instant as a persistent native
recovery panel. It uses original listbox/buttons/edit/font/theme/input services
and a scrollable map list identified by group. Its practice/one-AI, difficulty
and positive 32-bit seed selections reach the existing native mission launch;
there is no asynchronous Android Instant Action dialog to lose during lifecycle
changes. Pending ICEdit text is committed before validation, invalid seeds keep
the panel open, the numeric keyboard is available, duplicate launches are gated,
and Back/reset clear stale activation state. The complete original network/lobby
widgets, previews and advanced team/host options are still not restored; this
candidate exposes only settings applied by the native mission session.

Java import/export/music/diagnostic/touch checks pass 133 checks, and all Android
Java compiles with SDK 35. Menu runtime tests pass 49 checks, including actual
pointer map selection,
opponent/difficulty changes, seed editing/rejection, Launch, Back and repeated
panel preparation. Shell navigation tests pass 26 fixture checks and 4 additional
checks against the decoded reference objectives. All 68 ASan/UBSan host tests
pass with documented leak-detection disabling under the ptrace sandbox. Software
GLES renders the city/setup controls, verifies activation before the 5-second
entry camera ends, exercises Launch/Back and reproduces identical pixels after
GPU context recreation. This is a visual-only reference probe; production audio
is unchanged and audible playback has not been revalidated on the phone.

SDK 35, NDK 27.2.12479018, full Java 21, CMake 3.31.6, Ninja 1.11.1.1 and pinned
FFmpeg n6.1.1 are restored. The 1,791-file upstream check passes; all 603 selected
original engine translation units compile, and both ARM64 native libraries link
without unresolved symbols. The old 10 Hz simulation, original ABI configuration,
controls, rendering operations and fenced stream remain unchanged. No on-device
FPS/performance result is claimed for v22.

The prior cloud machine's private signing key is absent. The candidate therefore
uses a new side-by-side test identity, org.reigncarnated.instantcity, to preserve
the existing recovery installation/data. It needs the verified data ZIP imported
once in the new app. A private signer backup is kept outside Git/APK for future
compatible test updates. This is not an in-place update signed with the old key.
Every delivered APK retains source revision, native hashes, build instructions
and this progress record. Campaign UI/story flow and remaining attack-sound
investigation are unchanged by this Instant Action candidate.

Version 22 artifact: `Reigncarnated-Instant-City-v22.apk`, source
`a9f19789b2734e6bf6afa40bb294381620105807`, SHA-256
`70354a70d008e1b0a14d9389eaabbcc24d6ed9b29c2003749a6e12f15973f061`.
The owner selected a separate test app. Signature, ZIP CRC, 16 KiB ZIP alignment,
ARM64 ELF loads/static libc++, version code 22, explicit application identity,
new JNI exports and embedded source/build/progress receipts pass verification.
The private test signer has a separate owner-downloadable backup outside Git.
Source/data-package changes are preserved on the recovery branch. Device menu,
mission launch, pause/resume and FPS remain acceptance checks.

## APK 22 download delivery

The owner's Codex client cannot open the internal workspace APK links. The exact
verified APK 22 (unchanged SHA-256 70354a70d008e1b0a14d9389eaabbcc24d6ed9b29c2003749a6e12f15973f061)
is mirrored under builds/v0.16 for a normal HTTPS GitHub download. This is delivery
of the already-authorized test APK, not a new build. Game data and the private
signer/backup remain outside the public repository.


## APK 23 original offline menus, Android options and campaign loading path

The owner confirms APK 22 runs, reports campaign loading stalls and green/dark
mission rendering, and requests original Instant Action menus, Android options
and maximum graphics. No new screenshot or device ZIP accompanied this report.
The complete owned campaign ZIP is still not materialized in this cloud machine.

Production Instant Action now imports the original GameSetup::Instant,
GameConfigPlayers, GameConfigMission and GameConfigMenu layout with original
skins, localization and Map/Back/Launch icons. The offline adapter connects the
existing map/one-AI/difficulty/seed options, loads original preview metadata and
terrain.bmp when present, and validates starting-team capacity. Unsupported
team/alliance/multiple-AI and advanced host options remain explicitly pending on
screen. This is not a claim of complete original network/lobby parity.

The original Options button now opens working Android music/effects volume, pan
speed, brightness, keep-awake and performance-overlay settings, also available
in missions. Values persist and reach the actual audio/window/control services.
The High graphics preset retains unreduced 32-bit textures, full meshes and
original detail factors. GLES now implements real point/linear mip filtering,
lazy retained mip chains, invalidation after updates and accurate cache bounds.
The city terrain/models also use trilinear filtering. The 640 x 480 compatibility
viewport and unsupported MSAA/stencil-reflection/multitexture boundary are stated
in RECOVERY_BUILD.md. No new phone FPS claim is made.

Source tracing found campaigns were classified as having offline Instant Action
session data. Campaigns now enter the original single-player load branch, keeping
the mission's default client/scripted teams, original seed and campaign enemy
difficulty policy. A synthetic campaign grouping of the reference map passes
loading, simulation, default-client selection, pause/resume and cleanup twice.
This is a structural campaign-path regression check; the reported retail mission
stall, cinematics and full story progression are not verified by that fixture.

Loading status now remains until simulation starts. A thread-safe bounded stage
journal can prepare a diagnostic ZIP without waiting for the render queue; the
original loader subsystem and calls are recorded. A blocked native call can
still block Android surface/picker transitions, so this is observability, not an
unsafe thread-abort mechanism. The fog comparison changes distance fog only;
original sight/shroud and mission lighting stay intact. Runtime reports include
ambient/fog colors and shroud state. The reference map's intentionally colored
lighting is not evidence that the owner's affected map is visually correct.

Validation: 68 host tests and 11 engine-foundation tests pass, including real
GLES minification/update pixels and concurrent loading journal checks. Java
compilation with SDK 35 and 133 importer/music/export/diagnostic/touch checks
pass. The imported original Instant Action GPU probe passes Map/Done/Launch/Back
and exact image equality after context recreation. The reference Instant Action
mission and synthetic campaign path each pass load/simulation/pause/resume/close
twice on the host; the real Vid attachment/GLES/detach check also passes twice.
All 603 selected original units compile for ARM64 and host; the three changed
session/video units are rebuilt as explicit overrides. Both ARM64 libraries are
rebuilt and linked without unresolved symbols. Pinned upstream verification,
SDK 35, NDK 27.2.12479018, FFmpeg n6.1.1, ABI flags, original 10 Hz simulation,
existing touch gestures and fenced unsynchronized geometry streaming remain.

The delivered test update uses APK 22's private signer and
org.reigncarnated.instantcity identity, retaining that app's data. Source revision,
build instructions and this record are embedded. Signing keys, owned packs and
reference screenshots remain outside Git and the APK. Retail campaign loading,
fog/lighting appearance and phone performance remain explicit acceptance checks.


APK 23 artifact: `Reigncarnated-Original-Menus-Options-v23.apk`, source
`17c61ca85c0d3234895cb79f23e71e204c27c3b8`, SHA-256
`5e2336da039c13f122a9d62fddcfa83033f65b5061a9595697430843187fd487`.
APK CRC/signature, same signer as APK 22, embedded source/docs/native hashes,
ARM64 16 KiB ELF loads and ZIP alignment, static libc++ and absence of owned
packs/private keys pass independent verification. The unchanged signed APK is
mirrored under builds/v0.17 for the direct HTTPS download needed by mobile Codex.


## APK 24: restored touch and combat sound lifetime; campaign crash evidence

The owner reports campaign-load crashes and missing attack/hit sounds, requests
single-finger left-mouse clicks/drag selection, hold-to-build confirmation and
three-finger tap rotation, and defers further Instant Action layout work. No new
crash report or retail campaign archive was supplied with this request.

A reproduced native audio regression cut playback in Effect::~Effect, whereas
the original engine permits already-started samples to outlive transient FX.
Destruction now detaches the owner and preserves PCM playback; explicit stop
continues to stop loops. An integration fixture uses real original Effect,
FileSys, DTrack/Reaper and authored WAVs to verify attack/impact playback after
owner destruction, reused addresses, missing resources and explicit loop stop.
Voice stealing also restores the original strictly-lower-priority rule.
AAudio now handles negotiated output rate and reconnects after disconnection
outside its callback. Bounded resource, mixer and driver diagnostics distinguish
missing/failed/suppressed effects from silent output. GameSound::Poll matches
the original frame loop. Actual phone combat audio remains an acceptance check.

Touch now sends original held left mouse for one-finger drag, single-tap clicks,
and one stationary 450 ms hold click for build selection/placement. Placement
short taps only aim. Three-finger tap rotates a building once; two fingers pan
only. Cancellation/multitouch/pause release held input, holds do not repeat and
neither pinch zoom nor swipe rotation is introduced. Controller changes and
Instant Action layout adjustment remain deferred.

The retail campaign fault remains unconfirmed. MissionActivity now runs in a
private :mission process to contain a native crash and preserve the menu process.
Typed settings snapshots transfer on launch/normal return, avoiding unsupported
SharedPreferences multiprocess caching. New internal loader boundaries flush to
a bounded private disk journal so an abrupt process death preserves its last
stage. The menu collects the mission log and own-app Android historical crash/
ANR traces where provided by the OS. These changes improve containment and
post-crash evidence; they do not establish a campaign crash fix or Android
lifecycle validation. A report after the next retail crash is still needed.

Validation: all 12 engine foundation tests and 150 Java checks pass; 48 cover
mouse/hold/multitouch state transitions. The real original-class sound fixture
passes, as does the AAudio boundary fixture (44.1 kHz, disconnect/reopen, invalid
format/start failures). Journal tests prove a child-process abrupt exit retains
its last stage and bounded logs. Reference Instant Action and a synthetic
campaign group each pass load/simulation/pause/resume/shutdown twice. They do
not cover retail story scripts or actual Android hardware. SDK 35 Java compiles;
all 603 original ARM64 units rebuild with -O2 -g and original ABI flags. The
private unstripped ELF resolves original Game::OpenMission and adapter Frame
addresses to source lines; APK libraries remain stripped.

SDK 35, NDK 27.2.12479018, FFmpeg n6.1.1 and the 1,791-file pinned upstream check
remain. The original 10 Hz simulation, supported High graphics and fenced
append-only unsynchronized geometry stream are unchanged. No new phone FPS
measurement is claimed. Version 24 uses APK 22/23's private signer and test
identity, retaining imported data; keys, game packs, crash traces and symbols
stay outside public Git/APK. Reproduction and post-crash export instructions
are in RECOVERY_BUILD.md; exact source revision and records are embedded.


APK 24 artifact: `Reigncarnated-Touch-Combat-Audio-v24.apk`, source
`fa43ad3a7fa0fb750ff6a7721dafd5e2a44b766a`, SHA-256
`7343d4c0f16d3136af49eaee2257727847e94868c6165a53132299c8ef219c84`.
Independent checks pass: same APK 22/23 signer and identity, version 24/private
mission process in the compiled manifest, signature, CRC, 16 KiB ZIP/ELF loads,
static libc++, exact embedded source/docs/native hashes and absence of game packs
or signing keys. The unchanged signed artifact is mirrored under builds/v0.18.
The private unstripped ELF is retained outside Git for matching crash addresses.


## APK 25: uploaded J01 evidence, lexical path fix and truthful readiness

The owner's APK 24 ZIP matches its exact source/native receipt. J01 finishes
mission setup in about 350 ms, then the first gameplay frame returns `Empty
engine path`; simulation cycles and effect requests remain zero. AAudio is
running without callback errors and music plays. This report diagnoses a stopped
first frame, not missing combat samples in a running battle. Raw device reports
and platform traces remain private outside Git/APK.

Native `Dir::PathExpand` incorrectly used OS path resolution for the original
lexical `_splitpath` contract. Empty optional texture names threw before the
original bitmap manager could handle them. New I/O and original bitmap-manager
checks fail with the old implementation and pass with lexical splitting. Case,
separator spelling and DOS drive components are preserved during splitting;
actual empty OS opens and unmapped drive access still fail. No rendering errors
or unsupported operations are bypassed. The exact J01 throw-site caller is not
in the report, so successful retail startup remains a phone acceptance check.

Mission readiness now requires the first nonzero simulation cycle. Fatal JNI
startup/frame exceptions set FAILED, clear readiness and remain exportable;
nonfatal operation errors are still explicit. The deterministic GNU mission
build ID makes future OS crash traces matchable to private unstripped symbols.

The ZIP also contains an older main-process native fault about 65 minutes before
collection. Its menu build ID matches retained symbols and its stack resolves
to FileSys::Close/DataFile destruction during nested PTree/BlockFile menu loading.
It is distinct from the current caught J01 exception; no corruption root cause
or fix for that historical menu crash is claimed. The capped trace stays private.
See DIAGNOSTICS_REVIEW_V24.md for the sanitized review and remaining checks.

Validation: 12 ASan/UBSan foundation tests pass. Original bitmap-manager empty
lookup and reference Instant Action/synthetic campaign paths pass; both paths
load, simulate, pause/resume and close twice. Zero cycles never reports READY.
All Android Java compiles against SDK 35 and 48 touch checks pass. The changed
ARM64 I/O foundation and adapters rebuild; the mission relinks the verified 603
original APK 24 objects with no unresolved symbols. Upstream source pins, SDK
35, NDK 27.2.12479018, FFmpeg n6.1.1, original ABI flags/10 Hz simulation and
fenced append-only unsynchronized geometry remain. APK 24 touch/audio fixes are
retained and Instant Action layout is unchanged. No new phone FPS, successful
retail J01 load, campaign completion or device combat-audio result is claimed.
The test identity/signer are reused, with private keys and owned data excluded.


APK 25 artifact: `Reigncarnated-Campaign-Path-v25.apk`, source
`7250200d75073ae0691bda0dbb5d5fc9259647af`, SHA-256
`41ed2e9c6c310922e6e0e35762e53f07d13c4f990854385c37964fd9b1fafb37`.
Independent checks pass: same APK 22-24 signer/identity, compiled manifest
version 25/private mission process, signature, CRC, 16 KiB ZIP/ELF alignment,
static libc++, exact embedded source/docs/native hashes, matching stripped/
unstripped mission build ID and exclusion of game data/keys/private traces.
The unchanged signed artifact is mirrored under builds/v0.19. Source and private
unstripped symbols are preserved; phone J01/combat acceptance remains outstanding.


## APK 26: cinematic rendering gap and current mission diagnostics

The owner confirms Instant Action combat sounds work on the phone. The combat
sound lifetime fix is therefore device verified at that scope, with remaining
individual audio assets unclaimed. The owner reports Sprawler intro black with
in-game cinematic sound, and JDA Chapter 1 followed by an error.

The new menu ZIP matches APK 25 source/native receipts; its nested mission ZIP
is an older APK 24 collection. The current journal reaches real simulation at
263 ms, then reports an invalid VarPtr at 3,271 ms after the latest JDA launch.
The empty-path stop is no longer recorded. There is no current throw-site stack
or Sprawler native/screenshot collection in the supplied ZIP. Historical crash
traces are retained separately and not attributed to the current error.

The original cinematic Letterbox issues sub-viewports and partial clears, which
the adapter previously rejected. Authored original-engine timeline and API
fixtures reproduce that gap. The adapter now stores viewport state, clips TL
screen coordinates with GLES scissoring and implements clipped color/depth
clears and multiple rectangles. Pixels outside the active clear region survive.
Invalid rectangles and unsupported operations remain explicit failures. Full
screen rendering keeps its existing transform/clear path; no per-draw GPU query,
readback, neutral lighting override or stream synchronization is introduced.

The exact JDA variable root cause remains unknown. Invalid VarPtr accesses now
capture bounded operation/module-relative callers before unwinding, without
changing object layouts or suppressing exceptions. The journal records original
cinematic command names/cycles. On frame failure and pause/exit, current native
mission reports save source/mission/timestamp and camera/cinematic/movie flags,
render/audio state and caller evidence. Menu export includes the last and
previous distinct mission report (128 KiB each), avoiding reliance on stale
manual mission ZIPs when testing both campaigns. Raw reports remain private.

Validation: all 68 host tests and 12 engine foundation tests pass under ASan/
UBSan with the established sandbox leak-detection setting. Real GLES pixels
verify nonzero viewport offsets, partial/multiple clears, preserved surrounding
pixels, TL clipping and full viewport restoration. Original Vid/GLES lifecycle
passes twice. The authored original cineractive checks nonblack scene and
letterbox/fade pixels, shroud/input/HUD restoration, timeline completion and
invalid-variable stack capture. Synthetic campaign and reference Instant Action
load/simulation/pause/resume/shutdown pass twice each. SDK 35 Java and 48 touch
checks pass. The 1,791-file pinned source check passes; changed original units,
foundation/renderer/adapters and both ARM64 native libraries rebuild and relink.

SDK 35, NDK 27.2.12479018, FFmpeg n6.1.1, original ABI flags/10 Hz simulation and
fenced append-only unsynchronized streaming remain. Touch controls, supported
High graphics and Instant Action layout are retained. The existing test signer/
identity retain imported data; keys, game packs, screenshots/traces and private
symbols stay outside public Git/APK. Sprawler appearance and JDA error resolution
still need phone testing; neither a retail campaign fix nor a new phone FPS
result is claimed. DIAGNOSTICS_REVIEW_V25.md records the sanitized evidence.


APK 26 artifact: `Reigncarnated-Cinematic-Viewport-v26.apk`, source
`7254c6418b7bdf80575bc8f9d39eb57634a92f38`, SHA-256
`1334c4cd8d09f38aee651c8c9abf85fa7142fa41e57f441fdfa53b86024ca0ab`.
Independent checks pass: same APK 22-25 signer/identity, compiled manifest
version 26/private mission process, signature/CRC, 16 KiB ZIP/ELF alignment,
static libc++, exact embedded source/docs/native hashes, matching private symbol
and stripped mission build IDs, and exclusion of game packs/keys/private traces.
The unchanged signed artifact is mirrored under builds/v0.20. Retail cinematic
appearance, JDA caller/root cause and Android automatic report lifecycle remain
phone acceptance checks; Instant Action combat audio is owner confirmed.


## APK 27: owner-selected placement and original campaign handoff

The owner reports Sprawler's opening camera now reaches the start, then pan
complains that it needs Standard camera; JDA still stops in Chapter 1; Instant
Action works. The private ZIP matches APK 26 receipts. Sprawler EndCineractive
runs at cycle 945, with cinematic/movie inactive by cycle 1401 but current camera
still cinematic. Original next-mission requests also had no registered desktop
outer-loop Mission/Shell consumer in this native runner.

Ending a cinematic now restores the original default camera if its cinematic
camera still owns the view. Pan is gated while cinematics/handoffs own the camera.
Original campaign Mission/Shell requests are queued to Android after a whole
engine frame; original selection, ordering, results/progress and difficulty
rules remain. Android saves the old state, closes it safely, then opens the
selected target or returns to menu. No mission name is hardcoded or skipped.

The owner explicitly chose tap-to-anchor, more single taps to rotate and double
tap to confirm. Rotation/confirmation use the saved cursor position with no
pointer movement on anchored taps. A 300 ms single-tap delay permits double-tap
confirmation with no extra rotation. Drag deliberately repositions; hold still
chooses a build command outside placement. Placement hold confirmation and
three-finger rotation are superseded. Normal left click/drag and two-finger pan
without zoom remain; controller work stays deferred.

JDA's report was displaced when the owner tested Sprawler and Instant Action,
so its exact invalid VarPtr caller remains unavailable. A separate bounded pair
of failed-mission reports now survives successful missions, retaining source,
time and caller evidence. No JDA root-cause fix is claimed. The supplied 2siege
report shows 350 cycles, approximately 120 FPS and audible mixer callbacks,
consistent with owner-confirmed Instant Action/combat sounds. This is APK 26
evidence, not a fresh APK 27 benchmark. Invalid UV/NaN draw rejections remain
explicit and unresolved in Sprawler diagnostics; no rendering validation is
weakened. Raw reports/traces and game data remain private.

Validation: 55 gesture checks, 13 failure-retention checks and SDK 35 Java pass;
12 engine foundation tests pass. Original authored cinematic tests lock pan
while cinematic and restore gameplay pan after it ends. A private two-map
campaign fixture runs original NextMission selection, deferred target delivery,
close/load/simulation of that target and final menu route twice. Reference
Instant Action also loads/simulates/pauses/resumes/closes twice. These are host
fixtures, not retail story completion or Android lifecycle/gesture acceptance.
The 1,791-file pinned source check passes, the changed original mission unit and
ARM64 mission adapters rebuild/relink, and verified unchanged original/menu/
renderer objects are retained. Source reproduction uses the clean full builder.

SDK 35, NDK 27.2.12479018, FFmpeg n6.1.1, original ABI flags/10 Hz simulation,
supported High graphics and fenced append-only unsynchronized stream remain.
The same private test signer/identity retain imported data. Every APK embeds
exact source/build/progress receipts; keys, owned packs, raw diagnostics and
private symbols stay outside public Git/APK. Retail handoff, controls, JDA's
caller and new phone FPS remain acceptance checks. See DIAGNOSTICS_REVIEW_V26.md.


APK 27 artifact: `Reigncarnated-Placement-Campaign-v27.apk`, source
`99efd4bfd00b6143ef7c9d9562cfb3c6d7313390`, SHA-256
`c0e76ac0c4e1e09c992a5fe5c3f5051e4c217d9d09d74a833b842d29a948a3af`.
Independent checks pass: same APK 22-26 signer/identity, version 27/private mission
process, signature/CRC, 16 KiB ZIP/ELF loads, static libc++, embedded source/docs/
native hashes, matching stripped/private symbol build IDs, new Java report class/
JNI exports, and exclusion of game packs/keys/private traces. The unchanged signed
artifact is mirrored under builds/v0.21. Retail campaign handoff, phone gestures
and the actual JDA caller remain acceptance work; source and private symbols are
preserved for the next diagnostic report.


## APK 28: retained JDA stack identifies missing mesh-validation bindings

The new private ZIP matches APK 27 receipts and retains J01's actual failure.
It reaches simulation cycle 30, then cinematic Mesh construction dereferences
VarInteger inside original Mesh::Manager::FindRead at meshread.cpp:3007. That
line reads checkMaxVerts/checkMaxTris for MeshRoot::Check. The native bootstrap
omitted the original vid.heap.check.verts/tris bindings. They are now created
with original flags and actual heap sizes after Heap::Init; the original mesh
and rendering validation remains active. This is a concrete fix for the captured
failure, rather than a generic catch or fallback.

A new optional integration fixture loads an uncached real text mesh through the
same original cinematic primitive, copying owned base-pack bytes only inside a
private temporary runtime. ASan first identifies mismatched new[]/scalar-delete
cleanup in this previously untested raw loader. Its text/vector/map/index arrays
now use matching delete[] in success and early-return paths; singly allocated
feature objects and ABI layouts are unchanged. With cleanup fixed and bindings
still absent, the test reproduces the device's invalid VarPtr. Binding the two
variables then passes the raw cinematic load/render/cleanup twice. Allocation
mismatch detection was kept enabled; only the established sandbox leak setting
is used.

The Sprawler report now shows Transition into S01, 118 simulation cycles in
about 12 seconds and 120 FPS at collection. This confirms the original intro
handoff progressed on the phone; full transition/gameplay is not established.
Remaining invalid-UV/NaN draw rejections are still explicit. Rejection context
now states whether texturing was active, to distinguish used/unused channels in
future diagnostics. No relaxation or claim of a UV fix is made.

Validation: real uncached mesh cinematic, camera restoration, original campaign
next/followup/final-menu lifecycle and cleanup pass twice; reference Instant
Action lifecycle passes twice. All 12 foundation tests, 55 retained gesture checks
and 13 report-retention checks pass. The full 1,791-file pinned source check
passes, the changed original ARM64 mesh unit and mission adapters rebuild/link,
and SDK 35 Java compiles during packaging. These authored cinematic/mesh fixtures
are not claimed to validate the retail J01 story or Android lifecycle.

Original 10 Hz simulation, ABI flags, SDK 35, NDK 27.2.12479018, FFmpeg n6.1.1,
supported High graphics, audio and fenced append-only unsynchronized stream remain.
The chosen tap-anchor/rotate/double-tap placement and protected failure records
are retained. Signing identity/data are retained, with keys, owned packs, raw
reports and private symbols excluded from Git/APK. Retest JDA Chapter 1 for phone
acceptance and any later issue; no new APK 28 phone FPS or complete campaign
result is claimed. DIAGNOSTICS_REVIEW_V27.md records sanitized evidence.


APK 28 artifact: `Reigncarnated-Cinematic-Mesh-v28.apk`, source
`69469fe5983f2d06ea91faf25db64cf6539532b5`, SHA-256
`73a8e7b01806d93e23afc7697eb0455dc38f501489ba9eb3d40393fd6b505dd7`.
Independent checks pass: same APK 22-27 signer/identity, version 28/private mission
process, signature/CRC, 16 KiB ZIP/ELF alignment, static libc++, exact embedded
source/docs/native hashes, matching private symbol/stripped build IDs, and
exclusion of game data/raw models/keys/private traces. The raw-mesh host log
confirms two uncached loads with no rejected draws. The unchanged signed APK is
mirrored under builds/v0.22. Retail JDA and later story acceptance remain for the
owner's phone; source and private symbols are preserved for any further report.

## APK 29: mission speech decoding and soundtrack arbitration

Owner feedback after APK 28: missions now load with cutscenes. The remaining
reported audio issue is missing Sprawler voices while music continues over the
scene. This is a device report of mission/cutscene progress; it does not establish
that either campaign is complete or provide a new phone performance benchmark.

The original Message and unit Response paths submit named Reserved voices.
Original Reserved supports MP3, but the recovered sample adapter sent every
resource to the strict PCM WAV decoder. It therefore rejected MP3 speech.
FFmpeg remains pinned to n6.1.1 with the same verified source archive; its minimal
LGPL build now also enables `mp3float`, `mpegaudio` parsing and `mp3` demuxing.
The new decoder reads resource memory only, requires one Layer III audio stream,
limits input and decoded stereo PCM to 64 MiB each, and validates rate, channels
and finite samples. PCM WAV keeps its existing strict validation. Compressed WAV
and other unsupported formats still fail explicitly with named diagnostics.
No downloaded or owned audio is used as a checked-in test fixture.

Reserved samples retain original queue modes, reset and lifetime behavior. They
now use protected priority in the shared mixer so ordinary combat effects cannot
steal their channel. Recent resource diagnostics include Reserved submissions,
starts, missing files, decoding failures and output suppression; decoder errors
include the resource name. The reports also show reserved-playing and the native
music block reasons, making missing imports distinguishable from decode failure.

Android music now starts suspended while loading and pauses at its current track
position during an original cinematic, Bink movie, reserved voice or campaign
handoff. It resumes after those conditions clear, subject to audio focus and the
configured music volume. Both asynchronous preparation and focus-gain callbacks
honor the same policy. Mission music updates are posted only on changes and tied
to the current player instance; old callbacks cannot change a replacement player.
The original 10 Hz simulation, engine ABI, touch controls, render stream,
retained textures and strict rendering checks are unchanged.

Validation: 13 engine-foundation tests pass, including authored MPEG-1 mono and
MPEG-2 stereo speech, audible stereo mixer output, completion, malformed data and
size bounds. An integration test against original Reserved code plays queued
MP3s while 64 FX channels are saturated, protects speech from higher-priority
combat FX, advances to the next dialogue, completes and resets, and records a
named missing resource. Original combat Effect lifetime regression passes.
The original cinematic/raw-mesh/camera and two-mission handoff fixture passes
twice under host ASan/UBSan. Touch 55, retained reports 13 and music policy 14
Java checks pass. Host root regression and final APK checks are recorded in the
matching delivery receipt. Android speech and music behavior need owner testing;
the complete retail campaign audio is not available in this workspace. This
build does not claim to supply missing voice files or fix remaining invalid UVs.

APK 29 artifact: `Reigncarnated-Cutscene-Audio-v29.apk`, source
`b2737565d21ec4529736a04b3a3ddc36261e0289`, 9,503,871 bytes, SHA-256
`4f76e3b9f45eaf53e9a417bfb8c37ea72d41e960a1d22e48ab833728c55716ba`.
The owner clarified that the missing Sprawler voices are spoken dialogue during
the opening cutscene. All 68 general host regressions, 13 engine tests, original
speech queue and combat lifetime integration tests passed. APK29 has the same
signer as APK22–28 and verified embedded source/docs, ARM64/static libc++ and
16 KiB ELF/ZIP alignment. It contains no keys, owned data or test audio fixtures.
Phone audio and FPS acceptance remain pending. Native build IDs and all check
results are in `docs/builds/cutscene-audio-v29.json`.

## APK30: shared Android options and offline release acceptance

The new owner ZIP identifies APK28 source `69469fe...`, not APK29. The saved
Instant Action `2siege` match reaches 6,570 simulation cycles, 119.9 FPS and
5.3 ms native frame time before opening desktop Video stops it with `Var is not
an integer`. An original-code host reproduction logs missing
`vid.dialog.detail.low` and fails in IFaceVar/ICButton activation. The previous
nested JDA failure has its own older APK27 receipt. See DIAGNOSTICS_REVIEW_V28.md;
no device logs, traces, captures or game data are committed.

Both the original main-menu Configuration and in-game Options/Escape commands
now route `Sys::Options` activation to the Android settings dialog. A scoped
host binding coalesces the request, returns it after native event processing and
leaves unrelated control activation and strict variable validation intact. The
Android dialog is the same audio/display/pan UI used by the toolbar, applies to
the active renderer/audio settings and synchronizes settings through the existing
mission Activity result. Repeated requests do not stack dialogs; pausing dismisses
the current dialog. The old desktop Gameplay/Video/Sound/SaveLoad tab dialog is
replaced, rather than presenting desktop display switching or unsupported saves
as functional. Android Back returns to the main menu.

A host capture with one real AI additionally exposes three ownership defects:
random-personality pointer-array deletion, non-virtual destruction of the private
polymorphic Strategic::Location Base/Offset implementations, and scalar deletion
of Placement::Manager::ClusterSet's ClusterInfo array. Native staging corrects
the exact ownership boundaries. Location's public pointer-only layout, existing
engine ABI/configuration, seeded random choice, simulation and rendering are
unchanged. ASan allocation/type mismatch checks stay enabled.

Validation includes 68 general host regressions, now 51 menu-runtime checks
including the original Configuration console command and coalesced host action.
The real mission Escape request is delivered once without activating desktop
controls. The campaign/cinematic/raw-mesh/camera/navigation fixture passes twice.
The owned reference Instant Action capture exercises one strategic AI, selection,
a movement order, map scrolling and normal shutdown; see the artifact receipt for
its final ASan result and the 30-second trailer receipt for exact capture data.
SDK35 Java and native ARM64 link checks pass. APK29 speech/music support, 10 Hz
simulation, flags, current touch gestures and append-only geometry streaming are
retained; no APK30 phone performance or complete-campaign claim is made.

`RELEASE_0_1_AND_MULTIPLAYER.md` records the offline release gates. The 31-row
`CAMPAIGN_ACCEPTANCE.csv` comes from package metadata, not inferred mission
completion. Continuous campaigns, local save/restore and persistence, opening
speech acceptance, match result/restart behavior, active-texture NaN UV draws,
long-session/device/thermal testing and distribution checks remain to close.
The owner reports that Instant Action appears completable; no attached victory
receipt establishes that as a controlled acceptance result yet.

The multiplayer review finds original orders, cycle checksums and StyxNet
client/server code worth preserving. Current sessions still dispatch locally;
Windows socket/event/thread boundaries, wire layout and determinism require a
real port and two-peer LAN proof before internet services or PC crossplay claims.
No transport or online service is enabled by this change.

The requested landscape trailer mockup is a 30-second H.264/AAC edit with real
host GLES gameplay and authored placeholder audio, clearly labeled work in
progress/host preview. Its source editor and capture path are reproducible;
it contains no raw game data or owner diagnostics. The reduced reference data
has visual limitations, so Android combat footage is the next editorial input.

APK30 artifact: `Reigncarnated-Android-Options-v30.apk`, source
`9d4bbe08c9fda6e8cd0a61f2b2e77cebd5e59bb2`, 9,516,159 bytes, SHA-256
`d1f6898176c40cd7908f151f050f17ce4e05bc0eb2f9e1d9b0ec593dd2206438`.
The real one-AI run captures 300 frames across 200 additional simulation cycles
and completes original shutdown with zero open data files and no ASan failure.
APK identity, signer, source/docs, private-data exclusion, ARM64/static libc++
and 16 KiB alignment pass independent verification. Receipt:
`docs/builds/android-options-v30.json`. Phone acceptance remains pending.

Trailer mockup 01: `Reigncarnated-Trailer-Mockup-01.mp4`, 1,810,753 bytes,
30.000-second container duration, 1280x720 H.264 / stereo AAC, SHA-256
`51c5e0616ea8b26f99c2d31bb7f8ecb9c9c6315ae890c180ec8a1d6226f34046`.
All video/audio decodes without error; source/capture/placeholder-audio receipt
and poster accompany the MP4 in `trailers/mockup-01/`. Raw private captures stay
outside the checkout. This is a draft edit, with no release announcement.

## APK31: all campaign entries and player-only starting resources

The owner requests main-menu access to every campaign mission and cutscene for
testing, with clear campaign-start versus individual-entry modes. The existing
catalog exposes only progress-unlocked campaign entries. A separate testing
catalog now enumerates all original `Campaign/Missions/Add` entries in declaration
order, without changing that progress-filtered API. It retains missing entries
with availability flags instead of skipping to another opening. Named intro,
outro and transition packs are labeled cutscenes; embedded cinematics still run
as part of their original mission scripts.

Main menu **Campaigns** offers **Start JDA campaign** and **Start Sprawler
campaign** (or the corresponding imported campaign names), plus **Choose individual
mission / cutscene (testing)**. A campaign start always uses its original first
entry and follows the original sequence. The individual picker includes later
missions and standalone cutscenes without win/unlock requirements, labels each
entry's faction and type, and disables launch for missing imported data. Its
explicit testing mode returns to the main menu when the entry requests a mission
transition. Tests use `mission-test-runtime` instead of `mission-runtime`, so
individual testing writes separate profile/progress data. Starting a campaign
continues to use the normal profile; it does not erase existing progress.

The owner chooses **player only** for the Instant Action resource override.
During native IA selection the **Resources** button opens an Android integer
editor: blank/map default preserves the map's stockpile, 0 is valid, and accepted
amounts range from 0 to 1,000,000. The setting persists for subsequent IA launches.
It is applied once to the local player's team after map/team loading, before the
first simulation cycle. AI stockpiles and all campaign resource amounts remain
map/script-defined. Native validation rejects invalid bounds and campaign resource
overrides before mutating the session. Diagnostics record mode, override and
current per-team stockpiles. The original simulation remains 10 Hz.

Validation: all 68 general host tests pass. The authored catalog fixture covers
all 31 declared campaign routes (19 mission entries and 12 standalone named
cutscene/transition entries), both original openings, order, missing-opening
handling and duplicate rejection. Java model/input tests pass 16 campaign and
16 resource checks; retained touch 55, reports 13 and music policy 14 pass. The
native session-boundary test covers resource validation and rejection without
replacing the active session. Real IA loading with one original AI is run twice
per amount: local 44,444 / AI 20,000, local 0 / AI 20,000, and map-default local
20,000 / AI 20,000, checked before the first simulation cycle. Each run closes
cleanly with ASan allocation/type checks enabled. The authored cinematic/raw-mesh/
camera and normal campaign Followup/final-menu route pass twice; individual mode
returns to menu instead of opening Followup, also twice. SDK35 Java and native
ARM64 links pass. Complete retail campaign playthroughs and these new menus on a
phone still require owner acceptance; catalog coverage is not mission completion.

APK29 speech/music, APK30 Android options/AI ownership repairs, original ABI flags,
retained textures, append-only geometry streaming and current touch controls are
preserved. No game data, key, device recording or raw diagnostic is included.

APK31 artifact: `Reigncarnated-Campaign-Testing-v31.apk`, source
`e47230ebcb62a75b9d269f87afaaeffdfb1f1cd2`, 9,532,543 bytes, SHA-256
`f1676ad16b9f0a4ed4cd4998f36ef24c73e0a4ed8eadd59929b4dca7752e5303`.
It has the same signer/application ID as APK22–30 and independently verified
embedded source/docs, ARM64/static libc++ and 16 KiB ELF/ZIP alignment. The native
campaign catalog entry point is exported and both current libraries are packaged.
Full validation, exact resource fixture values and acceptance limits are in
`docs/builds/campaign-testing-v31.json`. No private game data, keys, recordings or
raw diagnostics enter the source branch or APK.

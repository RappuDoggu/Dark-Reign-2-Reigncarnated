# Reproducible ARM64 recovery build

This is a functional reconstruction from checkpoint
`75e34b99c312238ca9367cb4abdcf579665f40f6` and the supplied conversation handoff,
not the lost v0.12 C++ working tree. Source is preserved on
`recovery/native-arm64-v012`. Each delivered APK includes its exact Git revision
and native-library hashes in `assets/build/source.json`, this document in
`assets/build/BUILD.md` and the progress record in `assets/build/PROGRESS.md`. Commit all source changes before packaging.

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
  -DREIGN_KHRONOS_HEADERS="$work/khronos" \
  -DREIGN_FFMPEG_ROOT="$work/ffmpeg/host/install"
cmake --build "$work/engine-host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  ctest --test-dir "$work/engine-host" --output-on-failure
mkdir -p "$work/java"
javac --release 8 -d "$work/java" \
  android/menu/org/reigncarnated/menu/{DataFolderImporter,ZipGameSource,FrameTiming,GameDataZip}.java \
  tests/{DataFolderImporterTest,ZipGameSourceTest,MusicImportTest,GameDataZipTest}.java
for test in DataFolderImporterTest ZipGameSourceTest MusicImportTest GameDataZipTest; do
  java -cp "$work/java" org.reigncarnated.menu.$test || exit 1
done
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk builds/v0.7/Reigncarnated-Performance-v07.apk \
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

## All-in-one device collection (version 15)

Choose **Collect diagnostics ZIP (all in one)** in the menu's Report / data
dialog or the mission's Debug tools. Keep that screen active while collection
samples two seconds in fast mode and two seconds in synchronized mode, requests
three timed frame captures per mode, retains both one-frame geometry audits and
native journals, and runs GPU checks afterward. The original stream mode is
restored on completion, failure, pause, or cancellation. Simulation and render
layers are left unchanged. The final dialog offers **Save ZIP** through Android's
document picker or **Share ZIP** with a temporary read grant.

Collect in the menu first to capture the pillar artifacts, then repeat in Instant
Action while flickering is visible. A collection includes the previous explicit
collection from the other screen up to 6 MiB; larger/missing prior collections
are noted in the ZIP. Save both ZIPs if needed. Only current/previous explicit
collections are combined, without recursively accumulating earlier ZIPs.

Payload: source receipt, bounded session history/tail and prior exit reasons,
device/GPU identification, per-mode native/callback timing summaries, native
render rejection arguments/audits, timed PNGs and GPU-check output. ZIP publication
is atomic and payloads are bounded to 16 MiB uncompressed. Whole game folders,
signing keys and unrelated app files are excluded. Readback stalls are explicitly
identified in reports; these callback timings are not GPU elapsed time or proof
of rendering fidelity. Sampling stops on lifecycle transitions. Android save/share
and visual correctness still require device validation.

Version 16 retains this collector and uses fence-protected physical buffer banks
in fast mode. The GPU-check report labels each rollover mode explicitly. After
updating, verify that fast rollover checks pass on the phone, inspect the pillars
and mission flickering, and compare normal fast FPS with the v15 report. A
synchronized comparison remains a diagnostic mode. Do not treat a host pass as
proof that the Adreno failure is resolved.

## Match menus and controls (version 17)

Update the recovery application normally to retain its imported data. Choose
**Instant Action** to select an imported map, practice or one original AI
opponent, an imported difficulty setting and a positive random seed. An AI game
requires a second playable starting team and imported personality mods; missing
prerequisites produce an explicit error. This is the original strategic AI,
not a replacement implementation. Additional lobby settings and more opponents
are not yet exposed.

Choose **Campaign missions**, or the original Campaigns button, for installed
campaigns in their original `Campaign/Missions/Add` order. A fresh local Commander
profile offers the first mission of each installed campaign; previous wins
unlock the next mission. The selector reads the native mission-runtime profile,
which is distinct from shell profile selection. Campaign launches keep the
mission's original player, scripted AI and teams. Closing a mission saves the
local profile before campaign globals are cleared. Full campaign completion,
cutscenes and end-of-mission navigation still require implementation/validation;
the selector is not a claim of complete campaign parity.

Two fingers pan; spreading pinches zooms in and closing pinches zooms out. Tap
**Controls** for zoom, direction/building rotation and arrow buttons. **Deselect**
clears selection through the original event. Single-finger drag retains the
original selection path. Use the all-in-one collector from both the menu and
mission to compare pillar appearance, performance and the actual AI/team report.

The full source builder above includes the changed session adapter automatically.
For an incremental rebuild from the verified v16 object set, recompile original
`multiplayer/multiplayer.cpp` (which includes `session_native.inc` and
`session_game_native.inc`), then supply that object report after the baseline
report to `link_mission_android.py` using a second `--objects` argument. Rebuild
both native libraries and repackage all Java together; the mission JNI launch
signature changed. Do not package a v17 Java/native mixture.

## Touch rotation and placement (version 18)

Twist two fingers to rotate the map; dragging them together pans. The gesture
locks to rotation or pan until lifted and never generates wheel/zoom input.
Use **Camera → Zoom +/−** separately. After selecting a building, ordinary
touches aim its preview; **Build ↶/↷** rotates and **Left click** places it at the
world cursor. **Deselect** cancels placement and clears selection. Normal taps
outside placement still left-click, and one-finger drags retain box selection.
Controller changes are deferred. The original 10 Hz simulation and fenced
streaming path remain unchanged. Original full menu flow and the reported attack
sound issue remain tracked separately.

Rebuild the mission library and all Java together for the new placement-status
JNI method. The menu native source is unchanged from v17. The full builder above
also produces this version. The host touch test has no Android dependencies:

```sh
javac --release 8 -d "$work/java" \
  android/menu/org/reigncarnated/menu/MissionTouch.java tests/MissionTouchTest.java
java -cp "$work/java" org.reigncarnated.menu.MissionTouchTest
```

Verify twisting in both directions at a fixed zoom, building preview rotation
before confirmation, quick cancel, ordinary selection and two-finger pan on the
device. A host gesture test cannot establish on-device camera/placement behavior.

## Pan, double tap and menu transition (version 19)

One finger aims; a single-finger double tap performs one original left click.
Two fingers pan only. Three-finger horizontal swipes rotate the selected building
preview in compass steps. Left click/Build arrows/Deselect remain available.
Touch zoom and map rotation are removed. Single-finger box dragging is replaced
by aiming.

The pan correction changes only the original Standard camera's horizontal scroll
target. The full builder stages the SHA-verified original source, adds a
non-virtual PanNative method and compiles it into `coregame/viewer_standard.cpp`.
An incremental build must recompile that original unit as well as retaining the
updated `multiplayer/multiplayer.cpp` from v17, then relink the mission adapter.
All engine fields, virtual methods, ABI flags and the 10 Hz simulation remain.

The main menu displays PackageManager's installed version name/code and binds
the same value to original `sys.buildver`. Both Instant Action buttons play the
imported Main::Exit bookmark before opening selection. Cancel/Back plays
Main::Enter before enabling the main controls. Missing bookmarks fail explicitly.
The timeline holds the last valid pose after a non-looping path completes.
The remaining full source setup interface/scripts and attack-sound work are
tracked in `INSTANT_ACTION_MENU_RESTORATION.md`.

Rebuild both native libraries and all Java together: the menu JNI open call now
also takes the APK version. Source/build/progress receipts stay embedded. Verify
the visible APK 19 label, repeated panning over varied terrain without zoom creep,
double-tap actions, three-finger building rotation, Instant Action entry and
Cancel/Back, and pause/resume on the phone.

## Startup notification correction (version 20)

Install the v20 recovery APK over v19 to retain imported data. Version 19's
`Expected one run code` startup failure came from updating the version display:
the whole `sys` scope inherited NOTIFY and sent the string update to the run-code
command handler. Command notifications now belong only to explicit commands;
the handler also verifies the command identity before reading console arguments.
The same scope correction applies to gamegod display/flow variables. Real
run-code commands still require exactly one argument.

The startup fixture now sets the APK label and opens the shell preview in the
same order as native State initialization. Both malformed command checks remain
enabled. Only the menu native library changes; the v19 mission native library,
touch bindings, camera pan correction, engine ABI and simulation are retained.
The packager recompiles Java and embeds current source/build/progress receipts.

## Single game-data ZIP and safe restore (version 21)

Install v21 over v20 with the same recovery identity/signer; keep app data.
Open **Report / data → Export complete game-data ZIP**, then **Save ZIP** and
choose a location. This creates `Reigncarnated-Complete-Game-Data.zip` privately
on the device. Select this ZIP alone for future **Select game data ZIP(s)**
imports. The archive contains game data, not the APK or source code.

The exporter combines the current imported runtime snapshot with the active
music snapshot, including separately imported music. It restores the canonical
`packs/base.zwp` and `missions/system/shell.zwp` paths. It validates Sprawl,
base/shell and JDA/Sprawler client packs, startup config, multiplayer group/maps,
mods and at least one supported music track before saving. All available runtime
folders are copied; missing campaigns, worlds, movies or tracks cannot be
reconstructed from an installation that lacks them. A missing required asset
causes a named error instead of an incomplete backup. Import missing music with
**Report / data → Import music ZIP**, then retry export.

A sorted file manifest records SHA-256 and byte counts. The APK source receipt
is included as `apk-source.json`. ZIP CRCs are reread before publication. Export
cancellation/failure preserves a previous completed cache file; the Save dialog
writes the checked ZIP to the chosen destination. Profiles, saves, diagnostics,
credentials and signing keys are excluded. Keep owned game archives private.
Allow space for the cached ZIP and its saved copy; restoring also needs a new
isolated installation snapshot while the previous import remains intact.

Data re-import now selects music from the new ZIP when supplied, otherwise keeps
valid existing music. Cleanup preserves its referenced snapshot. Mission startup
refreshes resource symlinks to the current import, including dangling links and
base/patch packs, so deleting an obsolete import cannot leave Sprawl/maps mounted
from the old directory. Preflight refuses to replace real runtime files/folders;
profiles are untouched. The filesystem regression test exercises actual restore
links after deleting the prior snapshot.

Rebuild/relink the mission adapter and recompile/package Java for v21. The menu
native library and original engine object selection remain v20/v19 respectively;
there is no original-engine unit, ABI, timing, renderer or gesture change. The
full builder above also includes the header-only resource-link helper. Host tests
verify fixture ZIP round trips and runtime filesystem behavior. Creating/saving
a ZIP through Android's document picker still requires phone validation.

## Native Instant Action city/setup screen (version 22)

Both Instant Action entry points now use the source Cineractive::Multi::Enter
objective. The imported bookmark name and CineractiveCompleted time are read
from objectives_cineractives.cfg. At that event the in-engine recovery setup
panel is activated while the city camera can continue moving. Back and Android
Back use Cineractive::Multi::Exit, with its own imported control-activation time.
Main::Exit remains reserved for Quit/Logout. Unsupported navigation instructions
fail explicitly; the code does not invent paths or wait for the whole camera
curve as a substitute for its control-activation event.

The panel uses original ICWindow/ICListBox/ICButton/ICEdit rendering, fonts,
theme and input routing. It exposes imported map groups/maps, practice or one
original AI, imported difficulty settings and a positive 32-bit seed. Launch
commits pending edit text before validation and sends all selected values to the
existing native mission session. Map and option preferences persist on launch.
The seed opens the numeric phone keyboard. Back closes it and returns to Main;
shortcuts are hidden during setup/transition, and input is gated until activation.
This replaces the fragile Android Instant Action dialog. It is a recovery panel,
not the complete original GameSetupBase multiplayer lobby: map preview, multiple
AI slots, alliances, per-team side/color, and advanced host options remain future
work. No controls are displayed that save an ignored setting.

The cloud reset removed the earlier private recovery signer. The delivered v22
candidate therefore uses a separate identity, org.reigncarnated.instantcity,
labelled Reigncarnated IA City Test. It installs alongside the previous Recovery
app and cannot read that app's private data. Import the PR #2 verified data ZIP
once into the new app; keep the existing installation. The new private key must
be backed up outside Git and reused for later updates of this test identity.
If the previous recovery key becomes available, the packager can instead use
--signing-key with --application-id org.reigncarnated.recovery for a compatible
update, after verifying that key matches the installed app's signer.

For a complete source build, follow the pinned prerequisite/full-builder steps
above, adding these identity arguments:

```sh
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk /private/path/to/verified-v07-or-v12.apk \
  --output "$work/v22" --jobs 4 --new-recovery-key \
  --application-id org.reigncarnated.instantcity \
  --app-label 'Reigncarnated IA City Test' \
  --output-name Reigncarnated-Instant-City-v22.apk
```

The application ID/provider authority and launcher label are included in the
build receipt. Neither a private key nor game assets enter the APK or source.
Restore a retained test signer to the same package output before rebuilding.
The original ABI defines/flags, native API 26, SDK 35, NDK 27.2.12479018, static
libc++, FFmpeg n6.1.1, 16 KiB alignment, 10 Hz simulation and fenced unsynchronized
geometry stream are unchanged. The mission engine was rebuilt from all 603 pinned
source units, not silently borrowed from the old APK.

Validation commands for the new behavior:

```sh
ASAN_OPTIONS=detect_leaks=0 "$work/host/menu_runtime_smoke"
ASAN_OPTIONS=detect_leaks=0 "$work/host/shell_menu_navigation_smoke"
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  "$work/host/instant_setup_gpu_probe" /private/extracted/installation /private/setup.ppm
```

The optional GPU probe needs real game data. It deliberately validates pixels
and controls only; audio decoding/playback is not part of this visual probe.
The upstream micro archive uses compressed audio unsupported by MenuAudio, so
its visual fixture does not replace validation of the full owned package or
change production audio behavior. The probe tests activation before camera
completion, real rendered controls, Launch/Back and identical pixels after GPU
context recreation. Phone touch feel, actual mission launch, Android pause/
resume and FPS still require device testing. No new FPS measurement is claimed.


## APK 23: original offline layout, Android options and High graphics

Build from the committed recovery source. The tracked v07 APK above is the
checksum-verified public packaging base; both native libraries and all Java are
rebuilt. Its old native performance is not reused. To update the APK 22 test app
and retain its imported data, reuse its private signer and explicit identity:

```sh
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk builds/v0.7/Reigncarnated-Performance-v07.apk \
  --output "$work/v23" --jobs 4 \
  --signing-key /private/path/to/apk22.keystore \
  --application-id org.reigncarnated.instantcity \
  --app-label "Reigncarnated IA City Test" \
  --output-name Reigncarnated-Original-Menus-Options-v23.apk
```

The Instant Action screen imports the original offline lobby's windows and
Map/Back/Launch icons. Map opens the grouped map chooser; Done refreshes the
original preview metadata/bitmap. Add/Remove AI, difficulty and seed feed the
native offline session. Multiple AI/team/alliance editing and advanced rule
controls remain pending and are marked on screen. See
INSTANT_ACTION_MENU_RESTORATION.md for the adaptation boundary.

The original Options button opens Android settings: music and effects volume,
map pan speed, display brightness, keep-awake and FPS overlay. The same settings
are available from the mission toolbar and persist across launches. Brightness
uses Android's display setting; it does not rewrite mission lighting or shroud.
Two-finger pan, three-finger building rotation and double-tap clicks are retained;
controller changes remain deferred.

Graphics use the original High performance factors, unreduced 32-bit textures,
full meshes, animation interpolation, multiweight, overlays and environment
mapping. The GLES adapter now supports point/linear mip filtering and lazily
retains derived mip chains; updating a texture invalidates its derived levels.
The bounded texture cache accounts for these levels. Geometry streaming remains
append-only, unsynchronized and protected by fences. The compatibility viewport
is still 640 x 480. MSAA, stencil reflections and single-pass multitexturing are
not implemented or advertised as enabled. This is maximum supported quality,
not a claim that every desktop graphics feature has been ported. Measure phone
FPS after updating; the new quality settings have no on-device benchmark yet.

Campaign launches take the original single-player branch, preserving default
client/scripted teams and the original campaign random seed. Enemy difficulty
uses the original campaign policy, including its default setting. A synthetic
campaign grouping of the reference map validates this branch's load, simulation,
pause/resume and cleanup; it does not validate the retail campaign missions,
cinematics or story transitions.

Loading status stays visible until simulation actually starts. Tap it, or choose
Debug while loading, to prepare a bounded ZIP with the last subsystem and stage
history without queueing work on the GL thread. A truly blocked native call may
still prevent Android's surface pause/document picker from completing; the ZIP
is written to the app's diagnostics cache before the save/share dialog opens.
After a process restart a menu collection can include that previous mission ZIP.
The application does not abandon or kill an active engine load from another
thread. Normal simulation keeps the original 10 Hz clock.

For the green fog/dark lighting report, collect the mission ZIP with the problem
visible. Reports include the imported runtime ambient/fog colors and shroud
state. Debug's **Compare distance fog (keeps fog of war)** toggles distance fog
only; collect again for comparison. It does not reveal hidden terrain or enemies.
The available reference map has deliberately colored lighting, so its appearance
cannot establish the correct lighting of the owner's affected map. No neutral
color override is applied. Campaign loading and affected-map visual correctness
remain device acceptance checks.


## APK 24: left-mouse touch, combat audio and campaign crash capture

Use the same prerequisite versions and pinned upstream verification above. This
full build recompiles all 603 selected original units with optimization and debug
information. The unstripped mission ELF stays in the external build directory for
private crash symbolication; only stripped libraries enter the APK. Preserve that
ELF alongside the build receipt. No signing key or owned game pack enters Git.

```sh
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk builds/v0.7/Reigncarnated-Performance-v07.apk \
  --output "$work/v24" --jobs 4 \
  --signing-key /private/path/to/apk22.keystore \
  --application-id org.reigncarnated.instantcity \
  --app-label "Reigncarnated IA City Test" \
  --output-name Reigncarnated-Touch-Combat-Audio-v24.apk
```

This updates the APK 22/23 test app and retains its imported data when signed
with the same private key. The menu reports version 24 through the embedded
receipt/Android package version. Original Instant Action layout changes are
deferred at the owner's request. Controller support remains deferred.

| Gesture | Result |
| --- | --- |
| One-finger tap | Original left click: select, issue orders, activate controls |
| One-finger drag | Hold original left mouse to select units |
| Stationary hold, 450 ms | One click on a build control or placement confirmation |
| Tap/drag while placing | Aim preview; hold to confirm |
| Two-finger drag | Pan only; no pinch zoom or twist |
| Three-finger tap while placing | Rotate building one clockwise step |
| Deselect / Build arrows | Cancel selection/placement; either rotation direction |

Hold never repeats. Moving cancels the hold for that contact. Three-finger
rotation requires a brief stationary tap, with no preceding pan. Multitouch,
cancellation and pause release any held mouse button. The original engine still
runs at 10 Hz; original ABI defines and optimized flags, maximum supported
quality and fenced append-only unsynchronized geometry streaming are unchanged.

Combat effects now detach destroyed Effect owners while their already-started
PCM finishes. This restores the original engine's lifetime policy; explicit stop
still stops looping sounds. Voice stealing requires a strictly lower priority.
AAudio accepts the negotiated stereo float sample rate, retries disconnected
output outside its callback, and reports stream/mixer/resource counters. The
original GameSound::Poll path runs once per engine frame. These fix a reproduced
premature sound cutoff and add observability; audible retail combat on Android
still requires the owner's check.

MissionActivity runs in a private :mission process to contain native faults.
Menu/mission settings transfer as typed snapshots on launch and normal return;
settings changed only in a mission that crashes may not return to menu settings.
The original campaign single-player loading branch remains. Stage boundaries
flush to private mission-runtime/loading-last.log before original subsystem calls.
This improves crash evidence; the retail campaign crash is not yet reproduced or
claimed fixed. Java compilation and host tests do not establish Android process
lifecycle correctness.

After a campaign crash, reopen/return to the main menu, then choose Report/data
and Collect diagnostics ZIP. The export includes last-mission-loading.log,
mission-process.log and up to three bounded crashes/*.trace files when Android
provides them (API 30+ historical exit information; native traces depend on OS).
The stage log also works on older supported devices. Collect a second report
with missing combat sounds after attacking/being hit: mixer and per-resource
request/start/missing/decode/suppression counts distinguish missing data from
output failure. These private reports remain outside the public source/APK.

Validation for this revision: 12 engine foundation tests; 150 Java checks,
including 48 touch checks; an original Effect/FileSys authored-WAV integration
fixture proving attack/impact survival after destruction and explicit loop stop;
AAudio API fixture for negotiated rate, disconnect/reopen and failure paths;
reference Instant Action and synthetic campaign load/simulation/pause/resume/
shutdown twice. The campaign fixture uses a reference map, not retail story data.
All 603 original ARM64 units rebuilt; original and adapter addresses resolve to
source lines in the private unstripped ELF. No phone FPS, retail campaign crash
fix or device audio result is claimed by these host checks.


## APK 25: campaign filename splitting and first-cycle readiness

Build with the same pinned toolchain and full-builder command from APK 24,
changing `--output` to `"$work/v25"` and `--output-name` to
`Reigncarnated-Campaign-Path-v25.apk`. Keep the APK 22/23/24 private signer and
`org.reigncarnated.instantcity` identity to retain imported data. Both native
libraries and Java are packaged from this committed source; no game assets or
signing key are included. Unchanged original engine units may be reused only
from the verified pinned object receipts. The current artifact reuses the 603
original units freshly compiled for APK 24, rebuilds the changed I/O foundation
and mission adapters, and relinks the mission library. The full builder remains
the clean reproduction path.

`Dir::PathExpand` now implements lexical `_splitpath` semantics: empty optional
asset names, drive components, case and separators are preserved without OS
lookup. Empty OS filenames and unmapped drive access remain failures. This fixes
a reproduced source of the uploaded J01 `Empty engine path` error, while the
precise retail caller and successful J01 gameplay still need phone testing.
See [DIAGNOSTICS_REVIEW_V24.md](DIAGNOSTICS_REVIEW_V24.md) for evidence and limits.

Readiness now waits for the first simulation cycle. Startup/frame exceptions
report FAILED and remain visible; Debug can export a failed-frame ZIP. Explicit
unsupported operation errors remain reported. The mission ELF has a GNU build
ID for matching platform traces to private unstripped symbols. Keep that ELF and
the receipt outside public Git; only stripped libraries enter the APK.

APK 24 touch controls and combat sound lifetime fix are retained. Instant Action
layout is unchanged. The original 10 Hz simulation, ABI configuration, supported
High graphics and fenced append-only unsynchronized stream are unchanged. No
new phone FPS measurement is available.

Validation: 12 engine foundation tests (including lexical path splitting and
failure after readiness), original bitmap-manager optional-name regression and
two-pass reference Instant Action/synthetic campaign lifecycle checks under
ASan/UBSan. The real mission integration loop rejects READY with zero simulation
cycles. SDK 35 compiles all Android Java; the 48 touch state checks also pass.
These host checks do not substitute for J01/campaign completion or Android audio.
After J01 testing, collect diagnostics again; if gameplay runs, collect after
attacking and receiving damage to exercise actual sound requests.


## APK 26: cinematic viewports and current mission evidence

Use the full builder command documented for APK 24, with output `"$work/v26"`
and `--output-name Reigncarnated-Cinematic-Viewport-v26.apk`. Preserve the existing
private APK 22-25 signer and `org.reigncarnated.instantcity` application ID. This
updates that test app while retaining its data. All Java and both native
libraries are rebuilt from this committed source; the current incremental
artifact reuses verified original ARM64 objects with explicit rebuilt
util/varptr.cpp and coregame/viewer_action.cpp overrides. The clean full builder
rebuilds every selected original unit and verifies upstream hashes first.

The cinematic Letterbox path needs sub-viewports and partial clears. These are
now real GLES scissor operations: pretransformed TL coordinates remain absolute,
color/depth clears intersect the active viewport, and pixels outside it survive.
GetViewport returns stored state. Multiple clear rectangles are supported up to
256 per call; invalid rectangles, stencil clears and nonstandard depth ranges
remain explicit failures. No unsupported draw path is silently ignored. Normal
full-target rendering and its fast clear/stream path are retained.

The JDA report now reaches simulation, then fails on an invalid VarPtr about
three seconds later. Its actual variable caller was not captured. This build
records the operation and bounded module-relative caller stack before unwinding,
plus the last original cinematic command/cycle. It preserves the original error
and ABI layout. This is evidence gathering, not a confirmed JDA error fix.

Native reports are saved on frame failure and mission pause/exit, with source
receipt, mission and timestamp. Collecting from the menu includes
last-mission-report.txt and previous-mission-report.txt (each capped at 128 KiB),
covering the two most recently saved distinct missions. These reports include
camera/cinematic/movie flags and audio/render diagnostics. The earlier nested
manual ZIP remains separately identified by its own source receipt.

Test Sprawler's intro, return to menu, then test JDA Chapter 1. Return and collect
one menu diagnostics ZIP if either remains wrong; the two automatic reports
should preserve both sessions. A load that blocks the GL thread cannot save a
native snapshot; persistent loader stages and available Android crash traces
remain the fallback. Diagnostics stay private and are not packaged or committed.

The owner's Instant Action combat sounds are device confirmed. Current touch
controls and menu layout remain unchanged. The original 10 Hz clock, ABI flags,
supported High graphics and fenced append-only unsynchronized stream remain.
No new device cinematic/FPS result is claimed. See DIAGNOSTICS_REVIEW_V25.md for
evidence and limits. Validation includes 68 host tests, 12 foundation tests,
real viewport/clear/clipping pixels, two-pass original cinematic/reference
mission lifecycles, SDK 35 Java compilation and 48 touch checks.


## APK 27: anchored placement and deferred campaign handoff

Use the full builder command from APK 24, changing output to `"$work/v27"` and
output name to `Reigncarnated-Placement-Campaign-v27.apk`. Retain the APK 22-26
private signer and `org.reigncarnated.instantcity` identity. Both native libraries
and all Java are packaged with exact source/native receipts; existing game data
is retained. The current incremental artifact reuses the unchanged verified APK
26 menu/renderer, original APK 24 objects and APK 26 overrides, and rebuilds the
original game/missions.cpp plus mission adapters. The full builder verifies pins
and cleanly recompiles all selected units; no game assets or keys enter Git/APK.

The owner selected these placement controls: first tap anchors, later single taps
rotate in place, double-tap confirms. Single rotations wait 300 ms to distinguish
double tap; double tap confirms at the anchor with no extra rotation. Drag
repositions. Hold still chooses a building outside placement, but no longer
confirms while placing. Three-finger rotation is replaced by this scheme.
Normal single left-click, drag selection and two-finger pan remain. See
CONTROLS_BACKLOG.md for current behavior and the older gesture history.

An original cinematic that ends while still using its cinematic camera now
returns to the original default camera, inheriting its view. Pan gestures are
gated while a cinematic/transition owns the camera. Original campaign Mission/
Shell requests are delivered after the current frame returns, using original
NextMission selection and campaign ordering. Android then snapshots/closes the
old runtime and opens the requested target, or returns to the menu. No resource
teardown occurs inside a script/simulation tick. No hardcoded intro skip is used.

Reports now preserve last-mission-failure.txt and previous-mission-failure.txt
separately from successful mission snapshots, 128 KiB each, with mission/source/
time receipts. Successful Instant Action cannot replace JDA's failure evidence.
The actual JDA invalid-variable caller was lost from the supplied two-slot APK
26 report ring; **JDA is not claimed fixed**. Test Sprawler, JDA and controls,
then collect a new menu ZIP if needed. DIAGNOSTICS_REVIEW_V26.md records the
current evidence, including unresolved invalid UV draw diagnostics.

Validation: 55 touch checks, 13 retention checks, SDK 35 Java compilation and
12 foundation tests pass. Authored original cinematic camera/pan restoration
and a private two-map campaign fixture pass twice: original NextMission target,
safe close/reload, simulation, final menu route and pause/resume/cleanup. The
reference Instant Action lifecycle also passes twice. Original 10 Hz/ABI flags,
supported High graphics and fenced unsynchronized streaming remain. This does
not establish retail campaign completion, Android handoff lifecycle, phone
placement feel or a new APK 27 FPS benchmark.


## APK 28: JDA cinematic raw-mesh startup fix

Use the same pinned full-builder command from APK 24, changing output to
`"$work/v28"` and output name to `Reigncarnated-Cinematic-Mesh-v28.apk`. Reuse the
APK 22-27 private signer and `org.reigncarnated.instantcity` identity to update
without reimporting data. The current incremental artifact rebuilds the original
meshread.cpp and mission adapters atop verified original objects/overrides; the
unchanged menu remains byte-identical. The clean full builder verifies pins and
recompiles everything. No owned game data, raw diagnostics or key is packaged.

The retained JDA stack identifies the original cinematic text-mesh check reading
unbound checkMaxVerts/checkMaxTris VarIntegers. Bootstrap now creates the original
vid.heap.check.verts/tris variables after Heap::Init, with the current real heap
sizes and original notification flags. MeshRoot::Check and all rendering checks
remain active. The raw loader's new[] buffers now use matching delete[] cleanup,
including early-return paths. No object layouts or ABI configuration change.

To exercise the new regression, use the existing original engine-app host test
with an owned base pack containing engine_plane.xsi and set
REIGN_CINEMATIC_FIXTURE=1 REIGN_MESH_CINEMATIC_FIXTURE=1. The fixture copies its
bytes only to the private temporary mission runtime and invokes the original
cinematic Mesh primitive; it does not publish the asset. For the two-map deferred
navigation fixture also set REIGN_NAVIGATION_FIXTURE=1, using the private two-map
campaign group described for APK 27. Source-side reproduction first exposes
array cleanup mismatches, then the original missing binding; both corrections
are required for the passing raw-mesh cinematic path.

Validation: real uncached cinematic text-mesh load/render/cleanup twice, original
camera/pan restoration, next-mission/final-menu routing, reference Instant Action
lifecycle twice, 12 foundation tests, 55 gesture checks and 13 report checks. SDK
35 Java compiles during packaging. Sprawler's APK 27 report confirms entry into
Transition into S01 on the phone, with 120 FPS at that captured point. JDA's
specific failure is source diagnosed and fixed; full retail intro/campaign and
APK 28 phone performance remain acceptance checks.

Rejected-draw reports now include textureActive to investigate remaining bad UVs;
validation remains strict. Placement controls, audio, 10 Hz clock, original flags,
supported High graphics and fenced append-only unsynchronized stream are retained.
Old protected failures keep their own source/time receipt; do not attribute an
older APK's saved failure to a later successful session. See DIAGNOSTICS_REVIEW_V27.md.

## APK 29: MP3 mission speech and music suspension

Use the same SDK 35 / NDK 27.2.12479018 / API 26 / ARM64 / static libc++ recipe.
Rebuild **both** FFmpeg installs with the current script even if older Bink-only
libraries exist. The n6.1.1 source hash is unchanged. The recipe now enables
`mp3float`, the `mpegaudio` parser and `mp3` demuxer as well as Bink. Network and
programs remain disabled. The install contains `reign-codec-recipe.sha256`;
`setup_local.sh` refreshes stale installs, and the full candidate builder rejects
an outdated recipe before compiling original sources.

```sh
bash tools/build_bink_decoder.sh "$work/ffmpeg" host
bash tools/build_bink_decoder.sh "$work/ffmpeg" android
# Include REIGN_FFMPEG_ROOT in the engine host configuration for speech tests.
cmake -S engine -B "$work/engine-host" -G Ninja \
  -DREIGN_UPSTREAM="$work/upstream" -DREIGN_SANITIZE=ON \
  -DREIGN_KHRONOS_HEADERS="$work/khronos" \
  -DREIGN_FFMPEG_ROOT="$work/ffmpeg/host/install"
cmake --build "$work/engine-host" --parallel 4
ASAN_OPTIONS=detect_leaks=0 LIBGL_ALWAYS_SOFTWARE=1 \
  ctest --test-dir "$work/engine-host" --output-on-failure

# From a committed, clean source revision, using the private APK22 test signer:
python3 tools/build_performance_candidate.py \
  --upstream "$work/upstream" --sdk "$work/sdk" \
  --ffmpeg "$work/ffmpeg/android/install" \
  --base-apk builds/v0.7/Reigncarnated-Performance-v07.apk \
  --output "$work/v29" --output-name Reigncarnated-Cutscene-Audio-v29.apk \
  --signing-key /private/path/to/recovery-debug.keystore \
  --application-id org.reigncarnated.instantcity \
  --app-label 'Reigncarnated IA City Test' --jobs 4
```

The incremental recovery build uses the APK24 full object baseline, overrides
from APK26/27/28 and freshly staged `sound/sound_digital.cpp` and
`sound/sound_digital_reserved.cpp`. Rebuild the engine audio foundation, menu
library and all direct mission adapters. The full builder above compiles the
complete inventory from the current staging recipe without cached overrides.
Keep unstripped native libraries, game data and signing material private.

`tests/fixtures/speech-*.mp3` are authored half-second tones, excluded from the
APK. `engine_speech_decode_test` validates both sample rates, mono/stereo,
audible decoded output, completion and invalid input. Link
`engine_reserved_speech_test.cpp` with `tools/test_engine_vid.py`, using the
original host objects and `--test-arg "$PWD/tests/fixtures"`, to test the original
queue under combat channel saturation. Retain the combat lifetime test and the
cinematic/raw-mesh/navigation fixture from APK28. Compile and run
`MusicPlaybackPolicyTest.java` with `MusicPlaybackPolicy.java` for asynchronous
preparation/focus/cinematic/voice/movie/loading/close policy transitions.

On the phone, update the existing **IA City Test** installation. Replay the
Sprawler opening: check spoken dialogue, quiet background soundtrack throughout
the scene, and music resuming afterward. Check normal unit responses and combat
sounds. If a voice is still absent, use **Debug → Collect diagnostics ZIP (all
in one)** after the scene. Its resource rows now distinguish requested, started,
missing, decode-failed and suppressed speech. Music block bits are cinematic=1,
movie=2, reserved voice=4, loading/transition=8; multiple reasons combine.

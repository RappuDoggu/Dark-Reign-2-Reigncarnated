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
  -DREIGN_KHRONOS_HEADERS="$work/khronos"
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

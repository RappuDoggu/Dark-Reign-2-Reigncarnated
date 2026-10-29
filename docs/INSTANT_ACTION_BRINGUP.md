# Instant Action full-engine bring-up — 2026-10-28

Target: original Instant Action mission, one human player, no opponent; touch
selection of a real unit and construction of a real structure. The v0.5 launch
APK remains the working baseline; the user has confirmed its intro/menu on device.
No new mission APK is delivered by this checkpoint.

Local Windows/WSL continuation: [step-by-step setup](LOCAL_WINDOWS_SETUP.md).

## Shared ABI foundation checkpoint

`engine/CMakeLists.txt` now provides a separate build tree for original global
engine definitions. `prepare_engine_abi.py` verifies source hashes and stages
complete original headers, changing U32/S32 to fixed-width types, supplying
U64/S64 on native targets and mapping calling-convention declarations. It does
not vend upstream code into this repository or implement missing Win32 APIs.
The existing namespaced launch slices remain isolated from this future engine
target; these are not two interchangeable class layouts.

Host ASan/UBSan execution validates the original 56-byte PackInfo layout,
intrusive list/tree pointers with nonzero node offsets and high-bit IDs, and
fixed expected original random-generator states. Compiling against the unmodified
original definitions fails the ABI assertions as expected. The same test links
as an Android ARM64 ELF; device execution has not occurred. See
`logs/instant-action/engine-abi.log`.

This removes a foundational width issue for the new target. It does **not**
mean the 705-source engine compiles: the Windows/DirectX services, mission
runtime and original selection/construction integration below are still needed.

## Verified starting point

Original source: grasmanek94/darkreign2 at
`9ce9da5f21604310a997f0c41e9cd383f5e292c3`. This is source-port work, not a need to
decompile the gameplay engine. Binary investigation is reserved for an observed
source/retail discrepancy or missing behavior.

`tools/probe_full_engine.py` walks the original appdr2 project references:
18 projects and 705 declared translation units. Eight representative complete
files were syntax-probed with NDK 27.2, ARM64 API 26. All stop at the shared
`system/win32.h` dependency on windows.h. This is a first-blocker probe, not a
claim to have enumerated every compiler or linker error. Current slice targets
are not a complete engine build. See logs/instant-action/full-engine.json.

`system/defines.h` defines U32/S32 using unsigned/signed long. These must remain
32 bits throughout the full build on Android LP64. The existing slice preludes
solve this for slices, not automatically for every original translation unit.

The recovered owned installation enumerates 60 missions in 9 groups. New
`instant_action_data_probe` uses the existing original mission catalog, parser
and terrain-loader boundary on stock `2crazed`: 100x100, Dome terrain group,
125 object definitions, 10100 terrain cells including pitch padding, two map
team definitions and one default client. Definitions are not instantiated units.
The probe does not resolve all object types or textures, run simulation, or
implement local-player setup. See logs/instant-action/map-preflight.log.

Reproduce with:

```
python3 tools/probe_full_engine.py /path/to/upstream \
  --compiler /path/to/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++ \
  --output /path/to/probe-results
cmake --build /path/to/host-build --target instant_action_data_probe
ASAN_OPTIONS=detect_leaks=0 /path/to/host-build/instant_action_data_probe /path/to/owned-installation
```

The syntax probe intentionally exits nonzero while compilation is blocked.
The map probe's optional second argument selects another registered multiplayer
mission. 2crazed is a repeatable proof fixture, not the desktop's unique default:
`interface_setup_instant.cfg` executes setrandommission then addai.

## Required integration, in dependency order

1. Central platform/type layer and whole-file build: preserve original class
   layouts, fixed-width data formats and gameplay logic; implement actual OS
   services instead of scattering success-returning stubs. Separate optional
   online services from local session setup.
2. Resource/type registration and full mission lifecycle:
   `Game::PreOpenMission` initializes teams, configuration, task controller,
   object types, footprint, pathfinding, physics, movement and effects;
   `Game::OpenMission` calls `CoreGame::OpenMission`, world-object loading,
   team/player post-load and `Team::CreateSideBasedUnits`.
3. Local Instant Action session: preserve the local player/team and ruleset
   machinery in multiplayer_cmd.cpp/multiplayer_host.cpp. Do not remove the
   entire multiplayer library just because online play is out of scope.
   Omit addai, retain the human slot, purge unused playable teams through the
   original flow, and verify protecthq does not terminate an empty-opponent test
   immediately. No-opponent behavior is not yet runtime-tested.
4. Original simulation cadence, object updates and gameplay renderer connection.
   Current shell rendering is not this simulation or the complete renderer.
5. Touch: screen-to-world/object picking, selection through ClearSelected and
   AddSelected orders, construction palette/placement routed through
   Orders::Game::Build::Generate. Its Execute resolves the type and issues
   Tasks::UnitBuild; footprint validation, movement, resources and construction
   must remain engine-owned. Do not substitute a visual building spawn.

Acceptance: launch from Instant Action, remain in the no-opponent mission,
select a starting unit by touch, choose and place a permitted structure, see
the original build task finish and resources update. Pause/resume must not
issue a stale touch command. Map data parsing alone does not satisfy this.

## Renderer decision

DR2's default vid_decl.h uses IDirectDraw7/IDirect3DDevice7 (with an older DX6
conditional). Generals' d3d8gles cannot simply be linked into this engine.
DXVK documents Direct3D 8/9/10/11, not D3D7. DxWrapper's dd7to9 is a candidate
for preserving the original renderer, but its Windows-targeted integration
must be evaluated for native ARM64/Android before committing to that route.
No translator has been compiled or validated here yet; no speed claim is made.

Prefer preserving original mesh/terrain/bucket processing and adapting the
device boundary over extending shell_scene.cpp into a replacement gameplay
engine. Compare a D3D7-compatible translator's dependencies against a native
Vid/backend adapter using actual build evidence before choosing. Existing GLES
upload/presentation code remains a useful fallback, not a reason to recreate
all high-level effects.

References inspected:
- https://github.com/MYSOREZ/GeneralsZH-Android-Port/blob/main/docs/port/PORTING_PATTERNS.md
- https://github.com/MYSOREZ/GeneralsZH-Android-Port/blob/main/Core/Libraries/Source/d3d8gles/CMakeLists.txt
- https://github.com/imbaner/fallout2-ce-mobile/blob/main/src/player_commands.h
- https://github.com/imbaner/fallout2-ce-mobile/blob/main/os/android/app/src/main/java/com/alexbatalov/fallout2ce/GameImport.java
- https://github.com/doitsujin/dxvk/blob/master/README.md
- https://github.com/elishacloud/dxwrapper/blob/master/README.md

Generals motivates preserving the engine and adapting subsystem boundaries;
Fallout motivates input-independent commands and robust import. Neither is
evidence that their renderer or gameplay code is directly reusable in DR2.

## Native resource and mission-manager checkpoint (2026-10-28)

The separate `engine/` build now compiles complete original translation units
for FileSys (directory, pack and stream sources), DTrack, strings, PTree, FScope,
VNode, StdParse, TBuf, BlockFile, Missions, Campaigns, Difficulty, StdLoad,
Stats, utility types, RunCodes and Random. Global original class names are
preserved. This target does not link the menu's replacement FileSys or its
extracted mission-catalog slice.

The native File/Dir implementation preserves the public original interface,
uses POSIX descriptors/mappings, resolves Windows separators and ASCII case,
and keeps engine-generated backslash paths unchanged for CRC identifiers.
Mappings retain pointer-sized ownership and validate bounds. Read/write counts
retain the original byte-count semantics. Unsupported DOS drive discovery
returns failure; it does not invent a CD drive.

Validation with the owned installation:

- Native File/Dir regression tests: mapping offset and EOF handling, write maps,
  append, overflow rejection, case-insensitive lookup, directory enumeration,
  file CRC, missing files and path-identifier spelling.
- All 5 entries in `2crazed.zwp` and all 2,074 entries in `packs/base.zwp` mapped
  and compared byte-for-byte with sequential reads.
- Original FileSys/PTree/BlockFile: mission and base streams, clipped reads,
  absolute native paths, 100x100 mission config, 125 object definitions,
  terrain block lookup, unload and no remaining open DataFiles.
- Original Missions: discover 60 missions in 9 groups; select `2crazed` from
  the Instant Action group; SetActive/OpenActiveStream; parse the active
  game.cfg; LaunchMission queues the original Mission run-code; unload.
- Five host CTest checks pass with ASan/UBSan, with LeakSanitizer disabled
  because this execution environment cannot enumerate process threads.
- All targets compile and link for Android ARM64/API 26 using NDK 27.2.12479018.
  ARM64 execution on a device has not been performed.

Important distinction: the mission-start test registers a null Mission callback
solely to inspect the pending run-code request. It never calls RunCodes::Process
and cannot run a fake mission. Game::PreOpenMission/OpenMission, players, teams,
objects, simulation and rendering remain unlinked. No new APK is produced.

The test executable uses function-section garbage collection to omit uncalled
save/UI/full-game entry points. This is not a claim that every function in those
libraries has all its dependencies ported. In particular, original
StdLoad::GetIdentAsU32 originally stored a heap pointer in a 32-bit ID. The
native implementation now returns an owning-registry token; the full-source
Resolver::Type adapter consumes that token by value. Object instance IDs remain
32-bit. Tokens are not recycled, and missing/consumed tokens fail explicitly.
The original loader is exercised by the native runtime test. This does not yet
establish end-to-end GameObjType creation in a running mission.
Original Matrix retains its 64-byte row layout; its unused, nonstandard
anonymous SIMD `Mat` alias is removed. The original Windows-only RECT conversion
is excluded on native builds.

Fixed while exercising the real resource path: DataFilePack::Read copied the
requested byte count after calculating a shorter remaining count. It now copies
only the clipped count. The regression test verifies bytes beyond the returned
count remain untouched. The directory index now unlinks its null-valued entries
instead of instantiating delete-void. CRC string indexing accepts high bytes
without indexing before its table. Existing reviewed parser fixes are reused.

Run locally after the normal setup:

```sh
cmake -S "$REIGN_REPO/engine" -B "$REIGN_WORK_ROOT/build-engine-host" -G Ninja \
  -DREIGN_UPSTREAM="$REIGN_UPSTREAM" -DREIGN_SANITIZE=ON \
  -DREIGN_GAME_DATA="$REIGN_WORK_ROOT/owned/Dark Reign 2"
cmake --build "$REIGN_WORK_ROOT/build-engine-host"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$REIGN_WORK_ROOT/build-engine-host" --output-on-failure
```

Next runtime boundary: compile/link the original Game initialization and its
object/type, world, team/player and task dependencies, and attach the actual
Mission handler. The pending-run-code test must not be relabeled a mission boot.

OpenDR review: its OpenRA-based DR1 recreation reinforces reuse of an established
simulation and separation of input orders from execution. Its sprite terrain
renderer is not a DR2 3D backend, and its map importer contains incomplete actor
conversion. It does not change the native DR2 target.

- https://github.com/drogoganor/OpenDR/blob/master/README.md
- https://github.com/drogoganor/OpenDR/blob/master/OpenRA.Mods.Dr/Orders/BuilderUnitBuildingOrderGenerator.cs
- https://github.com/drogoganor/OpenDR/blob/master/OpenRA.Mods.Dr/UtilityCommands/ImportDrMapCommand.cs

D7VK was also inspected: its `devel` branch's `src/ddraw/ddraw_main.cpp` loads an
underlying Windows/Wine ddraw library and forwards DirectDrawCreateEx. It is not
a standalone native Android renderer that can simply replace DR2's device layer.
No D7VK build was attempted.


## Complete gameplay compilation and mission link gate (2026-10-28)

The full-source adapter now compiles all 255 translation units from the original
`coregame`, `coregame_objects`, `coregame_tasks`, `coregame_orders` and `game`
projects on native Linux and Android ARM64/API 26. These are complete translation
units, including original mission initialization, unit construction tasks and
orders. This is object compilation, not executable linking or gameplay testing.

The broader 705-source app-project inventory produced 556 successful ARM64 object
files before the final gameplay fixes. That inventory includes unused Windows
application/network/editor code, so its success fraction is not a completion
estimate. A mission-entry link probe combining successful broader objects with
the final gameplay objects reaches the linker but reports 336 unresolved symbols.
See `logs/instant-action/native-mission-link.json` for the exact dependency list.
No mission executable, APK, or device run is claimed.

New native adaptations:

- Original DX7 declarations use fixed-width Windows data types without defining
  `_WIN32`. They declare the device interfaces; they do not implement a renderer.
- Standard C++ dependent-name lookup, state-machine member pointers, complete-type
  lookup, and explicit specializations replace MSVC-only acceptance rules.
- The runtime-only bookmark union uses separate fields, while individual field
  serialization stays unchanged. Animation frame setters now return their value.
- Object type-name resolution uses the checked owning registry described above.
- Dynamic objective/radio message strings are native UTF-16 pointers passed through
  the native variadic ABI, replacing manual x86 pushes/pops and U32 pointer casts.
- Original queue mutexes and semaphores use real process-local C++ synchronization.
  Unsupported named kernel objects throw; no success stubs are supplied.
- Debug stack collection uses native unwinding rather than x86 EBP inspection.
- The existing aligned allocator now preserves the original class-shaped API.

The five ASan/UBSan host tests pass, including original-loader token creation,
consumption/stale-token rejection, concurrent registry ownership, synchronization
and unwinding. Foundation tests also compile/link for ARM64. LeakSanitizer remains
disabled for the execution environment restriction described above. No ARM64
binary was executed here.

Reproduce complete gameplay compilation after the foundation configuration:

```sh
python3 "$REIGN_REPO/tools/prepare_engine_full.py" "$REIGN_UPSTREAM" "$REIGN_WORK_ROOT/upstream-native"
python3 "$REIGN_REPO/tools/build_engine_gameplay.py"   "$REIGN_UPSTREAM" "$REIGN_WORK_ROOT/upstream-native" "$REIGN_WORK_ROOT/build-gameplay-host"
python3 "$REIGN_REPO/tools/build_engine_gameplay.py"   "$REIGN_UPSTREAM" "$REIGN_WORK_ROOT/upstream-native" "$REIGN_WORK_ROOT/build-gameplay-android"   --compiler "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
```

The staging script validates all 1,791 original source/header inputs against the
pinned tree digest and regenerates its reviewed prerequisite overlays. Build and
staging scripts use a shared/exclusive lock to prevent concurrent source rewrites.
Failed compiles remove stale objects. Cached successes require an identical input
fingerprint. The scripts target Linux/WSL and the Linux NDK host toolchain.

For the broader dependency inventory, add `--all-projects` and use a separate
output directory. To run the link gate:

```sh
python3 "$REIGN_REPO/tools/link_engine_mission_probe.py"   "$REIGN_WORK_ROOT/upstream-native" "$REIGN_WORK_ROOT/build-engine-android"   "$REIGN_WORK_ROOT/build-mission-link-android"   --compiler "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"   --objects "$REIGN_WORK_ROOT/build-engine-all-android"   --objects "$REIGN_WORK_ROOT/build-gameplay-android"
```

The probe retains real Game mission entry points for linking. Its main function
explicitly reports that it is a link check and exits with failure; it is not a
bootstrap or a substitute mission runner.

Remaining release gate: supply the real native application/renderer/input/audio
connections needed by those entry points, execute original world/team/player/
object creation, advance simulation, then wire main-menu Instant Action and test
on Android. The existing v0.5 main-menu APK remains unchanged.


## UI, graphics dependencies and native timing (next recompilation pass)

The mission-entry link gate now reports **155 unresolved symbols**, down from
336. This pass resolved 217 previous symbols and exposed 36 new dependencies as
additional implementations became reachable. There are no other linker errors
in this recorded attempt. The executable still does not link or run.

See `logs/instant-action/native-mission-link-phase2.json` for the remaining,
resolved and newly exposed symbols. The preceding 336-symbol report is retained
as the baseline, rather than being silently replaced.

The selected `interface`, `coregame_interface` and `graphics` projects now compile
189 of 205 complete translation units for ARM64. Newly available implementations
include UI controls, interface initialization, terrain, mesh management and
several rendering commands. This does not implement the missing native graphics
device, bitmap allocation/video backend, or input device connection.

Changes include:

- Original DirectInput scan-code constants are staged without importing a
  Windows input device implementation.
- Integral color overloads, explicit VarInteger conversions and legacy C++
  syntax fixes allow controls and render commands to compile.
- Mesh lexer iteration and its end-token lookup use the actual token table.
- Fixed-point UI number formatting uses bounded output instead of Windows-only
  conversion functions; negative floating values retain their sign.
- Original Clock calendar/statistics code uses native monotonic counters,
  correctly sized `time_t` storage and thread-local date buffers. The February
  leap-year offset and smoothed-average intermediate overflow are fixed.
- Profiling CycleTimer/CycleWatch values are now **elapsed nanoseconds**, not
  CPU cycles. They must not be used to infer CPU frequency.
- Float-to-integer conversions and rounding-mode save/restore use native fenv
  operations, including nearest-even rounding and checked invalid conversion.
  Restore tokens are opaque native values, not x87 control words.
- Bounded ASCII case comparison preserves original identifier comparisons.

Five ASan/UBSan host tests pass, including the new clock/calendar, overflow,
rounding restoration and comparison cases. Foundation executables compile and
link for ARM64/API 26. They have not been executed on a device.

To reproduce the selected module pass, add these options to the full-source
compilation command, using a separate `build-controls-android` output directory:

```sh
--all-projects --project interface --project coregame_interface --project graphics
```

Add that directory as the final `--objects` argument to the mission link probe,
after the broader inventory and gameplay objects. This pass uses an incremental
composition of those object groups; it does not claim a fresh successful build
of all 705 sources. The link report now records non-symbol errors separately so
an early linker failure cannot be mistaken for zero missing dependencies.

No new APK was packaged. Next dependencies are native bitmap/device operations,
application runtime/config dispatch, input, audio, localization/console utilities
and the remaining AI compilation fixes.


## Native input, text, AI and mesh integration checkpoint

The native input backend now feeds the original EventSys queue. It supports
pointer/button events, DirectInput-compatible key codes, character input and
wheel events, preserves pending input when the queue fills, and sends releases
on pause. Typed flush preserves other event types. The original SafeQueue full
and empty checks now use its semaphore state. APK JNI touch callbacks have not
yet been connected to this full-engine backend.

Native text helpers preserve 16-bit CH storage and Windows-1252 conversion for
the English installation. UTF-16 formatting no longer aliases 32-bit wchar_t;
wide literals receive separately owned CH arrays. Console callback pointers use
pointer-width keys. AI script callbacks use existing lifecycle IDs and placement
requests use existing token IDs instead of truncated addresses.

All 22 utility and 62 AI units compile for ARM64. Main configuration dispatch,
mesh loading, runtime MRM allocation and video-variable commands also compile.
The external COM mesh simplifier remains disabled by the original release
feature flag; this is not a new mesh simplification implementation.

Six host tests pass under ASan/UBSan. The ARM64 foundation targets link but have
not run on Android. Incremental link checks reached 73 unresolved symbols before
the final clean inventory rebuild. These counts are diagnostics, not a progress
percentage or a claim that resolving names alone makes the game playable.

Remaining platform integration includes Bitmap/DirectDraw device operations,
original-engine audio, application bootstrap/frame state and the offline session
path. Engine initialization, mission object creation, simulation, rendering and
JNI input still require end-to-end execution. No new APK or successful Instant
Action launch is claimed. The verified device baseline remains the v0.5 launch
APK.

The subsequent fresh inventory build compiled 598/705 declared units. These
include unneeded desktop/editor/network projects; this is not a completion
percentage. Linking only this fresh object set plus the native foundation reports
**72 unresolved symbols and zero other linker errors**. No older object override
was used. See `logs/instant-action/native-phase3-build.json` and
`logs/instant-action/native-mission-link-phase3.json` for the exact failures and
link command. All 255 gameplay, 62 AI and 22 utility units compiled in this run.

## Native diagnostics and frame state checkpoint

The mission link now reports **62 unresolved symbols**, down from 72, with zero
other linker errors. The ten resolved dependencies are the four Debug entry
points, the original DirectX error mapper/global error value, and the original
Main run-code controller/frame time/frame number state. No success-returning
placeholder implementations were added.

`debug_native.cpp` preserves file/line/error-category context per thread, emits
pointer-width stack addresses and raises a native exception. Its opt-in watchdog
uses a real background thread: it warns after four minutes without a heartbeat
and aborts after another minute. Native fatal termination replaces the original
Windows thread-context/code-injection mechanism. Watchdog lifecycle calls belong
to the engine thread and must stop the watchdog before Android suspends it.

`frame_native.cpp` owns the real Main RunCodes object and original frame timing
state. Its engine-thread API accepts monotonic milliseconds, handles U32 clock
rollover and excludes suspended time from the next frame delta. It does not
register fake Mission handlers or execute gameplay. The Android scheduler still
needs to call this API and the original run-code processor during integration.

The complete original bitmap primitive implementation now compiles, using bounded
formatting for pixel-format names. The original `viderror.cpp` error-message
switch also compiles: DirectSound/DirectInput error expressions are generated
from the pinned SDK headers; only fixed Windows error-number declarations are
supplied locally. This does not provide a DirectSound/DirectInput device.

Seven host ASan/UBSan tests pass, including diagnostic context isolation and
watchdog start/stop, frame timing, pause/resume, backward-clock rejection and
rollover. Native foundation targets link for ARM64. The graphics/main compilation
and mission link are incremental against the preceding fresh full-engine build;
no ARM64 executable or new APK has been run. See
`logs/instant-action/native-mission-link-phase4.json` and
`logs/instant-action/native-phase4-tests.log`.

## Full Bitmap source and native movie adapter checkpoint

The complete original `graphics/bitmap.cpp` now compiles for ARM64. A fresh
whole-inventory rebuild compiles 601/705 declared units and the mission link
reports **52 unresolved symbols, zero other linker errors**. All 16 previously
missing Bitmap symbols resolve. Six previously hidden Vid pixel-format/device
symbols are now exposed; the count is diagnostic, not a completion percentage.

The source adapter supplies the exact 14-byte BMP file header and 40-byte info
header, replaces obsolete compiler/Win32 conveniences, and reads RIFF palettes
through the native original FileSys implementation. The palette parser validates
container/chunk bounds, version and entry count before changing the palette;
unknown chunks and odd-byte padding are handled without WinMM.

`bink_native.cpp` is a source-level adapter for the original Bitmap movie calls.
It uses the existing FFmpeg-backed `reign::BinkMovie`, with bounded memory input,
original one-based frame state, pause/seek/loop behavior, and 32-bit BGRX plus
555/565/655/664 pixel output. An audio movie requires an explicitly configured
PCM sink before opening; audio is not silently discarded. The sink receives
stereo PCM on the engine thread and must consume/copy it before returning.
Android playback/seek flushing and renderer integration remain to be wired.
This does not replace the device-tested v0.5 launch APK's existing intro path.

Nine host ASan/UBSan tests pass. The adapter was additionally checked against the
owned 294-frame animated billboard. The owned 154-frame intro matched the
existing decoder's frame pixels and all 1,359,360 interleaved PCM samples.
Tests cover all five output formats, padded destination rows, bounds rejection,
repeat decode, pause, seek, loop, and missing audio-sink rejection. These are
decoder/callback tests, not audible Android playback or mission rendering.
The native foundation and movie test also link as ARM64 executables; device
execution has not occurred.

Build the external decoder with `tools/build_bink_decoder.sh`, then configure
`engine/` with `REIGN_FFMPEG_ROOT` set to that platform's `install` directory.
Use `REIGN_TEST_MOVIE` for an owned Bink fixture and `REIGN_GAME_DATA` for the
owned installation. Pass the same FFmpeg install to the mission link checker
using `--ffmpeg-root`. These inputs stay outside Git.

`build_engine_gameplay.py --source graphics/bitmap.cpp` now supports focused,
inventory-validated source builds, avoiding a whole-project compile for each
local correction. Use the resulting object report as a later `--objects`
override; the link checker still removes older successes when a new build fails.

Evidence: `logs/instant-action/native-phase5-build.json`,
`native-phase5-tests.log`, and `native-mission-link-phase5.json`.
The temporary workspace was rebuilt from the pinned upstream commit, this
repository, the pinned NDK/FFmpeg downloads and the owner's stored installation.
No APK or successful Instant Action boot is claimed by this checkpoint.


## Native surfaces and GLES texture checkpoint (phase 6)

Original Bitmap now allocates real CPU-backed DirectDraw-compatible surfaces,
removing its dependency on Vid::ddx. The surface implementation owns pixels,
validates formats and rectangles, supports locks, color fills, overlapping and
scaled copies, source color keys, reference counting, and attached mip levels.
Unsupported window/GDI/overlay operations explicitly return E_NOTIMPL.

SurfaceTexture uploads those pixels through the existing LegacyGles renderer,
refreshes dirty textures, and retains CPU data across context loss. Eleven host
ASan/UBSan tests pass, including rendered pixel readback in two software EGL
contexts. This verifies texture upload and recreation, not mission rendering.
The surface implementation, GLES adapter, and original Bitmap compile for ARM64.

The mission probe now reports 51 unresolved symbols and zero other linker
errors. Remaining dependencies include original Vid device/state initialization,
presentation, audio services, offline session data, and mono configuration.
The original D3D7 draw/state calls still need connection to the GLES renderer.
No new APK, device execution, or Instant Action boot is claimed.

Evidence: logs/instant-action/native-mission-link-phase6.json,
native-phase6-build.json, and native-phase6-tests.log. Host GLES tests require
REIGN_KHRONOS_HEADERS pointing to externally staged EGL/GLES headers; Android
uses NDK headers. Owned game data and external SDK/decoder sources stay outside
this repository.


## Native display, music and local-session boundary (phase 7)

The retained mission-entry probe plus the new video attachment/detachment entry
points now report **15 unresolved symbols, zero other linker errors**. Twelve
are digital audio and three are MultiPlayer::IsHost, Data::Online and Load.
The whole-inventory ARM64 build compiled 603/705 adapted entries; subsequent
focused session builds bring the combined result to 606/705. This counts declared
project entries, including the previously documented duplicate PCH entries.

This is source-level separation of portable code from desktop platform services.
The Vid, setup and multiplayer staging explicitly selects portable portions;
unported Windows startup, CD discovery, socket and session entry points remain
undefined. It does not compile those discarded portions or supply success stubs.
The original checksum accessors and local StyxNet key/index store are retained.

Vid owns its original globals, render state, pixel-format objects and callback
registration. AttachVideo requires a real device, its matching target surface,
actual capabilities/texture formats, and an application presentation callback.
It configures viewport, backbuffer metadata, driver capabilities and formats.
RenderFlush fails when unattached or when presentation fails. Detach releases
references and clears display data. Engine managers, camera startup and the
original D3D7 draw/state-to-GLES device adapter still need integration; attaching
a display deliberately does not assert that the full engine is initialized.
The original Windows driver-quirk override is not applied to native device caps.

The original Status::ClearData used sizeof(this), which can overwrite adjacent
memory on ARM64; staging now uses sizeof(*this). Session storage now uses array
delete, copies aliased replacements before freeing old data, releases owned
records on destruction, and uses RAII for deserialization buffers.

The Redbook adapter uses imported numbered music files instead of a CD drive.
InstallMusicPlayer requires an application player and validated track paths;
without it Play reports failure. TrackCount preserves disc numbering (highest
track number, including the skipped data-track slot), not the number of imported
files. Tests cover wrap, gaps, completion, volume, invalid replacement and player
failure. The existing Android audio player is not yet wired to this boundary;
no audible Android playback is claimed by these tests.

Twelve foundation CTest cases pass with ASan/UBSan. Two additional host tests
pass: a fixture-device display attachment/presentation/lifetime test (also
checking the status canary and original sound ABI declarations), and original
session-store serialization, permissions, aliased replacement and CRC tests.
The additional harness/foundation are sanitizer-linked; reused original host
object files were not all built with sanitizer instrumentation. Neither fixture
is a mission render or network-session test. Native music and display/session
components compile for ARM64; no device execution occurred.

For the host boundary tests, compile graphics/ and util/mathtypes.cpp using
build_engine_gameplay.py with the host compiler. Supply those reports and later
focused vid/session reports to tools/test_engine_vid.py using repeated --objects,
plus the host foundation build and --ffmpeg-root. Select the session test with
--test-source engine_session_test.cpp. All sources and owned data stay external
to generated staging and binary output.

Evidence: logs/instant-action/native-phase7-build.json,
native-phase7-tests.log and native-mission-link-phase7.json.
No new APK or Instant Action boot is claimed. Link completion will still need
application startup integration and runtime validation before a test APK.


## Native PCM audio and local-session checkpoint (phase 8)

The mission-entry probe now links for ARM64 with zero unresolved symbols and
zero other linker errors. It retains native display attachment and real AAudio
startup/shutdown as well as the six original mission lifecycle entry points.
This executable is a link check: main deliberately returns failure without
executing a mission. No APK or mission boot is claimed.

A broader optional --frame-loop probe retains Game::RC initialization and its
Load/SimInit/Sim callbacks. That probe exposes 27 additional unresolved symbols,
recorded separately in native-frame-link-phase8.json. They cover client debug,
GUI activation hooks, frame variables/utilities, session orchestration, version
metadata and renderer resource lifecycle. Resolving the narrow probe is not
completion of application startup or the simulation/display loop.

A bounded PCM WAV decoder and software stereo mixer replace the Miles 2D audio
backend while retaining original Effect, Record and Reserved source logic.
The mixer uses integer resampling phase, panning, looping, priority replacement,
owner/effect stops and a callback-safe fixed voice pool. AndroidAudioOutput owns
a real AAudio float/stereo/48 kHz stream and reports startup/disconnection state.
Application lifecycle wiring and audible device validation are still pending.
The current driver has a shared 64-voice pool; Claim records availability but
requested voice-count subdivision and dedicated reserved capacity need follow-up.
3D positional services remain unavailable. Compressed MP3/OGG digital streams
are not supported by this PCM decoder; existing imported music uses its separate
player adapter. Unsupported decoding returns a playback failure and diagnostic.

All 519 WAV effects in the owned base archive decoded successfully: 14,683,947
source frames. Host tests exercise original effect playback, pan, completion,
Reaper invalidation, reserved playback/reset and named PCM stream stop. Thirteen
foundation CTest cases pass with ASan/UBSan; additional sound and session boundary
tests pass with sanitizer-linked harness/foundation and reused original host
objects (not all original objects are instrumented). No owned assets enter Git.
A FastFind base-pointer deletion failure caught by ASan was corrected with a
virtual destructor in the resource staging adapter; the Android inventory was
rebuilt against that interface. 612/705 adapted inventory entries compile.

The local-only session backend has explicit begin/end identity, host status and
CRC/data reset. Online returns false because this target has no transport.
Load accepts an empty local session scope and explicitly rejects multiplayer
save restoration before changing stored session data. It does not reconstruct
network hosts or players. Tests cover identity/reset and rejection preserving
existing data. Mission selection and the game-loop bootstrap still need to call
this lifecycle from the Android application.

Evidence: native-mission-link-phase8.json, native-frame-link-phase8.json,
native-phase8-build.json and native-phase8-tests.log in logs/instant-action.


## Phase 9: original frame-loop dependencies

The broader ARM64 frame-loop probe now reports 15 unresolved symbols (down
from 27), with no other link errors. This is still a failing link check, not
an executed mission or APK. The narrow mission lifecycle milestone remains
separate from complete startup and rendering.

Retained original Vid::InitResources, DisposeAll and LogPerf from the pinned
source, with a null-file guard for performance logging. Original Client::Debug
now compiles with its monochrome-only weapon display call properly guarded.
Portable unsigned wide division rejects zero and overflowing quotients; FP
reset clears exception flags and selects truncation; UTF-16 integer conversion
preserves signed decimal and unsigned nondecimal behavior, including INT_MIN.
Native GUI hooks retain full-width function pointers and dispatch activation
transitions to the original client's game-time readjustment callback. Duplicate
registration and missing removal are rejected. Hooks are engine-thread-only.
Trilinear availability follows the attached device's minification/mip-filter
caps and resets to disabled on detach, replacing Windows driver-quirk lookup.

Validation: both affected original translation units compile for ARM64; all
13 foundation CTest cases pass with ASan/UBSan; the native Vid fixture tests
pass, including full and partial filter capabilities and detach reset. The
Vid harness/foundation are sanitizer-enabled but reused original objects are
not all instrumented. Resource creation/disposal is retained and link-checked,
not executed against a complete initialized engine in this checkpoint.

Remaining symbols are Main::elapCap/triCount; local-session readiness, setup,
processing, saving and transport-facing interfaces; Version::GetBuildString;
and associated local player enumeration. Define these with coherent lifecycle
and offline mission behavior, not unconditional success returns. Real DX7 draw
submission into GLES, engine/app startup and a device mission boot remain.
Evidence: native-frame-link-phase9.json and native-phase9-tests.log.


## Phase 10: frame-loop link complete for the offline target

The broadened ARM64 link probe now succeeds: zero unresolved symbols and zero
other link errors, retaining 598 compiled source objects in the archive. It
retains Game::RC Load/SimInit/Sim callbacks, native video/audio boundaries, and
explicit local-session begin/end plus runtime-variable initialization. This is
not an APK or executable mission boot; main deliberately refuses to run it.

The local-only session owns one original Network::Player entry, resettable
readiness, and synchronous local data-handler dispatch. It has no network
packet queue, so lag is zero and ProcessSyncData returns false without changing
outputs. Process validates the active session; there is no asynchronous
transport to pump. Registration rejects duplicate handlers; missing handlers
and inactive sends fail explicitly. Names obey the original 15-byte limit and
embedded NULs are rejected. Data::HaveData includes an active native session so
original Load::Post actually calls the offline mission setup hooks.

SetupPreMission seeds the original deterministic RNG with 1 and initializes
its data CRC. SetupMission requires a preloaded mission, selects its first
available playable Team, creates an original Player, assigns the current and
display team, then invokes original Team::Purge to remove unoccupied playable
teams. It creates no AI opponents and preserves mission side/resources/objects;
lobby options and mod rule sets are not implemented. SetupPostMission checks
that loading and player/team setup completed. These game-setup paths compile
and link but have not yet executed with a loaded mission. The application must
select/mount the requested mission and initialize engine managers first.

Main's elapCap/triCount retain their original VarInteger types and defaults
(8/0); InitializeRuntimeVariables must run after TrackSys/VarSys startup.
The native build string reports Reigncarnated and the pinned upstream revision,
without requiring Windows executable version resources. Full local session
save/restore remains unsupported; Save throws before writing a partial save.

Validation: three changed original source units compile for ARM64. Expanded
host session tests pass for runtime variable creation/update/duplicate refusal,
identity/length validation, player enumeration, readiness consumption/reset,
local message dispatch and duplicate/unknown-handler failure, offline sync
output preservation, explicit save rejection and end-session cleanup. Harness
and foundation use ASan/UBSan; reused original host objects are not all
instrumented. Original mission/team creation is not claimed as runtime tested.

Evidence: native-frame-link-phase10.json and native-phase10-tests.log. Next:
real DX7 draw/state submission into GLES, complete manager/app startup, then
Instant Action on-device launch validation. Additional dependencies may appear
as those currently unretained application entry points are integrated.


## Phase 11: first real DX7 device-to-GLES drawing path

CreateGlesDevice now supplies a reference-counted IDirect3DDevice7 adapter over
the existing LegacyGles renderer and native CPU surfaces. This is a real draw
adapter, not the prior fixture device. Initial support is full-target viewport,
combined color/depth clear, scene lifecycle, indexed/nonindexed TL triangle
lists, one texture stage with revision-aware uploads, basic depth/blend/fog/
specular state, texture addressing, matching point/linear min/mag filtering,
and a subset of texture modulation/selection combinations. Unknown state,
unsupported FVF/topology, partial clears, mip filtering, and extra stages fail
explicitly. Invalid indices are rejected before drawing. Renderer/current GL
context must outlive the device; release the device before context loss.

Software EGL tests now invoke DX7 virtual draw calls and verify solid red,
textured green, and updated blue pixels. They exercise invalid scene/index/FVF
and unsupported-state failures, plus destruction/recreation across two contexts.
All 13 foundation tests pass with ASan/UBSan. The adapter also compiles into the
ARM64 surface/GLES static library. This does not execute original engine startup
or a mission, and the adapter is not yet attached to the Android application.

Remaining renderer work includes additional original render-state and texture
combinations, culling, partial clears, state queries, additional vertex/draw
modes required by the engine, richer caps reporting, and coordinated app/context
lifetime. Startup still needs manager initialization, mission mounting and RC
integration. No new APK is claimed by this checkpoint.
Evidence: native-phase11-tests.log.


## Phase 12: additional original rendering states

The DX7/GLES adapter now supports clockwise/counterclockwise TL triangle
culling, greater-than alpha testing with byte reference, texture modulate2x /
modulate4x and decal RGB with modulated alpha. Culling uses signed screen-space
area before submission (TL y increases downward); it currently allocates a
filtered index vector per culled draw, so optimize after correctness bring-up.
Depth comparison explicitly accepts the renderer's LEQUAL mode; other compare
functions still fail. Alpha comparison explicitly accepts GREATER only.

LegacyGles now supports independent whole-target color and depth clears, with
validated custom depth values. Rectangle and stencil clears remain unsupported.
Pixel tests verify both cull directions, alpha boundary rejection/acceptance,
2x/4x saturation, decal alpha, depth-only clear preserving color, and color-only
clear preserving depth. All 13 foundation tests pass under ASan/UBSan, and the
updated renderer/adapter compiles for ARM64. Evidence: native-phase12-tests.log.
These are software EGL rendering tests; no original mission or APK execution.
Startup integration and remaining renderer API coverage are still pending.


## Phase 13: state readback and additional triangle topologies

Added GetRenderState for supported depth/write/blend/fog/specular/cull/alpha
state and GetTextureStageState for the supported single-stage configuration.
This restores the read-before-temporary-write pattern used by original
Vid::SetZWriteState. Failed unsupported setters leave existing state intact.
Fixed pipeline configuration explicitly accepts clipping enabled, lighting and
extent generation disabled, solid fill and Gouraud shading, consistent with
the implemented prelit TL/GLES path; incompatible configurations fail.

Triangle fans and strips now expand to triangle lists with strip parity
preserved before culling. Input indices and bounded counts are validated before
expansion. Tests sample both sides of the rendered quads under CCW culling;
state tests cover depth-write read/change/restore, null outputs, unsupported
lighting enable, texture-address readback and failed-update preservation.
All 13 ASan/UBSan foundation tests pass; ARM64 renderer compilation succeeds.
Evidence: native-phase13-tests.log. No mission startup or new APK yet.


## Phase 14: original Vid draw and presentation integration

AttachGlesVideo now creates a native RGBA surface and real GLES device, attaches
them to Vid, and releases temporary references. DetachVideo owns final release;
invalid sizes/present callbacks and duplicate attachment are rejected. Renderer
and current context must outlive attachment. This does not initialize engine
managers, cameras, resources, or mark the full engine initialized.

A new software EGL integration test attaches through this helper, invokes the
original Vid::SetZWriteState read/change/restore sequence and original
Vid::DrawIndexedPrimitive with RS_BLEND_DEF, presents through Vid::RenderFlush,
and verifies displayed red pixels. It then detaches and repeats with a second
context. Initial zero flags in the test generated invalid blend selections;
using the original wrapper's required packed blend flags removes those warnings.
The completed test passes cleanly. Harness/foundation are sanitizer-enabled;
reused original host objects are not all instrumented.

The broader ARM64 frame-loop probe now retains AttachGlesVideo and links the
real surface/GLES device library as well as the original game callbacks. It
still links with zero unresolved symbols and zero other errors. The updated
original vid.cpp compiles for ARM64. No mission or Android activity executed.
Full engine-manager/application startup remains the next integration target.
Evidence: native-frame-link-phase14.json, native-phase14-tests.log.

Run real host integration with tools/test_engine_vid.py --test-source
engine_vid_gles_test.cpp --khronos-headers <headers>, supplying compiled original
graphics/math/adapted Vid objects, foundation libraries and FFmpeg root. Use
LIBGL_ALWAYS_SOFTWARE=1 and ASAN_OPTIONS=detect_leaks=0 in this environment.


## Phase 15: initial graphics resource startup executes

The real EGL/Vid test now runs original Heap::Init, InitBuckets, bitmap/material
manager initialization and InitResources(TRUE), followed by original drawing
and presentation. Resource managers shut down before device detach, buckets
and heap last. An initial wrong test cleanup order caused an ASan null-device
crash in Bitmap::Manager::DisposeAll; corrected ordering removes it.

Device material storage/readback now supports original default material setup.
TL lighting is already supplied by vertices; GPU lighting remains unsupported.
Disabled texture stages support the original cleanup path, including disabling
stage-one alpha. The integrated startup/draw/shutdown test now passes cleanly
across two contexts. It needs original console and VarSys/TrackSys host objects
in addition to the phase-14 dependencies. All 13 foundation tests pass and the
adapter compiles for ARM64. Evidence: native-phase15-tests.log.

This initializes only the minimal graphics resources (InitResources(TRUE));
light/mesh resources, terrain, camera, interface and full Game startup remain.
No mission loaded and no APK produced at this checkpoint.


## Phase 16: mesh and lighting startup with owned resources

Added engine_graphics_startup_test as an integrated host runner. It mounts an
owned base.zwp, initializes original TrackSys/VarSys/FileSys, heap/buckets,
bitmap/material managers, Light::Init, Mesh::Manager::Init (including the null
mesh), minimal default resources and Light::InitResources. All three light
textures are checked for actual native surfaces, not just placeholder objects.
The original draw/presentation path then runs and all managers tear down in
order. The complete sequence passes across two contexts, with zero open files
at FileSys shutdown. Software pixel readback permits one byte of rounding
(254 or 255 red, exact zero green/blue). No owned assets are committed.

Run test_engine_vid.py --test-source engine_graphics_startup_test.cpp with
--test-arg pointing to owned base.zwp, --khronos-headers, original graphics/math/
Vid/console/VarSys/TrackSys host objects and foundation/FFmpeg libraries. The
harness/foundation use ASan/UBSan; reused original host objects are not all
instrumented. This checkpoint adds a verified startup runner, not Android
application wiring. Terrain, camera, interface and Game initialization/mission
loading remain. Evidence: native-phase16-startup.log.


## Phase 17: terrain/camera execute; interface startup reached

The passing graphics runner now initializes original Terrain and a real Camera,
then draws and tears down both across two contexts. A broader host inventory
compiled 613/705 entries. Added engine_mission_boot_test as a development startup
runner: it links the original IFace::Init and Game::Init paths with the real
GLES backend. It currently fails during interface startup: after adding missing
MultiLanguage::Init, the next assertion is currentLanguage. The language
configuration/database must be loaded before interface control construction.
Game initialization and mission loading have not executed. Exception cleanup
also reports remaining interface state and aborts from a nonempty tree; partial
startup recovery needs work. This runner is not a passing test or mission boot.

Resolved dependencies exposed by this broader retention: original ICEdit now
compiles with fixed-width Min/Max arguments and an application-local text
clipboard (Android system clipboard remains unwired); MeshEffectSystem namespace
qualification is corrected. Native 3D-provider queries report no provider and
refuse provider changes. Music settings now have enabled/claim/release state;
playback still requires a real installed player. Desktop window-mode toggling
returns false on the Android target. Duplicate ownership of Stream methods was
removed from the old Miles stream unit; native sound_digital owns them.

Evidence: native-phase17-tests.log, native-phase17-startup-failure.log. The
foundation tests pass; fixed original source units compile on host and ARM64.
The language configuration blocker and full application integration remain.


## Phase 18: language, interface and Game initialization execute

The startup runner now reads the owned library/engine/startup.cfg and applies
its ConfigureMultiLanguage scope, loading the original English database from
base.zwp. Fixed VarItem::Clear deleting binary byte arrays with scalar delete;
ASan caught this while loading/cleaning the language database. Added native
music SyncEnabled for original profile login's claim/release sequence.

Original IFace::Init now completes. Original Game::Init/Common::Init needs an
active User, so the runner creates two temporary profiles in an isolated temp
directory, leaving the owned installation untouched. Both passes now execute
Game::Init, Game::Done, User shutdown and IFace shutdown successfully. The second
pass exposed stale data.cgDefault after its owning color tree was disposed;
clearing that pointer fixes the ASan use-after-free. Repeated startup, drawing,
resource shutdown and profile-directory cleanup now complete with zero open
files. Harness/foundation use sanitizers; original objects are not all instrumented.

The binary-variable and interface fixes compile for ARM64. This is a successful
host initialization milestone, not a mission boot: PreOpenMission/OpenMission
and simulation are not called yet. Full mission preloading remains next.
Evidence: native-phase18-startup.log.


## Phase 19: original 2crazed PreOpenMission entered

The development runner now applies the owned ConfigureFileSystem scope, exposes
owned mission/world/side/pack directories through read-only-intent symlinks in
its isolated profile workspace, initializes campaign/mission/difficulty/mod
managers, selects the original 2crazed entry and begins an offline session.
It calls Game::PreOpenMission(nullptr) for the first time. No mission files
are modified; generated profiles remain temporary.

PreOpenMission tracing confirms Team, Environment, MoveTable, SidePlacement,
UnitLimits, TerrainGroup, AI, TaskCtrl, Config and MissionVar initialization.
The runner then fails on an invalid VarPtr during configuration/preload.
Video command/default variable initialization is still missing. An attempted
original Vid::Command::Init introduces desktop-only mode switching, gamma,
hardware detection and driver-policy link requirements; its failing link log
is preserved separately. That call is not left in the default runner so the
original preload failure remains reproducible. Adapt that variable setup with
real native behavior before continuing. MissionPreLoaded is not reached and
OpenMission is not called. Partial-startup exception cleanup still aborts.

Original game.cpp startup diagnostics compile for ARM64. This checkpoint is a
failing integrated runtime attempt, not a passing mission test or new APK.
Evidence: native-phase19-runtime-failure.log and native-phase19-video-init-link.log.


## Phase 20: missing mission loader variables identified and bound

Invalid-VarPtr diagnostics now capture caller addresses, and the host boundary
runner links without PIE so addr2line can identify original call sites. The
phase-19 failure was traced to GameGod::Loader::SubSystem assigning its progress
VarString, not to Vid variables. The prior video-variable diagnosis was an
inference and was incorrect for that failure. Added InitNativeVariables in the
original Loader namespace to bind the exact system/percent variables normally
created by Windows GameGod startup, with duplicate-registration rejection.

The mission runner calls this before PreOpenMission and initializes original
Sound. Preload now passes that variable assignment but stops on a separate
initialized assertion while the loader repaints, before the first config-scope
trace. Full interface painting requires more startup prerequisites. Calling
GameTime::Init alone also failed early; do not add it without its dependencies.
MissionPreLoaded/OpenMission remain unreached; exception cleanup still aborts.
Added per-config-scope diagnostics for the next investigation. Loader and trace
source units compile for ARM64 (3/3). Evidence: native-phase20-runtime-failure.log.


## Phase 21: original 2crazed mission opens on the host

Verified a complete host lifecycle with the owned base/mission data and real
software GLES: original startup, English database, interface, profile, Game,
mission discovery/selection, PreOpenMission, native local player/team assignment,
OpenMission, MissionLoaded assertion, PostMission checks, full Game shutdown,
resource shutdown and final presented-pixel check. The unused second playable
team is purged. Exit code is 0; no sanitizer failure; FileSys reports zero open
files. This does not run simulation ticks, render the loaded mission scene,
select units, build structures, or execute Android app startup. The last pixel
check is a known red quad after mission shutdown, not a mission screenshot.

Runtime traces resolved successive missing prerequisites: Clock::Time::Init,
PerfStats::Init, Sides, GameTime, SaveGame and GameSound; mesh command defaults;
far-plane/fog variables; GameGod type/object-check variables. The obsolete
x86 CPU-speed mesh budget heuristic is replaced with explicit 10000/11000
rendering-budget defaults, with automatic MRM still off. Original configuration
and type loading now complete, including GOD animation/resource loading.

Fixed overlap-safe Strmcpy (original mesh animation name self-assignment hit
strncpy overlap), and array ownership in Tactical settings/modifiers, terrain
mapping, pathfinding maps, GameObjType display names and UnitObjType AI bleed
maps. Retained original decimal/float formatting for performance diagnostics,
with widened decimal magnitude for INT_MIN. Mixer CPU telemetry is explicitly
unsupported and throws if requested. Native attachment now creates its GLES
frame buffer before a loading-screen repaint; the device also accepts the
original full-target rectangle clear form. Post-mission test drawing explicitly
unbinds textures and sets its own render state.

All 13 foundation tests pass, including added overlapping-string, numeric
format and full-rectangle-clear checks. The foundation/renderer and 9 affected
original source units compile for ARM64. Harness/foundation are ASan/UBSan
enabled; not all reused original host objects are instrumented. Exception
wrapping is test-only and records caller addresses for reproducible failures.
Evidence: native-phase21-mission-open.log and native-phase21-tests.log.

Next: run original simulation/render callbacks against this loaded mission,
then integrate the bootstrap with Android Activity/EGL/audio/input lifetime and
build the Instant Action candidate APK. No new APK is produced here.


## Phase 22: original Load/SimInit/Sim executes on host

The 640x360 real-GLES host runner selects the original 2crazed mission, creates
its local player, runs the original Load -> SimInit -> Sim callbacks, and
verifies 29 nonzero simulation cycles over 32 RC frames. It then explicitly
clears the active RC (invoking Sim::Done), destroys the original managers,
checks a known red quad after mission shutdown, and exits 0 with zero open
files. The red quad is a backend/lifetime check, not proof of correct terrain.

An inspected frame captured before shutdown shows the original mission HUD,
but the world view is visibly incorrect. This is not a visually correct or
playable mission, an Android boot, a main-menu launch, or a new APK. Handled
console/configuration exceptions still occur during original interface setup.
Camera/terrain/mesh rendering and native UI command bindings require further
investigation. The local session still has one player and no AI opponents.

The native device now copies possibly odd-address bucket index bytes into an
aligned WORD buffer before validation/culling/submission. Both original TL
clipping settings are accepted: vertices are transformed and clipped by the
engine, with GLES rasterization clipping remaining active. Invalid settings
still fail. A real-GLES regression renders deliberately odd-address indices,
rejects out-of-range indices, and verifies presented pixels over two lifecycles.

Native video variable startup now binds far/fog/filter, mirror-debug,
render-post-sim and the four original 1.0 performance defaults. The runtime
exposes the shipped release UI configuration and a read-only zero online flag.
GameGod exposes registration/shutdown for its original menu controls. Original
video/mesh option controls register; desktop InitDD/SetMode attempts explicitly
return failure because EGL surface/mode lifetime belongs to the native app.
Mesh export status logs without entering the Windows mesh-viewer loop.

Original network report/player-info/local-ping controls retain local identity
and truthful zero transport telemetry. The offline roster/sync list use the
original list-box lifecycle/configuration with the actual local player list;
remote team/group/lobby editing is not exposed. The host bootstrap registers
these controls and the original chat editor for the shipped mission config.

Runtime traces also fixed array ownership in minimap row buffers, order-history
byte allocations and drop-list variable names. The runner now calls RC::Clear
before RC::Done, matching original game shutdown, rather than deleting the RC
registry while the simulation/client remain active. Native debug exit hooks,
sync logging and current-local-player access from the prior in-progress local
checkpoint are preserved with this continuation.

Validation: 13 affected original units compile for both host and ARM64; the
GLES adapter also compiles for ARM64. The session boundary test passes. The
final simulation run uses freshly rebuilt changed objects and exits 0.
Harness/foundation use ASan/UBSan; reused original objects are not all
instrumented, and LeakSanitizer is disabled for Mesa integration. Evidence:
native-phase22-simulation.log, native-phase22-session.log and
native-phase22-validation.json. The simulation runner is selected through
`tools/test_engine_vid.py --test-source engine_simulation_test.cpp`; provide
owned base.zwp via --test-arg and the rebuilt original object directories via
--objects. Optional REIGN_CAPTURE_PATH writes the presented pre-shutdown frame
as PPM for visual inspection.

Next: correct the observed world rendering and incomplete interface command
bindings, then integrate this bootstrap with Android Activity/EGL/audio/input
lifetime and produce the Instant Action test APK.


## Phase 23: ARM64 Instant Action APK candidate packaged

Produced Reigncarnated-Instant-Action-v06-Candidate.apk. This is a separate test
application (org.reigncarnated.missioncandidate), target SDK 35, min API 26,
ARM64 only, signed and verified, with ZIP checksums and 16 KiB library/APK
alignment verified. It has not been launched on an Android device or emulator.
It retains the exact v05 native menu binary and rebuilt Java menu Activity;
its explicit Instant Action (test) button opens a separate GLES mission Activity
and the original 2crazed engine through libreign_mission.so. This is not the
original menu's complete Instant Action configuration/lobby implementation.

A reusable NativeEngine::MissionApp now owns original startup, data mounting,
profile login/creation, Load/SimInit/Sim, frame clock, input viewport and ordered
shutdown. The JNI boundary supplies an EGL-current renderer and AAudio output.
The Activity translates one-finger taps/drag selection into the original primary
mouse action, supplies arrow buttons for camera keys, and has a Deselect button
for the original secondary action. Pause releases held native keys, stops audio,
freezes frame/simulation processing and readjusts timing on resume. Context loss
restarts this test mission; save/restore is still unsupported. Returning from a
mission closes it on its GL thread before pausing the finishing Activity.

The phase-22 image diagnosis is refined: the camera is 40 metres above the local
floor, and its forward ray projects to screen centre. Its starting team owns
three rigs, each with seven-cell sight and valid visible cells. The short initial
capture contained the normal gradual shroud reveal (fog 90 after 29 cycles),
not evidence that the terrain projection was generally broken. With a longer
run the ordinary shroud reaches 255 in the rigs' visible area, without reveal
cheats. A second concrete issue was native video attachment never publishing
isStatus.initialized; original render-state initialization was therefore skipped.
Attach/detach now publish/reset it. This enables correct depth/clear and sampler
setup and requires binding the original wire/flat/specular/dither/texture vars.

The original sampler setter visited two stages regardless of capabilities;
it now visits only the stage(s) the device exposes. The native adapter advertises
one mip level and disables unsupported mip filtering in its published defaults.
Added real GLES dither control and affine UV interpolation alongside perspective
UVs, plus queryable legacy ambient/texture-factor/fog-distance state for the TL
pipeline. Unsupported nonzero legacy antialias/color-key/depth-bias requests
still fail. A two-colour texture under unequal reciprocal W distinguishes the
affine and perspective modes in a real-pixel regression; two GLES lifecycles pass.

The compact data ZIP contains the owned base/shell/world/side assets, original
library configuration, mods and the original 2crazed map. Both ZIP indexing and
folder import now retain library paths. A Java-only test executes the actual
ZIP/importer implementation and verifies the resulting runtime dependencies.
The shared C++ application test then boots that imported directory (including
its flat imported base.zwp), selects a rig through queued pointer events, issues
an original primary-click contextual move, and observes displacement
(15.9999,15.9999). It also verifies pause/resume, held-key release, duplicate
instance refusal, shutdown and existing-profile login on a second full startup.
The imported-data run completed both lifecycles (249 and 82 simulation cycles).

Repeated startup exposed PerfStats::Done leaving sysInit true; the staged
original now clears it. A prior debug run terminated with SIGABRT during type
loading without a useful diagnostic; subsequent direct and imported-data
lifecycle runs completed. Device behavior remains to be established.
Harness/new runtime/foundation use ASan/UBSan; original reused objects are not
all instrumented and LeakSanitizer is disabled for Mesa integration.

The ARM64 library links 603 compiled original units plus the new application,
JNI and current renderer/diagnostic sources with --no-undefined, hidden internal
symbols and -Bsymbolic. Only seven JNI entry points are exported, avoiding C++
symbol/DrawState ABI interposition with the retained v05 menu library. The
runtime links libc++ statically. The final package contains both native libraries.

Rebuild entry points: tools/link_mission_android.py (prepared original objects,
foundation archives, FFmpeg and NDK), tools/package_mission_candidate.py (exact
v05 menu APK, new mission library and SDK), and tools/test_engine_vid.py
--test-source engine_app_test.cpp for the shared host lifecycle. The matching
source/relink ZIP includes the original menu decoder recovery material, current
link objects/archives, source snapshot, link commands, test signing key and
unstripped mission symbols. No game assets are included in that relink ZIP.

Current limits: one human team and no AI opponents; some original configuration
commands/options remain unbound and emit handled console errors; construction,
combat, mission victory and device audio/input/context behavior are not claimed
validated. Next validation is on the Samsung Fold 5: import the matching data
ZIP, let the existing native menu start, then tap Instant Action (test).
Evidence: native-phase23-application.log, native-phase23-package.log and
native-phase23-validation.json.

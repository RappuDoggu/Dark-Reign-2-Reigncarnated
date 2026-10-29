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

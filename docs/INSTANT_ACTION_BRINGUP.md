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
- Four host CTest checks pass with ASan/UBSan, with LeakSanitizer disabled
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
StdLoad::GetIdentAsU32 stores a heap pointer in a 32-bit ID. Its unsafe inline
body is replaced by an unresolved declaration until the matching original
Resolver::Type path is ported; using it currently fails at link time, rather
than silently truncating pointers. Original Matrix retains its 64-byte row
layout; its unused, nonstandard anonymous SIMD `Mat` alias is removed. The
original Windows-only RECT conversion is excluded on native builds.

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

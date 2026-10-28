# Instant Action full-engine bring-up — 2026-10-28

Target: original Instant Action mission, one human player, no opponent; touch
selection of a real unit and construction of a real structure. The v0.5 launch
APK remains the working baseline; the user has confirmed its intro/menu on device.
No new mission APK is delivered by this checkpoint.

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

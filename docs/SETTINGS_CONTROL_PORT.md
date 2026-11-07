# Original settings controls — 2026-10-26

The original Vid::Options class/lifetime, complete Vid::Graphics window,
Game::SaveLoad dialog, ColorButton/ColorEditor and RGB/HLS conversion functions,
and ICSoundConfig window are now staged from pinned external source and compiled.
No original source files or installation assets are copied into this repository.

## Native boundaries and limits

- Display options list the supplied renderer and current surface dimensions.
  Android owns the surface mode. Dedicated video-memory fields are hidden because
  no reliable portable measurement is available. Unsupported DirectDraw switching
  and graphics-setting application throw explicit errors. Vid::Graphics retains
  its original events; its native mode-change boundary disables guard-band clipping.
- Save/load retains original event sequencing and delegates metadata, active
  mission, save, load and date/time to explicit services. Missing services throw.
  This is not a mission serializer or demonstrated save-game loading.
- The original color controls and conversion functions compile with null-pointer,
  fixed-width integer and transparent-alpha overload corrections. No replacement
  color-picker UI was written.
- Sound settings have an explicitly disabled-audio backend. Volume preferences
  are retained; there are no hardware providers, successful claims or CD playback.
  Native sound output remains unimplemented.

## Verification

The combined configuration/control test passes **56 checks**. It exercises live
color-button drawing, RGB/HLS reference values, display-list population/resizing,
original save/load request dispatch and failure boundaries, and original sound
window activation in disabled mode. The full local ASan/UBSan suite passes
**51/51**, with LeakSanitizer disabled for this workspace's procfs limitation.

Failures preserved in logs include missing TRANSLATE mapping, color overloads,
and an initial color-math link dependency placed too high in the library graph.
Those source/build issues are fixed. The workspace also produced two empty object
files, an empty test executable and a test executable without execution permission;
these exact generated artifacts were rebuilt/repaired before the final pass.
The cause of that workspace artifact damage is not established.

With the user's external base.zwp, the original startup configuration reaches
**112 interface statements, 37 registered classes, 9 fonts, 4 bitmaps and 46
cursors**, then stops at **Game::CampaignSelection**. This is configuration
progress, not a percentage of the engine, a complete Login lifecycle, or a menu APK.

## Renderer integration CI

Commit acc203341b72ffc5e9ec44cdfc3d81742894d8bc passed host and Android jobs in
run 40310661006. The Android job's software-EGL tests report the ordered original
UI geometry/font presenter and frame recovery PASS in both context generations.
The preceding d904a757 build exposed an include-order collision with Windows
bucket.h; isolating UI pixel-check compilation fixed it. These results do not
validate this newer settings-control increment on ARM64; its CI is still pending.

## Next dependency

Port the original campaign selection control with campaign/difficulty/mission
catalog and preview services, followed by Login/profile lifecycle and the Android
frame host. Do not replace this path with a fabricated main menu.

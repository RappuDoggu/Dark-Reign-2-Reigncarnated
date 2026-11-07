# Original campaign catalog, selector and preview — 2026-10-26

## Implemented

- Original campaign ordering, progress serialization/restoration and difficulty
  settings compile from pinned external source. The original read-only mission
  catalog uses native, bounded directory enumeration instead of Win32 Find APIs.
- Paths retain original backslash-separated identifiers while native lookup
  supports case-insensitive installation names and rejects traversal/symlink escape.
  Registered mission loaders read either a directory or a ZWP archive.
- The complete original CampaignSelection control and DropList compile and link.
  Original preview metadata parses game.cfg, team/side/start-point data and rules;
  terrain.bmp is decoded with the existing original bitmap conversion path.
- Preview file contexts and CPU texture ownership now use scoped native resources.
  Constructor failures clean up teams, missing resources restore the prior context,
  and display-mode changes clear/rebind the preview control's image around reload.
- DropList fixes include missing legacy precompiled-header dependencies, the
  transparent-style spelling, full-width pointer formatting and matching delete[].
- MonoView uses the original MONO_DISABLED build branch: legacy monochrome debug
  hardware is disabled. The original keyboard viewer uses the configured key-name
  table with scoped binding and a checked 256-entry boundary.
- Original TextureInfo(Bitmap*,mode) is now staged alongside the draw helpers.

## Actual verification

**52/52** local ASan/UBSan tests pass (LeakSanitizer disabled for workspace procfs).
The catalog/preview test passes **31 checks** for mission ordering, campaign-progress
round trips, difficulty validation/recovery, native paths, preview metadata and
terrain pixels/reload, and missing service errors. The combined live-control test
passes **60 checks**, including campaign selector/drop-list activation/deactivation
with an empty campaign fixture. This does not validate an interactive populated
campaign screen against real installation data.

The original catalog reads the user's external installation and reports **9 groups,
60 missions, 2 campaigns, 4 difficulty settings, default difficulty 2**. No assets
or player profiles were copied into the repository.

The real Login configuration now reaches **120 interface statements, 47 classes,
9 fonts, 8 bitmaps and 46 cursors**, stopping at **Game::Login**. The four additional
bitmaps are loaded by the original wallpaper configuration. No Login lifecycle,
main menu, game simulation or new Android APK has been demonstrated.

## Remaining boundaries

Campaign preview activation requires an actual mission-resource loader and a
configured default-faction provider. Mission launch requires a real run-code
consumer; a missing consumer throws. Faction initialization, player profiles/Login,
root command dispatch, shell lifecycle and the Android frame host remain work.
Mission catalog APIs do not initialize simulation or implement write/delete APIs.

## Failures and fixes

Initial compilation exposed missing parser include order and a missing legacy
string-helper declaration. Native std::strstr preserves that comparison. The
preview-only test exposed a static-library dependency cycle through UI registration;
registration is now compiled separately from preview service implementation.
The first progress test omitted the original Progress::Clear lifetime requirement;
its scoped fixture now clears it before destruction. Workspace-generated empty
objects and lost executable permissions were repaired before the successful pass;
clean CI remains the independent reproducibility check.

The preceding settings-control commit ef95c364e8c38b4177b7962177baaac70823f475
passed both jobs in run 40312030783. CI for this newer increment is pending.

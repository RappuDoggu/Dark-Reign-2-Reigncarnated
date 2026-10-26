# One-folder setup v0.3 — 2026-10-26

The owner confirmed v0.2 working on-device. v0.3 keeps its package and signing
key, preserves app-private profiles, and replaces the separate base/shell/music
pickers with a one-time installation-folder import.

## User flow

- Install v0.3 over v0.2. Existing v0.2 paths and selected music are retained until
  the owner chooses Settings > Change game data folder.
- Fresh installation: select the extracted Dark Reign 2 folder containing
  `packs/base.zwp`, `missions/system/shell.zwp`, and optionally `music/`.
- Copy and validate the menu data once. This increment imports base, shell and
  all OGG/MP3/WAV music files directly inside music, not the full game installation.
- Later launches open Login without individual asset selection. Music starts
  automatically and advances through the imported tracks, wrapping at the end.
- Settings contains effects/music volume, data-folder replacement and diagnostics.
  Keyboard remains available for the original Login profile editor.

Android's document-tree picker grants access to the selected folder. No broad
storage permission or direct filesystem assumption is used for the source.
Because the menu files are copied locally, reopening does not depend on keeping
access to an external SD card or provider. The source folder is never modified.
Android may prevent selecting a storage root or the Downloads root; select the
extracted game subfolder instead. Platform reference:
https://developer.android.com/training/data-storage/shared/documents-files

## Integrity and lifecycle

The importer checks required folders case-insensitively and rejects ambiguous
names. A missing music directory is supported with a visible notice. Archive
headers, table ranges, records and required configuration names are checked.
Known source sizes are checked after copy; unknown provider sizes are supported.
Bounds: 512 MiB per file, 2 GiB total, 128 music tracks, 10,000 children per listed
directory. It checks free storage for known sizes and supports cancellation.

A unique staging directory is copied and synced, validated, then renamed.
Preferences switch to the completed snapshot only after success. Cancellation,
short reads and invalid archives leave the prior installation intact. Replaced
snapshots are removed only after the old native reader is closed; unreferenced
importer snapshots are cleaned on the next launch. Existing v0.2 loose imports
are retained for compatibility. Process death during import leaves a disposable
unreferenced staging directory, not an active partial installation.

Music uses MediaPlayer with focus/lifecycle handling and separate saved volume.
An undecodable track is skipped, with a bounded retry through the playlist.
Pause/resume restarts the playlist from its beginning; resume-position storage
and exact original Redbook track scheduling remain future work.

## Moving shell groundwork

Pinned original `coregame/spline.h` and `spline.cpp` are hash checked and staged
outside Git. Their arithmetic compiles unchanged in an isolated namespace using
a narrow three-float vector adapter. No original source is copied into this repo.
Only the Hermite path is exercised; the original Bezier implementation is not
claimed validated. This isolated target is not yet wired into the APK renderer.

`tools/shell_camera_probe.cpp` uses the original config parser on the actual shell
archive, locates Main::Enter's Movement and Focus curves, and samples them through
the original CubicSpline implementation. The archive contains 120 top-level
object declarations and 18 bookmarks. The entrance movement has two usable
segments totaling 6 seconds (the final non-looping control node does not add a
segment). The shell objective selects it at 0.5 seconds and sends the menu's
completion event at 4 seconds. These are distinct timings, not an assumed loop.

Remaining scene work: load `terrain.blk`, resolve the Sprawl world material pack
and side/model resources, render the scene with a world-space camera, then bind
bookmark progression and objective/cinematic callbacks. Scene weather and object
animation follow. The APK still has the static backdrop; no moving background,
terrain renderer or mission gameplay is claimed by this checkpoint.

## Validation

- 58/58 host tests pass with ASan/UBSan and ASAN_OPTIONS=detect_leaks=0.
- Java importer: 24 checks with the real installation, including case folding,
  optional music, unknown sizes, missing directories, duplicate names, wrong and
  corrupt archives, short reads, cancellation, size limits and old data retention.
  Real data import discovers both archives and all 9 music tracks.
- Original spline: 10 endpoint/tangent/interior/duration checks against an
  independent Hermite polynomial oracle; real shell camera/focus samples pass.
- Original spline also compiles into an AArch64 ELF object with NDK r27c.
- Android Java compiles against API 35; APK ARM64 build, signature, 16 KiB
  alignment, metadata and ZIP checks pass. v0.2/v0.3 signing certificates match.
- SAF picker interaction, UI layout and automatic audio need a Fold 5 device test.
  No Android emulator or physical device was available in this workspace.

Logs: `docs/logs/folder-v03/`. The CI workflow now runs the synthetic Java
importer tests in addition to native checks.

## Private APK

[Reigncarnated-Main-Menu-v03.apk](https://chatgpt.com/library?file_id=libfile_ea9117e1ec0081918615beb0c0b5150e&account_id=personal)

Select the extracted installation folder, not the .7z archive. The original
[private installation archive](https://chatgpt.com/library?file_id=libfile_a485ea4680c8819190055acaec63a1ba&account_id=personal)
remains available if a device needs the folder restored. No game assets are
embedded in the APK. Use the v0.2 signing recovery archive for future updates.

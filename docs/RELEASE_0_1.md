# Reigncarnated 0.1 — native ARM64 Android early access

Reigncarnated brings Dark Reign 2's original engine to Android, with native ARM64
execution, offline Instant Action, original menu/city animation and touch controls.
Version **0.1** is an early-access release for testing and feedback. It is not a
claim that every campaign or original PC feature is complete.

## Available

- Original 10 Hz simulation, native GLES rendering and retained texture/geometry
  streaming. Display refresh is independent of simulation speed.
- Original Instant Action setup with map/difficulty/seed selection and one original
  strategic AI opponent, plus practice mode.
- Campaign starts and unrestricted individual mission/cutscene tests. The known
  package catalog has 31 declared targets, including 12 separately named
  intros/transitions/outro entries. Missing imported files are marked.
- Player-only Instant Action starting-resource editing; blank keeps the map
  default, zero is valid, and the AI keeps its map-defined stockpile.
- Single-finger left click, drag selection, deliberate building placement and
  rotation, two-finger map pan, quick deselect and on-screen camera controls.
- Android music/effects/brightness/pan/FPS/keep-awake settings; native combat and
  response audio, MP3 speech decoding and soundtrack suspension during dialogue.
- ZIP/folder game import, private game-data export and all-in-one diagnostics ZIPs.

## Install and update

Android 8.0/API26 or later, **ARM64**, GLES3.0 GPU. APK targets SDK35 and has
16 KiB-aligned native libraries. No x86, 32-bit or PC-crossplay support is claimed.

Install `Reigncarnated-0.1.apk`. It updates the existing **IA City Test** app with
the same package/signer and displays the launcher name **Reigncarnated**. Keep the
original game archives outside app storage. Do not uninstall or clear data to
update: that removes imported files and local progress. Import a legally owned
complete installation separately; **no original game data is included**.

See [player guide](PLAYER_GUIDE.md) for required folders, controls and reporting.
Version name **0.1**, Android version code **32**. Source revision, library hashes,
build instructions, progress, this guide and provenance are embedded in the APK.
The release includes SHA256SUMS, a build receipt and matching source/relink ZIP.

## Known limitations

Complete continuous campaign playthroughs and all dialogue/ending sequences still
need phone verification. Direct mission access is a testing facility, not proof
that objectives are completable. Individual tests keep separate progress and
return to the main menu on mission transitions.

Local mid-mission save/restore is not implemented. Network multiplayer and PC
crossplay are not implemented. Some active textured draws still reject invalid
UVs; rendering, shroud/lighting, broad device coverage and long-session behavior
need further work. Controller changes are deferred. The release does not claim
new phone FPS results: the previously reported roughly 120 FPS session was APK28.

## Validation

Host CTest suites: 68 general checks and 13 native foundation checks. Java import,
ZIP export, diagnostics, touch, report retention, music, campaign-selection and
resource-input checks are run for this release. The original engine's campaign
handoff and individual-test return paths, plus player-only 44,444/zero/default
stockpiles with an unchanged AI stockpile, have real-engine host fixtures.

The release source/relink kit includes both application libraries' exact portable
link inputs and pinned FFmpeg n6.1.1 source/configuration. It is extracted to a new
path and relinked to verify identical stripped library hashes. The signed APK is
independently checked for identity/signer, version, CRC, source/docs, native ABI,
static libc++, alignment and absence of game data/keys. See the build receipt for
actual results; phone execution of version 0.1 remains a separate acceptance step.

## Feedback

Report app version, phone model, Android version, imported content, map/mission,
launch mode, repeatable steps and expected/actual behavior. Use **Report / data**
or in-game **Debug → Collect diagnostics ZIP (all in one)** after the issue.
Keep owned game archives, signing files and unrelated personal recordings private.
See [roadmap](ROADMAP.md), [build instructions](RECOVERY_BUILD.md) and
[source/provenance notices](SOURCE_PROVENANCE.md).

# Reigncarnated

**Dark Reign 2 on Android — native ARM64, offline early access.**

## 0.1 release candidate

[**Download APK**](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/1b55b7cab7d571421a64fc192bac5b970563a490/builds/v0.1/Reigncarnated-0.1.apk) ·
[Release notes](docs/RELEASE_0_1.md) ·
[Source/relink ZIP](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/1b55b7cab7d571421a64fc192bac5b970563a490/builds/v0.1/Reigncarnated-0.1-source-relink.zip) ·
[Checksums](builds/v0.1/SHA256SUMS.txt) ·
[Player guide](docs/PLAYER_GUIDE.md) · [Roadmap](docs/ROADMAP.md)

Android **8.0+**, **ARM64**, **GLES3.0**. Version **0.1**, Android version code **32**.
The existing IA City Test app updates in place and is now labeled **Reigncarnated**.
Install over it without uninstalling to retain imported data and progress.

**Game data is not included.** Import your legally owned installation separately.
Keep the original archives outside app storage. This is a community source port;
no publisher endorsement or ownership of the original title is claimed.

## Available in 0.1

- Original animated menu and offline Instant Action setup with practice/one original AI.
- Campaign starts plus individual mission/cutscene testing without unlock requirements.
- Player-only starting-resource editing for Instant Action; map defaults/AI preserved.
- Native GLES rendering with original **10 Hz simulation**, independent of screen refresh.
- Touch selection, drag selection, anchored building rotation/confirmation and two-finger pan.
- Android settings, native combat/unit audio, MP3 speech support and music arbitration.
- ZIP/folder import, private data backup/export and all-in-one diagnostics.

The known campaign metadata lists 31 sequence entries, including 12 separately
named intros/transitions/outro entries. Availability depends on imported files;
embedded cinematics stay in their mission scripts. Individual tests keep separate
progress and return to the menu on transitions.

## Early-access limits

Complete campaigns and every dialogue/objective/ending are not yet verified.
Mid-mission save/restore and multiplayer are not implemented. Remaining invalid
UV rendering, shroud/lighting, broad device/audio/long-session and thermal testing
are tracked openly. Controller work is deferred. No new 0.1 phone FPS claim is made.
See [known issues and validation](docs/RELEASE_0_1.md),
[release receipt](docs/builds/release-0.1.json) and
[campaign acceptance matrix](docs/CAMPAIGN_ACCEPTANCE.csv).

## Install and report

1. Download and install the APK on an ARM64 Android phone with GLES3.
2. Import complete game data containing packs, missions, worlds, sides, mods and library.
3. Open Instant Action or Campaigns. See the player guide for gestures and testing modes.

For an update, install over the existing matching-signed app. Uninstall/clear-data
removes imported files and progress; Android app backup is disabled. Keep a private
export externally. For bugs, provide version, device/Android, map/mission, launch
mode and reproduction steps, with the all-in-one diagnostics ZIP after the issue.
Use [bug reporting](.github/ISSUE_TEMPLATE/bug_report.md); do not upload game archives.

## Source, builds and release status

Active implementation is on **`recovery/native-arm64-v012`**. Main remains the
older baseline until [recovery-to-main PR #3](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/pull/3)
is reviewed and merged. The candidate APK,
source/relink archive and receipts are backed up on that branch. A draft GitHub
[Release](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/releases/tag/untagged-bfc407c1b0cb1dffc4a3)
contains the four verified assets and is reviewable separately by maintainers;
a draft is not a public/stable release. The APK/source links above are public
and pinned to the immutable delivery commit. See [GitHub verification](docs/builds/release-0.1-github.json).

The matching source/relink ZIP includes port source, complete pinned FFmpeg
n6.1.1 source/configuration and portable inputs for **both** native libraries.
Its extracted relink is checked against the APK's exact library hashes. No keys,
owned game packs, raw diagnostics or unstripped native libraries are included.
[Build instructions](docs/RECOVERY_BUILD.md) · [Progress](docs/RECOVERY_PROGRESS.md) ·
[Source/provenance and notices](docs/SOURCE_PROVENANCE.md) ·
[Repository/StarDroid comparison](docs/RELEASE_AUDIT_0_1.md).

SDK35, NDK27.2.12479018, API26, static libc++, original ABI flags and 16KiB alignment
are preserved. Original engine source is separately fetched at
`9ce9da5f21604310a997f0c41e9cd383f5e292c3` and verified against 1,791 file hashes.
Older v0.x milestones and audit documents describe historical checkpoints.

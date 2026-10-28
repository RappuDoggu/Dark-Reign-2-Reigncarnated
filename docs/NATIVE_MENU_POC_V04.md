# Native main-menu proof of concept v0.4

## What this build does

Import legally owned game data from one or several ZIPs, then enter the original
Shell::Main controls over native GLES scene rendering. The app bypasses Login
for this proof of concept. Original menu navigation other than Quit is disabled.
The existing music playlist/effects backend remains in use.

The shared ShellScene implementation renders the saved terrain and 95 model
placements, with original pose math and attachment transforms. It is now linked
into the Android JNI library, not confined to the host diagnostic. Android
requests continuous frames and recreates native resources after pause/resume.

The release objectives select Main::Init initially and Main::Enter at 0.5 seconds.
This milestone implements that camera sequence and holds the last entrance view
after the non-looping curve finishes. It does not switch back to the overhead
Main::Init view. The full cinematic scheduler, fade and ambient simulation remain
unimplemented. A moving entrance is demonstrated; perpetual ambient animation
is not claimed. The menu is visible during the entrance in this proof of concept.

## Private downloads

- [Reigncarnated-Native-Menu-v04.apk](https://chatgpt.com/library?file_id=libfile_4f8e7a2870d0819182746a7062c3dcc9&account_id=personal)
- [Reigncarnated-Native-Menu-Data.zip](https://chatgpt.com/library?file_id=libfile_3c8c19944d0c8191a7f5290d28569740&account_id=personal)

These are owner-account downloads, not public releases.

## Install

1. Install Reigncarnated-Native-Menu-v04.apk on an ARM64 Android device (API 26+).
2. Open it and choose **Select game data ZIP(s)**.
3. Select Reigncarnated-Native-Menu-Data.zip, prepared from the owner's uploaded
   installation, or select ZIPs containing base.zwp, shell.zwp and sprawl.zwp.
4. Wait for import and scene loading. Future launches reuse imported data.

Do not unzip first. Music is imported from the music folder or TrackNN files.
The matching data ZIP includes nine music tracks. No proprietary data is embedded
in the APK or committed to GitHub. A full installation folder can also be selected.
Older imports without worlds/sprawl.zwp prompt for new data.

Package org.reigncarnated.menu.v02, versionCode 4, signed using the recovered
v0.2/v0.3 development key. It is intended to update that app without uninstalling.
The top-right Report / data button shares diagnostics or starts another import.

## Verification

- All 63 ASan/UBSan host tests pass. Three local executable permission bits lost
  by the workspace were restored before the successful run.
- Folder importer: 28 checks pass.
- ZIP importer: 10 checks including the real supplied data. Covers multi-ZIP,
  case/wrapper handling, missing world, duplicates, traversal rejection,
  cancellation, invalid packs and rollback. Existing snapshots remain untouched
  on failure. ZIP entry CRC is checked during streaming reads.
- The actual companion ZIP was imported through the Java importer, then its
  resulting flat/native data layout loaded by the shared scene/menu renderer.
- Five scene/UI frames at 0, 3, 5.5, 6.5 and 9.5 seconds reproduce byte-for-byte
  across two EGL contexts. Entrance frames change; settled frames may be equal.
  Each draws 20 geometry batches and 9 font batches for the original menu.
- APK built for ARM64, signature verified, ZIP checksums passed, 16 KiB ZIP
  alignment checked. APK manifest reports versionCode 4 and minimum API 26.
- Existing audio code is retained and nine tracks import; audio playback in this
  new build and Android lifecycle behavior still require device confirmation.

Logs and SHA-256 receipts: [logs/native-menu-poc](logs/native-menu-poc).
No Android emulator or physical Fold 5 execution is claimed.

## Limits

Base-texture, unlit rendering; missing original lighting, full materials,
sky/water and ambient effects. Camera frustum remains an explicit diagnostic
setting. Model cycles use explicit frame sampling rather than full MeshEnt
clock/transition state. Missions, multiplayer and options are unavailable.

The source and host GPU probe share the same scene implementation. Future work
should start from device feedback on this APK rather than delaying delivery for
complete visual fidelity.

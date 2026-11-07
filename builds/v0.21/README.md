# APK 27: anchored placement and campaign handoff

[Download APK 27](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.21/Reigncarnated-Placement-Campaign-v27.apk)

Source: `99efd4bfd00b6143ef7c9d9562cfb3c6d7313390`.
SHA-256: `c0e76ac0c4e1e09c992a5fe5c3f5051e4c217d9d09d74a833b842d29a948a3af`.
Size: 9,356,415 bytes (8.9 MiB).

Updates APK 22-26's **Reigncarnated IA City Test** with the same private signer;
keep the installed app/data. No game packs, private diagnostics, keys or symbols
are included.

The owner selected these building controls: tap once to anchor, later single taps
to rotate at that position, double-tap to confirm. Anchored taps never move the
pointer. Single rotations wait 300 ms to distinguish double tap; drag deliberately
repositions. Normal left click/drag selection and hold-to-choose a build command
remain. Placement hold confirmation and three-finger rotation are superseded.

Cinematic end restores gameplay camera if the cinematic camera still owns the
view, and pan is gated while cinematics/handoffs own it. Original Mission/Shell
requests now reach Android after the current frame, preserving original campaign
selection/ordering. Android snapshots/closes the old runtime and opens the next
selected target, or returns to menu. No mission name is hardcoded or skipped.

**JDA's invalid variable is not claimed fixed.** Its report was displaced by
later Sprawler/Instant Action testing. This build preserves two failed-mission
reports separately, with source/time/stack evidence, so successful missions cannot
overwrite them. Retest the campaigns and collect one new menu diagnostics ZIP.

55 gesture checks, 13 report checks, SDK 35 Java and 12 foundation tests pass.
Authored cinematic camera/pan restoration and original NextMission -> target
load -> final menu checks pass twice; reference Instant Action lifecycle passes
twice. Android handoff/gesture feel, retail story flow and new FPS remain phone
checks. Combat audio is owner confirmed from the earlier working build.

[Diagnostic review](../../docs/DIAGNOSTICS_REVIEW_V26.md),
[controls](../../docs/CONTROLS_BACKLOG.md),
[build instructions](../../docs/RECOVERY_BUILD.md),
[progress](../../docs/RECOVERY_PROGRESS.md),
[verified receipt](../../docs/builds/placement-campaign-v27.json).

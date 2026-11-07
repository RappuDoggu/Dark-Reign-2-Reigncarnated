# APK 24: touch controls and combat audio

[Download APK 24](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.18/Reigncarnated-Touch-Combat-Audio-v24.apk)

Source: `fa43ad3a7fa0fb750ff6a7721dafd5e2a44b766a`.
SHA-256: `7343d4c0f16d3136af49eaee2257727847e94868c6165a53132299c8ef219c84`.
Size: 9,323,647 bytes (8.9 MiB).

Updates APK 22/23's **Reigncarnated IA City Test** using the same private signer.
Keep the existing app and imported data. No game packs or signing keys included.

- One-finger tap left-clicks; drag selects with the original held left mouse.
- Hold 450 ms to activate build controls or confirm placement. Placement short
  taps aim only. Two fingers pan; three-finger tap rotates a building once.
- Fixes reproduced premature attack/impact sound cutoff; recovers disconnected
  AAudio output and records bounded resource/mixer diagnostics. Phone combat
  audio remains a test requirement.
- Campaign loading runs in a private mission process with persistent stage logs
  and own-app platform crash trace collection where Android provides traces.
  **The retail campaign crash is not yet diagnosed or claimed fixed.**
- Instant Action menu layout is unchanged from APK 23 at the owner's request.

After a crash, return/reopen the main menu and choose Report/data, then Collect
diagnostics ZIP. Upload that ZIP to diagnose the actual failing campaign stage.

[Build instructions](../../docs/RECOVERY_BUILD.md),
[progress](../../docs/RECOVERY_PROGRESS.md),
[verified receipt](../../docs/builds/touch-combat-audio-v24.json).

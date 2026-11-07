# APK 25: campaign path and first-cycle readiness

[Download APK 25](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.19/Reigncarnated-Campaign-Path-v25.apk)

Source: `7250200d75073ae0691bda0dbb5d5fc9259647af`.
SHA-256: `41ed2e9c6c310922e6e0e35762e53f07d13c4f990854385c37964fd9b1fafb37`.
Size: 9,331,839 bytes (8.9 MiB).

Updates APK 22/23/24's **Reigncarnated IA City Test** with the same private signer.
Keep the installed app and imported data; no re-import is required. No game data,
signing key or private diagnostics are included.

The uploaded J01 report reaches the simulation state, then stops with `Empty
engine path` before any simulation cycle. Filename splitting now accepts empty
optional texture names through the original engine's existing handling, without
OS lookup or changing identifier case/separators. Actual invalid OS file opens
and unsupported operations still fail explicitly. Readiness waits for a real
simulation cycle; fatal exceptions remain visible as FAILED for ZIP export.
The mission ELF now has a deterministic build ID for crash symbol matching.

The regression fails before this fix and passes after it; 12 foundation tests,
48 touch checks and reference/synthetic mission lifecycle tests pass. **Retail
J01 still needs phone validation**, and the historical main-menu native crash
has no confirmed root-cause fix. No phone FPS result is claimed.

APK 24's single-finger click/drag, hold-to-place, three-finger tap rotation and
combat sound lifetime fix are retained. Instant Action layout is unchanged.
Please retest J01; collect another diagnostic ZIP if it stops. If it runs,
collect after combat so the sound counters record attack/hit requests.

[Diagnostic review](../../docs/DIAGNOSTICS_REVIEW_V24.md),
[build instructions](../../docs/RECOVERY_BUILD.md),
[progress](../../docs/RECOVERY_PROGRESS.md),
[verified receipt](../../docs/builds/campaign-path-v25.json).

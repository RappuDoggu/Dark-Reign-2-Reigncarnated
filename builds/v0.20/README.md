# APK 26: cinematic viewport and campaign evidence

[Download APK 26](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.20/Reigncarnated-Cinematic-Viewport-v26.apk)

Source: `7254c6418b7bdf80575bc8f9d39eb57634a92f38`.
SHA-256: `1334c4cd8d09f38aee651c8c9abf85fa7142fa41e57f441fdfa53b86024ca0ab`.
Size: 9,340,031 bytes (8.9 MiB).

Updates APK 22-25's **Reigncarnated IA City Test** with the same private signer;
keep the app/data. No game packs, private diagnostics, keys or debug symbols are
included. The owner confirms Instant Action combat sounds work on the phone.

Original cinematic letterboxing needs smaller viewports and partial clears.
The adapter now implements these operations, preserving absolute TL positions
and surrounding pixels. Real GLES pixel checks and an authored original-engine
cinematic timeline pass. Unsupported operations remain explicit failures.
**Sprawler's retail intro still needs phone validation.**

The newest JDA journal reaches simulation, then an invalid VarPtr after about
three seconds. **Its root cause is not yet fixed.** This build captures the
operation and module-relative caller stack, plus cinematic command/cycle history.
Mission reports save automatically on frame failure and pause/exit, with exact
source/time receipts. The menu ZIP includes the last two distinct mission reports.
The old nested manual mission ZIP in the supplied report was from APK 24.

Retest Sprawler, return to menu, then test JDA. Collect one menu diagnostics ZIP
if either remains wrong. Current touch controls and Instant Action layout remain.
68 host tests, 12 foundation tests, real cinematic/viewport pixels, two-pass
reference mission lifecycles, SDK 35 Java compilation and 48 touch checks pass.
No new phone FPS or retail campaign completion result is claimed.

[Diagnostic review](../../docs/DIAGNOSTICS_REVIEW_V25.md),
[build instructions](../../docs/RECOVERY_BUILD.md),
[progress](../../docs/RECOVERY_PROGRESS.md),
[verified receipt](../../docs/builds/cinematic-viewport-v26.json).

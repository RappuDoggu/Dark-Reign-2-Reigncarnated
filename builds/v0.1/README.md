# Reigncarnated 0.1 — early-access release candidate

Android versionName **0.1**, versionCode **32**, ARM64/API26+/GLES3, target SDK35.
Package `org.reigncarnated.instantcity`, launcher **Reigncarnated**. Same signer
as APK22–31: update the existing IA City Test app without uninstalling/reimporting.

- `Reigncarnated-0.1.apk`: signed APK with embedded exact source/build/progress,
  release guide, player guide and provenance.
- `Reigncarnated-0.1-source-relink.zip`: matching port source, full pinned FFmpeg
  n6.1.1 source/licence/configuration and portable relink objects for BOTH current
  native libraries, plus the unsigned APK for replacing/re-signing modified libs.
- `SHA256SUMS.txt` and `build-receipt.json`: exact artifact hashes and acceptance.

Source: `2d4ea66bcb24daa44871e993c5c3e57c2b658d0e`; native implementation/compiled
inputs: APK31 source `e47230ebcb62a75b9d269f87afaaeffdfb1f1cd2`. Debug-stripped
portable inputs are relinked for the release, then relinked again from a separately
extracted ZIP with exact library SHA256 agreement. Native .text/.rodata remain
identical to APK31. No key, original game pack, unstripped library or raw diagnostic
is included. Import your original game data separately.

Host suites pass 68 + 13 tests and 216 Java checks. Real original-engine IA resource
and campaign/cinematic fixtures pass twice. Manifest, signature/update certificate,
source/docs, CRC, ARM64/static libc++, 16KiB native/ZIP alignment and private-data
exclusion are verified. See receipt for detailed evidence and unverified phone work.

Full campaigns, save/restore, remaining UV rendering issues, device/audio/long-session
acceptance and multiplayer remain open. This is not a complete-game/stable claim.
[Release synopsis](../../docs/RELEASE_0_1.md) · [Player guide](../../docs/PLAYER_GUIDE.md) ·
[Roadmap](../../docs/ROADMAP.md) · [Build](../../docs/RECOVERY_BUILD.md) ·
[Provenance](../../docs/SOURCE_PROVENANCE.md) · [Audit](../../docs/RELEASE_AUDIT_0_1.md).

A draft GitHub Release and recovery-to-main PR are separate review artifacts.
Neither publishing the Release nor merging main is implied by these files.

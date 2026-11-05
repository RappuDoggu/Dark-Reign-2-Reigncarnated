# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current testing build: APK31

Active implementation and APKs are on **`recovery/native-arm64-v012`**. The main
branch remains the older audit baseline. This checkpoint is an offline testing
build; no 0.1 stable release is declared.

[**Download APK31 — Campaign Testing (9.5 MB)**](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/983c52e50a05ce4d37433f4eb114d509875b082c/builds/v0.25/Reigncarnated-Campaign-Testing-v31.apk)

- Android package: `org.reigncarnated.instantcity`, **Reigncarnated IA City Test**.
- Android version: `0.25-campaign-testing`, version code **31**.
- Source: `e47230ebcb62a75b9d269f87afaaeffdfb1f1cd2`.
- SHA-256: `f1676ad16b9f0a4ed4cd4998f36ef24c73e0a4ed8eadd59929b4dca7752e5303`.
- Same private signer as APK22–30: install as an update to retain imported data.
- APK contains build/source receipts, instructions and progress in `assets/build/`.
  Signing keys and game data are not bundled; import your legally owned files.

## Available for testing

Original menu/city animation, offline Instant Action with one original AI,
native mission/cinematic execution, combat and unit audio, MP3 mission speech
support, music suspension during cutscenes/speech, Android settings and touch
selection/building placement. Original simulation remains 10 Hz.

**Campaigns** separates original campaign starts from unrestricted individual
mission/cutscene tests. The catalog supports all declared imported sequence
entries; the known owned-package metadata lists 31. Missing files are marked.
Individual tests use separate progress and return to the menu on transitions.
**Instant Action → Resources** edits only the local player's starting stockpile;
blank preserves map default and zero is supported. AI starting amounts are unchanged.

The owner reports Instant Action appears completable and missions now load with
cutscenes. Those reports do not establish complete campaign playthroughs. APK31
host checks include 68 general tests, real player-only resource initialization
with an AI, original campaign handoff and individual menu-return fixtures.
Phone acceptance of APK31's new UI remains pending. Detailed evidence and limits:
[APK31 receipt](docs/builds/campaign-testing-v31.json),
[build/test notes](builds/v0.25/README.md),
[recovery progress](docs/RECOVERY_PROGRESS.md).

## Proposed public prerelease

**`v0.0.1-alpha`** is the proposed first public testing tag; it has not been
created or published by this work. See the concrete
[draft release notes](docs/releases/v0.0.1-alpha.DRAFT.md). APK31's internal Android
version remains stated above; do not rename it as a stable 0.1 build.

Full campaign completion, local save/restore, remaining active-texture NaN UV
defects, dialogue acceptance and broader device/long-session testing remain open.
Multiplayer is offline in this build. Follow the
[release acceptance plan](docs/RELEASE_0_1_AND_MULTIPLAYER.md) and
[campaign verification matrix](docs/CAMPAIGN_ACCEPTANCE.csv).

The earlier [dependency/provenance audit](docs/DEPENDENCIES.md) records an
unresolved upstream redistribution-authority question. This checkpoint does not
resolve it. Public-release preparation also needs current FFmpeg corresponding
source/relink materials; the historical v0.5 relink packager is not the current
mission-library release kit. No new public-distribution rights are asserted here.

## Build and provenance

Use [the reproducible recovery instructions](docs/RECOVERY_BUILD.md): SDK35,
NDK27.2.12479018, Java17/21, CMake, Ninja, Python and the pinned FFmpeg n6.1.1
recipe. The pinned original source is fetched separately at
`9ce9da5f21604310a997f0c41e9cd383f5e292c3` and checked against the repository's
1,791-file inventory. Preserve API26, ARM64/static libc++, original ABI flags,
16 KiB native alignment and the 10 Hz simulation.

Historical v0.3–v0.7 documents and the earlier `dev/native-arm64-primitives`
checkpoint describe previous milestones. The recovery branch, source receipts
and current progress record identify the implementation delivered now. Keep
signing keys, owned asset archives and raw device diagnostics private.

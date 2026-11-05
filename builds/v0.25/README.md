# APK31: campaign and resource testing

Update **Reigncarnated IA City Test**, application ID `org.reigncarnated.instantcity`,
version 31 / `0.25-campaign-testing`, same private signer as APK22–30. Existing
imports remain in place; game data and signing material are not bundled.

Source `e47230ebcb62a75b9d269f87afaaeffdfb1f1cd2` on
`recovery/native-arm64-v012`. Source/library hashes, instructions and progress
are embedded in `assets/build/`. See `docs/RECOVERY_BUILD.md`,
`docs/RECOVERY_PROGRESS.md` and `docs/builds/campaign-testing-v31.json`.

## Phone testing

- **Campaigns → Start JDA campaign / Start Sprawler campaign** uses each original
  opening and follows normal campaign transitions with the normal profile.
- **Campaigns → Choose individual mission / cutscene (testing)** lists every
  declared imported campaign entry, without progression locks. Entries identify
  faction and mission/cutscene type. Missing data stays visible with launch disabled.
  Individual tests keep separate progress and return to the menu on mission
  transitions. Embedded cinematics remain part of their mission scripts.
- **Instant Action → Resources** edits your team's starting resources only. Blank
  or **Map default** retains the map's stockpile; zero is valid; maximum 1,000,000.
  AI teams and campaigns retain their original resource amounts. The setting is
  remembered for later Instant Action launches.

The authored catalog fixture covers the 31 known sequence targets, including
12 separately named cutscene/transition entries. This validates access and order,
not full retail mission completion. Keep using `docs/CAMPAIGN_ACCEPTANCE.csv` for
playthrough evidence; individual testing does not certify continuous campaigns.

Checks pass: 68 general host tests, 16 campaign selection and 16 resource input
checks; retained 55 touch, 13 report and 14 music policy checks; native resource
bounds and campaign exclusion; normal campaign handoff and individual menu-return
fixtures twice. Real IA checks before the first simulation cycle show local
44,444 / AI 20,000, local 0 / AI 20,000 and map-default local 20,000 / AI 20,000,
two runs each, with clean ASan shutdown. SDK35 Java, native ARM64 link, same signer,
static libc++, 16 KiB alignment and embedded source/docs are verified.

Original 10 Hz, engine flags, audio, rendering stream and touch behavior remain.
New phone UI, full campaigns and APK31 phone FPS still require acceptance. No
new multiplayer, local save/restore or NaN UV fix is claimed.

# APK30: Android options

Update **Reigncarnated IA City Test**, `org.reigncarnated.instantcity`, same
private signer as APK22–29. Version 30 / `0.24-android-options`; existing imported
data is retained. No keys, raw diagnostic captures or game-data packs are included.

Source `9d4bbe08c9fda6e8cd0a61f2b2e77cebd5e59bb2` on
`recovery/native-arm64-v012`. Exact source/library hashes, build instructions and
progress are embedded in `assets/build/`. See `docs/RECOVERY_BUILD.md`,
`docs/RECOVERY_PROGRESS.md` and `docs/builds/android-options-v30.json`.

Native main-menu Configuration and in-game Options/Escape now open the same
Android settings as the toolbar. This avoids the desktop Video tab's missing
variable/type failure. The desktop tabbed dialog is replaced; Android Back returns
to the main menu. Verify options Apply/Cancel and persistence on the phone.
APK29 MP3 dialogue and music suspension are retained; the new ZIP is from APK28
and cannot verify those speech changes. Please test the Sprawler opening here.

Fixes three AI ownership defects found by a real one-AI host capture and shutdown:
random-personality array deletion, private location polymorphic destruction and
placement ClusterInfo array deletion. Allocation/type mismatch checks stay on.
Original ABI/flags, 10 Hz, rendering and touch behavior are retained.

Checks: 68 general host tests, 51 menu-runtime checks, 55 touch/13 reports/14 music
policy checks, original options/cinematic/raw-mesh/camera/campaign handoff twice,
real AI gameplay capture and clean shutdown under ASan. Same signer, ARM64,
static libc++, 16 KiB ELF/ZIP alignment, embedded docs/source and privacy verified.
No APK30 phone performance, fixed NaN UVs, complete campaign, save/restore or
multiplayer claim is made. Follow `docs/RELEASE_0_1_AND_MULTIPLAYER.md` and
`docs/CAMPAIGN_ACCEPTANCE.csv` for offline 0.1 and later network acceptance.

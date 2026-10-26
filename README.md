# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current checkpoint: one-folder setup v0.3

The Android app now imports its menu archives and music from one selected,
legally owned installation folder. It remembers setup, plays music automatically,
and keeps file management and diagnostics in Settings. v0.3 updates v0.2 in place.

Original Login, the release menu layout, volume settings, Logout and Quit remain
available. The backdrop is still static and gameplay-dependent actions disabled.
The original camera spline now compiles and samples the real shell path on host;
terrain/model rendering and cinematic integration remain.

58 native tests and 24 real-installation importer checks pass. The signed ARM64
APK is ready for device testing; Android folder-picker/audio behavior is pending.

[Current implementation and test instructions](docs/FOLDER_SETUP_V03.md) •
[Prior main-menu checkpoint](docs/MAIN_MENU_V02.md)

[Source and installation recovery](docs/RECOVERY_ARCHIVES.md): neither requires
the original desktop. Active implementation lives on `dev/native-arm64-primitives`.
Older documents in `docs/` describe historical checkpoints, not current status.

## Build

Use the pinned upstream snapshot `9ce9da5f21604310a997f0c41e9cd383f5e292c3`
from https://github.com/grasmanek94/darkreign2 outside this repository.

```sh
cmake -S . -B build -DREIGN_UPSTREAM=/path/to/upstream -DREIGN_SANITIZE=ON
cmake --build build --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build --output-on-failure
```

Set ANDROID_SDK_ROOT, ANDROID_NDK_HOME and REIGN_UPSTREAM, then run
`bash tools/package_menu.sh` for the private menu APK. Keep the private debug
keystore for subsequent compatible v0.2 updates.

## Scope and provenance

Preserve original engine behavior while adapting rendering, platform services
and input. Full mission simulation/rendering, gameplay audio and AYN Thor
secondary-display integration remain. The working Winlator version is a
behavioral reference, not native-port completion.

Upstream source, proprietary game assets, signing keys and engine-derived
binaries stay outside Git. Reviewed stagers generate source overlays from the
pinned source. Public source availability is not a redistribution license.
See [porting audit](docs/PORTING_AUDIT.md) and [dependencies](docs/DEPENDENCIES.md).

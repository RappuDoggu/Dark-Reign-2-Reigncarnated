# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current checkpoint: main menu and audio v0.2

Original Login now hands off to the release main-menu controls, using a temporary
static backdrop. Options exposes effects/music volume, Logout returns to profiles,
and Quit closes the app. Original WAV interface effects and optional imported OGG
music are connected to Android playback. Gameplay-dependent actions are disabled;
this is not a playable mission or the original animated shell.

57/57 sanitizer host tests pass. Real-asset login/menu/logout flows and software
GLES rendering pass. The ARM64 APK is packaged separately for private device
validation; on-device main-menu behavior and audio remain to be confirmed.

[Implementation, limitations and test instructions](docs/MAIN_MENU_V02.md)

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

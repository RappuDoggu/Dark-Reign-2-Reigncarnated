# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current checkpoint: native shell camera and terrain loading

The target is the original live 3D main-menu scene running natively on Android.
The owner confirmed the v0.3 folder import, menu, music and effects on the Fold 5.
Its released APK still has a static backdrop; gameplay-dependent actions remain
unavailable.

The original BookmarkObj curve runtime now handles all 18 shell bookmarks.
The original terrain loader reads the shell's 60×60 map, with all 20 terrain
textures decoded through the original bitmap manager. The folder importer now
also preserves the available Sprawl/JDA/Sprawler scene packs. These native data
components compile for ARM64; terrain/model drawing and cinematic integration
are not yet wired into the APK. No video-background implementation is included.

60 host tests and 34 importer checks pass. This is a source checkpoint, not a
new APK release.

[Native shell implementation and validation](docs/NATIVE_SHELL_PROGRESS.md) •
[Released v0.3 folder setup](docs/FOLDER_SETUP_V03.md) •
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

# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current checkpoint: native main-menu proof-of-concept APK v0.4

The ARM64 Android proof of concept imports one or more game-data ZIPs and opens
the original main-menu layout directly over the native 3D shell scene. No login
is required. The original entrance camera runs, then holds its final view.
Menu music and effects use the existing Android audio path. Options, logout and
mission destinations are disabled for this milestone; Quit remains available.

Import requires base.zwp, shell.zwp and sprawl.zwp. The previous compact v0.3 ZIP
may lack the world pack. Full installation ZIPs, enclosing folders and compact
ZIPs are supported. Music is detected automatically. Files are copied once;
later launches reuse app storage. Folder import remains available.

The scene is an unlit proof of concept: original lighting/material policy,
sky/water, ambient effects and complete cinematic/object simulation remain.
This is real scene geometry and original camera math, not video playback.

The signed APK, ZIP import with actual data, combined scene/UI GLES rendering,
and 63 host tests are verified. Execution on the owner's Fold 5 is pending.
See [v0.4 installation and validation](docs/NATIVE_MENU_POC_V04.md).

[Previous model-pose checkpoint](docs/GOD_POSE_PROGRESS.md) •
[Released v0.3 folder setup](docs/FOLDER_SETUP_V03.md)

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

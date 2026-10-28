# Reigncarnated

Native ARM64 Android source-port work for Dark Reign 2.

## Current checkpoint: original model poses in the shell scene

The target remains the original live 3D main-menu scene running natively on
Android. The released v0.3 APK has working menu controls, folder import and audio,
confirmed by the owner on the Fold 5. Its backdrop remains static.

The host GLES diagnostic now renders the original shell terrain and all 95 saved
model placements (28 unique models), including four HQ attachments, along the
original Main::Enter camera path. Original interpolation, hierarchy transforms
and skinning evaluate model poses. Three scene samples reproduce exactly across
graphics-context recreation. This is an unlit diagnostic: original material and
blend policy, lighting, sky/water, effects, stateful animation/cinematic timing
and the continuous Android menu scene loop remain to be connected.

All 63 host tests pass. The pose evaluator and staged original math compile for
Android ARM64. Real-asset checks cover 449 model roots and 234 animation cycles.
No new APK is released at this checkpoint.

[Model poses and shell scene validation](docs/GOD_POSE_PROGRESS.md) •
[Earlier complete model tree checks](docs/GOD_MODEL_TREE_PROGRESS.md) •
[Earlier model prefix boundary](docs/GOD_MESH_PROGRESS.md) •
[World rendering boundary and validation](docs/WORLD_RENDER_PROGRESS.md) •
[Camera/terrain data checkpoint](docs/NATIVE_SHELL_PROGRESS.md) •
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

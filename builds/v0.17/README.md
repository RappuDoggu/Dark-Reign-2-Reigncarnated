# APK 23: original offline menus and Android options

[Download APK 23](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.17/Reigncarnated-Original-Menus-Options-v23.apk)

Source: `17c61ca85c0d3234895cb79f23e71e204c27c3b8`.
SHA-256: `5e2336da039c13f122a9d62fddcfa83033f65b5061a9595697430843187fd487`.
Size: 9,299,071 bytes (8.9 MiB).

Updates APK 22's **Reigncarnated IA City Test** application using the same private
signer and `org.reigncarnated.instantcity` identity. Keep the existing app/data;
no re-import is required. No signing key or proprietary game data is included.

- Original offline lobby layout and Map/Back/Launch icons, one-AI match options,
  map chooser and imported preview when present; advanced team/lobby options
  remain pending on screen.
- Android music/effects volume, pan speed, brightness, keep-awake and FPS display.
- High supported graphics, full meshes, 32-bit textures and real trilinear
  filtering. Compatibility viewport remains 640 x 480; unsupported desktop
  graphics features are not advertised as enabled.
- Correct single-player campaign loading branch, visible loading stage/history
  and a loading diagnostic ZIP. Retail campaign startup needs device validation.
- Fog/lighting reports and a distance-fog comparison retaining original shroud.
  The reported phone tint/darkness is not claimed resolved without device data.

Build instructions and remaining acceptance checks:
[RECOVERY_BUILD.md](../../docs/RECOVERY_BUILD.md).
Validation/progress: [RECOVERY_PROGRESS.md](../../docs/RECOVERY_PROGRESS.md).
Receipt: [original-menus-options-v23.json](../../docs/builds/original-menus-options-v23.json).

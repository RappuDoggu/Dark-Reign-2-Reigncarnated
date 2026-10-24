# Source-port audit — initial inventory

**Date:** 2026-10-24  
**Upstream:** https://github.com/grasmanek94/darkreign2 (default branch `master`)  
**Confidence:** Directory and README findings are observed; technical dependency conclusions remain hypotheses until inspected at file level and compiled.

## Verified upstream findings

The repository has a Visual Studio solution `dr2.sln`, `settings.h`, a `3rdparty` directory and separate project directories:
- Application/startup: `appdr2`, `main`, `game`, `apps`
- Gameplay: `coregame`, `coregame_ai`, `coregame_interface`, `coregame_objects`, `coregame_orders`, `coregame_tasks`, `coregame_particles`
- Rendering: `graphics`, `appmesh`
- Platform/interfaces: `system`, `interface`, `sound`, `util`
- Network: `multiplayer`, `styxnet`, `won`, `wonlib`

Within `graphics`, observed `graphics.vcxproj`, `camera.cpp`, `bitmap.cpp`, `bucket.cpp`, `light.cpp` and other original rendering components.

Upstream `readme.txt` states it was updated for VS2015 and that LAN and single-player work. This is an upstream claim **not yet independently verified**.

`settings.h` carries a 1997–1999 Pandemic Studios copyright notice. GitHub metadata reports **no detected repository license**. Public access is not a license grant.

## Immediate investigations

1. Inspect `dr2.sln`, `appdr2`, `main`, and `system` to identify the actual executable entry point and build target graph.
2. Inspect `graphics` for Direct3D device lifecycle, draw calls, fixed-function pipeline assumptions, and proprietary image/mesh formats.
3. Audit `3rdparty`, `won` and `wonlib` for dependency versions, licenses and features removable from a single-player build.
4. Search for Win32 types, memory layout assumptions, inline x86 assembly, and compiler extensions.
5. Separate pure simulation code from platform APIs and establish testable compile targets.
6. Determine game data loading contract without checking proprietary assets into this repo.

## Porting decisions (provisional)

- Initial target: **Android arm64-v8a**, single display, one single-player mission.
- Keep original game simulation and resource formats.
- Use CMake / Android NDK for builds. SDL3 and OpenGL ES/Vulkan are candidates, **not yet chosen**.
- Dual-display AYN Thor UX starts only after a functioning single-screen engine.
- Keep Winlator as behavior baseline only: owner has confirmed working gameplay on Android.
- No claims of APK success until CI builds and on-device tests occur.

## Milestone evidence expected

- Build commands and captured logs.
- Dependency and licensing inventory.
- Compiling portable subsystem, followed by engine startup.
- Android runtime logs and reproducible test scene.
- User-owned asset import procedure, never bundled copyrighted game assets.

## Important constraint

Do not import third-party upstream code to this repository pending a provenance/licensing decision. A separate, original adapter/build harness can be developed meanwhile.

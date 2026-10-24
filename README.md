# Reigncarnated

Dark Reign 2 preservation and native Android source port.

## Objective

Produce a native ARM64 Android build of the original C++ engine, preserving gameplay, AI, missions, balance and asset formats. AYN Thor dual-screen tactical controls follow working single-screen gameplay. The owner's working Winlator installation is a behavioral baseline; this project builds native components.

## Verified status — 2026-10-24

- Original CRC and RNG sources compile with GCC 13 and Android NDK r27c Clang for arm64-v8a.
- Host original-engine primitive tests, platform-service tests and synthetic PACK format tests pass. ARM64 runtime remains untested.
- An original NativeActivity/EGL/GLES3 diagnostic shell links and packages into a signed, aligned local APK. It runs primitive checks by design; it does not initialize the full engine or render a game scene.
- GitHub host tests and ARM64 build/package CI both pass; [verified run and downloadable logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110369623).
- No playable Android port, original main menu or mission is demonstrated yet.

See [build results and failures](docs/BUILD_STATUS.md), [source audit](docs/PORTING_AUDIT.md), [dependencies and provenance](docs/DEPENDENCIES.md), and [build commands / next engineering work](docs/ANDROID_PORT_PLAN.md).

## Source and rights

Primary reference: https://github.com/grasmanek94/darkreign2 at `9ce9da5f21604310a997f0c41e9cd383f5e292c3`.
Additional reference: https://github.com/medwards/darkreign2 at `fad7b9fb3f50339c81c6d11a09144fca99d03fb7`.

Public source access is not a redistribution license. Upstream source stays in a separate temporary checkout. This repository contains original harnesses, adapters, tests, tooling, reports and metadata; no copied upstream implementations or proprietary game assets. Source-derived executables/APKs are built locally and are not published while redistribution rights remain unresolved. CI publishes logs only.

Use `bash tools/fetch_upstream.sh /tmp/dr2-reviewed` to obtain the reviewed source separately. The build validates content hashes before using it. Tools, SDKs, dependencies, game installation files, build products and debug signing keys stay outside versioned source.

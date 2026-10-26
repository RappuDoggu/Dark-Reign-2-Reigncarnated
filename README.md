Latest: real original Login configuration activates and emits draw batches; original profile persistence passes. Shell mission transition and Android menu host remain. [Current verified status](docs/LOGIN_PROFILE_PORT.md).

Latest: original Listbox/Slider/Menu controls compile; live list selection passes. Login configuration reaches UI Sound. [Status](docs/LIST_MENU_CONTROLS.md).

Latest: original cursors and console colors; 51/51 host tests pass. Login configuration reaches Listbox dependency. [Details](docs/CURSOR_PORT.md).

Current increment: original menu configuration dispatch; 50/50 host tests pass. Real Login config reaches cursor creation after loading 128 key names and 12 modifiers. No main-menu APK yet. See [evidence](docs/MENU_CONFIGURATION.md).

# Reigncarnated

Dark Reign 2 preservation and native Android source port.

## Latest verified progress — 2026-10-26 UTC

Original root, button, static-text and modal message-box controls now link and
execute in a host test. The test activates and draws the original controls,
renders synthetic font text, dispatches a dialog-button notification, then
verifies modal release and destruction. Host CI passes 47 tests; ARM64 compilation and GLES/package CI also pass
in [run 40267250825](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40267250825). Original Babel
localization and console/VarSys dispatch are also integrated into that target.

This is not yet the original Login/Shell menu or a new main-menu APK. Production
root lifetime, remaining widgets, user profiles, application commands, real menu
configuration and Android lifecycle integration remain. See
[verified behavior and remaining dependencies](docs/LIVE_FRONTEND_CONTROLS.md).
Historical checkpoints below describe earlier increments; workspace-installed
tools are transient and must be rechecked before reuse.

## Objective

Produce a native ARM64 Android build of the original C++ engine, preserving gameplay, AI, missions, balance and asset formats. AYN Thor dual-screen tactical controls follow working single-screen gameplay. The owner's working Winlator installation is a behavioral baseline; this project builds native components.

## Verified status — 2026-10-24

- Original CRC and RNG sources compile with GCC 13 and Android NDK r27c Clang for arm64-v8a.
- Host original-engine primitive tests, platform-service tests and synthetic PACK format tests pass. ARM64 runtime remains untested.
- An original NativeActivity/EGL/GLES3 diagnostic shell links and packages into a signed, aligned local APK. It runs primitive checks by design; it does not initialize the full engine or render a game scene.
- GitHub host tests and ARM64 build/package CI both pass; [verified run and downloadable logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110369623).
- Uploaded community installation checked: 88 archives / 3,763 entries read natively; original tokenizer processes 1,165 archived plus 16 loose configs without errors. See [installation audit](docs/INSTALLATION_AUDIT.md).
- Original-only ARM64 [asset-test APK and instructions](docs/ANDROID_ASSET_TEST.md) are ready for on-device testing. Full original-engine startup remains unimplemented.
- Original config parser, binary-block layer and run-code lifecycle now compile; J01's real type/object/strategic data parse. Initial GLES TL rendering passes offscreen pixel tests. [Progress and limits](docs/CONFIG_RENDER_PROGRESS.md).
- [Renderer Test v0.3](docs/ANDROID_RENDERER_TEST.md) checks the GPU path and previews the owner's cursor atlas. It does not initialize the full game.
- No playable Android port, original main menu or mission is demonstrated yet.

See [build results and failures](docs/BUILD_STATUS.md), [source audit](docs/PORTING_AUDIT.md), [dependencies and provenance](docs/DEPENDENCIES.md), and [build commands / next engineering work](docs/ANDROID_PORT_PLAN.md).

## Restored workspace checkpoint — 2026-10-25

Official REA6.8.0, Ghidra12.1.4 and full JDK21 are installed in an isolated workspace prefix. Standalone Ghidra decompiled the owner executable startup and matched the seven WinMain registrations; REA provider startup remains blocked by the Work PID/procfs mismatch. Original control declarations and coordinate/border methods now compile on host/ARM64; 26 host tests pass with the documented local LeakSanitizer limitation. [Tool output and reproduction](docs/REA_SETUP.md), [control integration and remaining menu gate](docs/CONTROL_PROGRESS.md). No new original-menu APK is ready.

## Source and rights

Primary reference: https://github.com/grasmanek94/darkreign2 at `9ce9da5f21604310a997f0c41e9cd383f5e292c3`.
Additional reference: https://github.com/medwards/darkreign2 at `fad7b9fb3f50339c81c6d11a09144fca99d03fb7`.

Public source access is not a redistribution license. Upstream source stays in a separate temporary checkout. This repository contains original harnesses, adapters, tests, tooling, reports and metadata; no copied upstream implementations or proprietary game assets. Source-derived executables/APKs are built locally and are not published while redistribution rights remain unresolved. CI publishes logs and the original-only asset-test APK; engine-derived binaries remain excluded.

Use `bash tools/fetch_upstream.sh /tmp/dr2-reviewed` to obtain the reviewed source separately. The build validates content hashes before using it. Tools, SDKs, dependencies, game installation files, build products and debug signing keys stay outside versioned source.

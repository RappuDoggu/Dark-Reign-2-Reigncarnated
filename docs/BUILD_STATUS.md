# Build status — 2026-10-24

## Milestones

| Milestone | Evidence | Status |
|---|---|---|
| M1 original subsystem with contemporary compiler | Original system/crc.cpp and random.cpp compile/link with GCC 13.3; host tests pass | Achieved for these primitive subsystems; broader source/license audit continues |
| M2 original portable components on Android ARM64 | Same original sources compile/link using NDK r27c Clang, API26, arm64-v8a; readelf confirms AArch64 | Achieved for these components; not the simulation |
| M3 APK initializes original engine and renders a scene | Diagnostic APK packages and verifies; original primitives linked, GLES shell implemented | **Not achieved**: no runtime test, no full engine initialization, no original scene |
| M4 original single-player mission | No game assets supplied or simulation port built | Not achieved |
| M5 Thor tactical display | No Thor/display runtime access | Not achieved |

## What was built and tested locally

- Linux x86_64: GCC 13.3, CMake 4.4.4. Three CTest targets PASS: original_engine_primitives, platform_services, pack_metadata_inspector. ASan/UBSan with fail-on-error enabled; LeakSanitizer disabled only because this sandbox cannot inspect the required /proc task data.
- Original engine checks: CRC known vector, existing Intro run-code ID, bytewise oracle, all 257 buffer split points with nonzero seed, ASCII and extended-byte folding, UTF-16 ordering, 32-bit RNG golden states, seeded sequence repeatability, bounded Integer and Float results, zero-modulus advancement, original packed header size56. These do not establish cross-platform whole-game determinism.
- Platform checks: lifecycle transitions including destroyed state, monotonic timing, exact bounded file read, missing files, parent traversal, absolute paths, oversize input and symlink escape rejection.
- Asset inspector: three unittest methods cover synthetic NORK/PACK v1 metadata, truncation at every byte boundary and corrupt signatures/versions/counts/offsets/ranges. No real user .zwp has been tested.
- Android: NDK 27.2.12479018 (r27c), API26, C++17, arm64-v8a. Original primitives, engine_smoke, platform_smoke and libreign_diagnostic.so compile/link. NativeActivity symbol exported; ELF LOAD segments have 0x4000 alignment.
- Android package: SDK platform35/build-tools35.0.0, Java17. Diagnostic APK generated, zipalign checked with 16KiB page setting, apksigner verification PASS with v2/v3 signatures. Android manifest declares ARM64 native library, API26 floor and GLES3. APK is debug signed and has no bundled game assets.
- Runtime: no adb, Android ARM64 emulator, connected Android device, or Windows runtime. No Android binary was executed. No graphics/performance/gameplay claim follows from packaging.

## Failures and resolutions

| Attempt | Actual failure | Resolution / remaining limit |
|---|---|---|
| Bare original CRC compilation | U32/U8/CH not declared | Isolated fixed-width forced header |
| Original std.h forced include on Linux | fatal windows.h not found | Avoid global Windows/DirectX prelude for portable targets |
| Original RNG on portable compiler | MSVC _rotr unavailable by inspection | Original C++32 rotate adapter; golden sequence validated |
| Extended-byte CRC regression | ASan global-buffer-overflow / UBSan index -128 at crc.cpp:176 | crc.cpp-only -funsigned-char; regression passes |
| Host sanitizer execution initially | LeakSanitizer cannot inspect /proc task info | ASAN_OPTIONS=detect_leaks=0 locally; ASan/UBSan still enabled; CI keeps default leak checks |
| First SDK archive extraction | Parent output directories absent | Create directories before unzip; SDK tools subsequently ran successfully |
| Initial ELF layout | 4KiB LOAD alignment despite APK alignment option | Enable NDK ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES; readelf verifies 16KiB LOAD alignment |
| Original Windows engine build | Windows SDK/MSVC/MSBuild unavailable | No complete Windows build attempted or claimed; original v140/SDK and hardcoded library paths audited |
| PTree portable compile probe | system/file.h includes unavailable io.h | Captured ptree-probe-failure.log; next slice needs File/Dir adapters, not parser rewrites |
| First GitHub Android CI run | sdkmanager command absent on runner (exit127); host job passed | Added checksum-verified standalone Google SDK/NDK installer; subsequent host and Android jobs PASS |
| Full Android engine boot | Main/GameGod require unported Win32, D3D7, input, audio, FileSys and other services | Explicitly deferred to next dependency slice; no fake stubs claiming initialization |

## Reproducibility and logs

Build commands: ANDROID_PORT_PLAN.md. Source hashes: cmake/VerifyUpstream.cmake. Machine-readable dependency candidates: upstream-inventory.json. `tools/inspect_pack.py installation/file.zwp` emits metadata only.

`docs/logs/` contains captured configure/build/test logs, the pre-fix CRC sanitizer failure, the sandbox LeakSanitizer failure, signed APK verification, APK manifest/ABI metadata and ELF headers. Absolute temporary paths reflect the actual execution environment.

GitHub workflow: `.github/workflows/native-primitives.yml`, with host and ARM64 build/package jobs. Only compilation/test/package logs may be uploaded. No upstream source, game data, object files, static libraries, native executables, debug keys or APKs are uploaded. CI run results are recorded below; local results above are independently observed.

## Next concrete engineering work

1. Gather Android diagnostic launch/pause/resume/window-recreation logs and actual EGL output on an ARM64 device, subject to source-derived binary handling.
2. Compile the original PTree/FScope/BlockFile/FileSys dependency slice with native types and file/memory/assertion adapters. Preserve archive/config search semantics.
3. Add deterministic math/FPU and serialized-layout fixtures before bringing simulation across.
4. Implement and validate the first Vid/Bitmap GLES draw path against Windows captures; port Sound/Input internals behind their original interfaces.
5. Resolve source/middleware distribution rights before publishing a downloadable engine build. Continue original adapters/tooling independently.

Initial CI evidence: commit `eea4dd3282dbd34526fa7ebe8e1dbdc6182a4815`, run https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110196683 — host PASS, Android SDK setup failure. Host log artifact ID `12565887634`. A follow-up fixes setup; see subsequent CI evidence below.

## Verified GitHub CI result

Commit `54d6f1c49bb0f74864bbcceb9cfaa0d2cf5ffc5b` passed both jobs in [run 40110369623](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110369623).

- Host job `120920630137`: original primitives, platform services and PACK inspector pass with ASan/UBSan and the runner's default LeakSanitizer enabled.
- Android job `120920630415`: checksum-verified SDK/NDK setup passes; original CRC/RNG, smoke executables and NativeActivity/GLES diagnostic library compile/link; diagnostic APK v2/v3 signature and ZIP alignment verification pass. Runtime remains untested.
- [Host compilation/test log artifact](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110369623/artifacts/12566092043), artifact ID `12566092043`.
- [Android compilation/package log artifact](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40110369623/artifacts/12566098284), artifact ID `12566098284`.
- Development branch: `dev/native-arm64-primitives`; [draft PR #1](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/pull/1). Initial implementation commit: `eea4dd3282dbd34526fa7ebe8e1dbdc6182a4815`. Follow-up toolchain fix and parser probe: `54d6f1c49bb0f74864bbcceb9cfaa0d2cf5ffc5b`.

The final evidence-only documentation commit does not alter tested code. No copyrighted engine/game binary was uploaded as an artifact.

## Continuation: supplied installation and native asset test

The community installation passes static/archive/config checks; see INSTALLATION_AUDIT.md. Original TBuf/StrBuf now compile on host and Android ARM64, alongside CRC/RNG. A native on-demand ZWP reader validates and reads 3,763 entries across 88 real packs (389,882,093 payload bytes), with every filename CRC checked against the original engine. Original TBuf tokenizes 1,165 archived text configs and 16 loose non-player configs without errors. This is lexical validation, not full PTree execution.

Seven local CTest targets pass under ASan/UBSan (local LeakSanitizer disabled for the previously recorded sandbox limitation), including tokenizer, archive corruption/truncation/lookup cases, and an original synthetic installation integration fixture. The Android original-engine diagnostic rebuild also compiles the tokenizer and native reader. A separate original-only asset APK compiles, passes signature/ZIP checks and has 16KiB ELF/ZIP alignment; it contains no upstream implementation or game assets. Its file-picker/JNI/device execution is untested and is the owner's next task.

Additional packaging work: a full JDK17 was required because the environment originally contained only a JRE. Raw Java compilation uses `--release 8` with the Android SDK on its classpath before D8 conversion; using the Android stub jar as javac's bootclasspath caused a LambdaMetafactory compile error and was corrected. SDK/NDK remain pinned as before. CI now uploads only the original-only asset test APK plus logs, never the engine-derived diagnostic.

Current execution logs: `docs/logs/install-slice/`. Updated CI at code commit `6526761636937cef238ed3c3dd71c9bef7454091` passes both jobs (see evidence below). M3, M4 and M5 remain unachieved. Next: run the provided asset test on Fold 5/Thor; continue the original parser/FileSys dependency slice and rendering integration with that runtime evidence.

### Updated CI evidence: installation I/O slice

[Run 40117369440](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40117369440) at code commit `6526761636937cef238ed3c3dd71c9bef7454091` completed with **host PASS and Android PASS**. Host job `120941316880` runs seven CTest targets with sanitizers. Android job `120941316945` compiles the original tokenizer and installation probe for ARM64, verifies both diagnostic packages, and publishes only the original-only asset-test APK.

- [Original-only ARM64 asset-test APK artifact](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40117369440/artifacts/12570351152), ID `12570351152`.
- [Host compilation/test logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40117369440/artifacts/12570060994), ID `12570060994`.
- [Android compilation/package logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40117369440/artifacts/12569591224), ID `12569591224`.

Real proprietary installation validation was performed locally; CI uses only original synthetic fixtures. Neither CI success nor ZIP/signature verification demonstrates Android runtime success. The owner's asset-probe report is still required. The locally delivered APK is separately debug signed from the CI build; both use the same tested source. Local delivered APK SHA-256: `40196a4bcd4cd9d02fc6f8a2e8e4d2358ba548dc172d82b88684ba309d7fb106`.

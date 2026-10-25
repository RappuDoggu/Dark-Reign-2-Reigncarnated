# Build status — latest update 2026-10-25

## Latest increment: frontend services

Original StdLoad menu functions now link and execute; native UTF-16/allocation helpers and a scoped original EventSys binding are implemented. All 29 host tests pass. The full frontend link still fails with 62 unresolved symbols (previous probe: 77); no main-menu APK or original interactive window is established. [Implementation, recovery notes and evidence](FRONTEND_SERVICES.md). Host and ARM64 CI both passed on `70c65f1`: [run 40208431181](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40208431181).

## Latest increment: full frontend control objects

Six full original UI translation units now compile for Linux x86_64 and Android ARM64/API26: IControl, ICRoot, ICButton, ICWindow (including title), IFaceVar and ICSystemButton. All 26 existing host tests pass. Executable linkage was attempted and failed on the remaining frontend services; no menu or new APK runtime is claimed. [Changes, reproduction and evidence](FRONTEND_CONTROLS.md). CI run [40203206699](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40203206699) passed both host and Android jobs for implementation commit c098b4758549bdbfea23b0d554509b904f14575d. [Recorded job/artifact evidence](logs/frontend-controls/ci-result.json).

## Latest increment: menu binary trace

Standalone Ghidra decompiled 13 Login/Shell lifecycle functions and callees from the owner-supplied executable. Six direct-call sequences verified; five corrupt evidence reports rejected. REA provider launch remains blocked by process/procfs inspection. No new C++ build, original-menu launch or APK runtime result is claimed in this increment. [Research, reproduction and next integration gates](REA_ASSESSMENT.md). Earlier sections below retain their dated build history.

## Milestones

| Milestone | Evidence | Status |
|---|---|---|
| M1 original subsystem with contemporary compiler | Original primitives, parser/run codes, Vid, buckets and bitmap pixel conversion compile/link with GCC 13.3; host tests pass | Achieved for these primitive subsystems; broader source/license audit continues |
| M2 original portable components on Android ARM64 | Original primitives, parser/run codes, Vid, buckets and bitmap conversion compile/link with NDK r27c Clang, API26, arm64-v8a | Achieved for these components; not the simulation |
| M3 APK initializes original engine and renders a scene | Original porting renderer runs on Fold 5; original Vid submission passes host GPU tests | **Not achieved**: no full engine initialization or original scene |
| M4 original single-player mission | Supplied installation validated; simulation port not built | Not achieved |
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

## Owner-verified Android runtime: Fold 5 asset probe v0.2

The owner supplied a successful runtime report from Samsung SM-F946U (Fold 5), Android API 36. This is owner-reported on-device evidence, not an automated test performed in the development environment.

- Result: PASS — native ARM64 archive access.
- Archive entries: 2,074.
- Payload bytes read: 99,631,656.
- Configuration files: 465; GOD meshes: 334; textures: 567.
- Reported elapsed time: 1,264 ms (single diagnostic run, not a performance benchmark).

Every count matches the inspected base.zwp baseline. This validates the file-picker/import/JNI/native archive-read path for this device and supplied archive. Full original engine initialization, original rendering, mission execution, audio/input integration and Thor dual-screen behavior remain unverified. M3–M5 remain unachieved.

The first asset-access runtime checkpoint is satisfied. No replacement installation or additional device report is currently required to continue the original PTree/FScope/FileSys integration and then the Vid/Bitmap backend.

## Current continuation: original configuration/run codes and initial GLES renderer

Original PTree/FScope/VNode/StdParse/BlockFile plus RunCodes/Main::Exec now compile through an external, hash-verified source overlay. Includes and real J01 types/object/strategic config trees parse successfully; text/binary round-trip, 32-bit numeric, stream precedence and original run-code lifecycle tests pass. See CONFIG_RENDER_PROGRESS.md for exact counts, failures, scoped source edits and unimplemented engine boundaries.

There are 11 host CTest targets with sanitizers plus separate offscreen GLES pixel validation (run twice with new contexts). Original configuration/runcode targets and the initial GLES adapter compile for Android ARM64. Renderer Test v0.3 packages original porting code only and loads the owner's cursor texture for GPU preview; signature, ZIP integrity and alignment are build checks, not device runtime evidence. The Fold 5 v0.2 archive PASS remains the verified device baseline. v0.3 GPU execution still needs testing. Full original engine/menu/gameplay remain unimplemented; M3–M5 are not achieved.

Touch/optional pen and dual-screen ideas are pinned in CONTROLS_BACKLOG.md for later. No additional installation files are currently requested. New CI results will be recorded after this code is committed.

Initial updated run 40121491459: host PASS and original ARM64 compilation/package step PASS, but the new software GLES CI step could not configure because the runner lacked libEGL. The workflow now explicitly installs libegl1, libegl-mesa0 and libgl1-mesa-dri before GPU-contract tests; a follow-up run is required. Local software EGL tests remain independently verified.

### Configuration/GLES CI result

[Run 40121657469](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40121657469) at `c7dd41bde5a7b9b466ce123621821017b9c562a0` completed with host PASS and Android PASS. This follows implementation commit `1e3e549692467a06c4e10edc1559c201bc42c803` and fixes only the missing software-EGL runner dependency.

- Host job `120954351242`: 11 CTest targets, sanitizers and default CI leak detection PASS.
- Android job `120954350938`: original config/run-code sources and initial GLES backend compile for ARM64; source-derived diagnostic package checks PASS; software EGL pixel tests PASS with two recreated contexts; original-only Renderer Test v0.3 build/signature/ZIP checks PASS.
- [Renderer Test v0.3 CI APK artifact](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40121657469/artifacts/12571696548), ID `12571696548`.
- [Host compilation/test logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40121657469/artifacts/12570828672), ID `12570828672`.
- [Android/GLES/package logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40121657469/artifacts/12571406870), ID `12571406870`.

The local direct-download v0.3 APK has SHA-256 `afb2fbc0b66cb1a2d2923de2a1307ddfd2e7da1ad8bf37acc5dacfb3895cedb6`, 2,433,659 bytes. Its separate retained development signing key matches the previous local delivery; CI generates its own development key. Runtime evidence for this new GLES path still requires the owner's Fold test. A successful GPU test will not by itself establish original engine/menu/mission initialization.


## Owner-verified Android GPU runtime: Fold 5 Renderer Test v0.3

The owner supplied a screenshot from Samsung SM-F946U showing the original cursor atlas rendered and all nine legacy-TL GPU pixel checks PASS. Reported GPU: Adreno (TM) 740; OpenGL ES 3.2 V@0676.77.1, driver date 01/27/26. Evidence is owner-supplied on-device output, not remotely automated execution.

Passing checks: ARGB byte order/half-pixel coverage; D3D top-left texture orientation; source-alpha blending; alpha-reference discard; zero-to-one depth mapping/occlusion; specular-alpha vertex fog; specular RGB addition; reciprocal homogeneous W; invalid-index rejection. Aggregate GLES3 legacy-TL pixel checks: PASS. The visible cursor atlas also confirms the selected archive-to-TGA-to-GPU preview path on this device.

Pause/resume and context-loss behavior are not established by this still screenshot. Original Vid/Bitmap integration, full engine initialization, main menu and mission execution remain outstanding; M3–M5 remain unachieved. Next engineering task: connect the tested GLES boundary to the original rendering/texture interfaces and continue required engine services. Touch/pen and dual-screen work remain deferred.


## Original Vid submission continuation

Original Vid texture/blend/depth/address setters, SetTextureDX and indexed submission now compile for Android ARM64 and execute through the native adapter into GLES on the host. Twelve host sanitizer CTest targets pass. GPU readback passes nine baseline plus 13 original-Vid checks in two new contexts. Full engine initialization is still incomplete. See [VID_STATE_PROGRESS.md](VID_STATE_PROGRESS.md) for exact extraction edits, rejection boundaries, compiler warning fixes and remaining work. New local logs: `docs/logs/vid-state/`. The source-derived test binaries stay private; no additional owner test is needed for this slice. Updated CI evidence follows when available.


### Verified CI: original Vid submission slice

[Run 40124738169](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40124738169) at implementation commit `695b18c9652a5f85aefe0be39b8b57edba0fe5b9` completed with **host PASS and Android PASS**. Host job `120963643069` passes 12 sanitizer CTest targets. Android job `120963643187` compiles the original Vid slice for ARM64, verifies diagnostic packaging, and runs 9 baseline plus 13 original-Vid GPU readback checks in two recreated software-EGL contexts. The logs explicitly confirm the original Vid modes, blending, depth and addressing tests. Public artifact packaging continues to exclude original engine implementations and proprietary assets.

- [Host compilation/test logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40124738169/artifacts/12572054376), artifact `12572054376`.
- [Android/GLES/package logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40124738169/artifacts/12572498966), artifact `12572498966`.
- The original-only v0.3 diagnostic is rebuilt by CI, artifact `12572309177`; it does not contain the original Vid implementation and is not evidence of full engine startup. The owner's already-installed v0.3 remains sufficient for the completed device checkpoint; no additional owner test is requested for this host-tested original slice.

Next: original Bitmap management/conversion and bucket flush/sorting, required texture stages, and remaining Main/GameGod initialization. No original menu/mission APK is claimed. Final evidence-only documentation commit does not alter the tested code.


## Original bitmap pixels and render buckets

Original PrimitiveDesc/Bucket/BucketMan/TranBucketMan now compile and render through the original Vid submission boundary. Original Pix and Bitmap::CopyLine conversion also compiles and supplies tested GLES texture pixels. Fourteen local sanitizer CTest targets pass; real software-GLES readback passes 31 checks per context in two fresh contexts. The new original targets and smoke executables compile for Android ARM64. Supplied base.zwp conversion matches 106 supported TGA images / 1,934,336 pixels; five unsupported native-decoder TGA formats are explicitly skipped. UBSan exposed a real original bucket split alignment bug; scoped external edits align the regions and bound index rebasing. See [BITMAP_BUCKET_PROGRESS.md](BITMAP_BUCKET_PROGRESS.md) and raw logs under `docs/logs/buckets/` / `docs/logs/pixels/`. Full Bitmap management, frontend/engine startup and M3–M5 remain unfinished. No new owner test is required. Updated CI evidence follows after the branch build.


### Verified CI: original bitmap pixels and opaque/translucent buckets

[Run 40140262166](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40140262166) at code commit `6f2b14ef76c7a1a3a904dbdb946755b7dceb78bc` completed with **host PASS and Android PASS**. Initial implementation is `0bcd6d000b355400965bc121af0013b7bef59d99`; the follow-up isolates conversion-only types in their own namespace and records optimized-build evidence. The earlier implementation also passed run 40139888918.

- Host job `121010326910`: 14 CTest targets pass with ASan/UBSan and default runner leak detection. Logs confirm original bucket alignment/merge/rebase/state/sort/450-bucket checks and all 18,504 bitmap golden comparisons.
- Android job `121010326747`: original bucket and pixel-conversion targets compile for ARM64; private diagnostic packaging verifies. Software EGL passes 31 GPU checks per context in two fresh contexts, including original pixel conversion -> buckets -> Vid -> GLES. The original-only diagnostic package also verifies.
- [Host logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40140262166/artifacts/12578691466), artifact `12578691466`.
- [Android/GLES/package logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40140262166/artifacts/12579100265), artifact `12579100265`.
- Original-only Renderer Test v0.3 rebuild artifact `12579145190` contains no original engine implementations/assets. It is not a new menu/gameplay build; no additional owner test is requested.

Locally, the two new component tests also pass in an optimized Release build with ASan/UBSan. Real supplied data matches 106 supported TGA images / 1,934,336 pixels with five unsupported native-decoder TGA formats explicitly skipped. See BITMAP_BUCKET_PROGRESS.md and release/raw logs for scope and failure details. M3–M5 remain unachieved. Next native launch dependencies are original Bitmap storage/load/registry/context restoration, FontSys/interface integration (current glyph drawing uses indexed rectangles), plus original frontend/startup orchestration. This final documentation commit changes no tested code.



## Original font and IFace rendering continuation

Original Font::Read/Width/Draw, FontSys atlas allocation/lookup and IFace glyph batching now compile through an external SHA-pinned overlay. Native RGBA bitmap storage implements lock/write/publication and atlas replay. Six supplied v1 fonts load: 1,351 glyphs, 12 atlases, 504 sample vertices; no font data is published. Sixteen host sanitizer targets and 39 GPU checks per recreated software-EGL context pass. Optimized font/storage checks also pass. Original font targets compile for ARM64; they are not yet executed on a physical device.

An actual missing-glyph-row failure exposed the old pre-raster Y inversion. The GLES backend now renders in logical game coordinates into an RGBA8/depth16 target and flips once at presentation. Original font coordinates/UVs are unchanged. The new original-only Renderer Test v0.4 contains 12 device checks (coverage, presentation and resize included) and needs Fold testing. Local APK SHA-256: `17e03f102b7887b6ba446ff2b6a6762e0879616a010890c53adc3cb243caf925`, 2,470,523 bytes; debug certificate matches local v0.3. See FONT_PROGRESS.md, ANDROID_RENDERER_TEST.md and docs/logs/fonts/. Full original menu/engine/mission startup remains unfinished; M3–M5 are not achieved. Updated CI evidence follows after this code is pushed.


### Verified CI: original font/IFace rendering and v0.4 coverage fix

[Run 40144610382](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40144610382) at implementation commit `381b6136cea58560387ea9344bad44978dc913cf` completed with **host PASS and Android PASS**.

- Host job `121023175267`: all 16 CTest targets pass with ASan/UBSan and default runner leak checks. Logs confirm original FFF v1/v2, glyph/metric/draw/shadow/clipping/capacity/texture flush/replay/recovery and native bitmap-storage checks.
- Android job `121023175052`: original font/IFace target and probes compile for ARM64; diagnostic package verifies. Software EGL passes all 39 checks per context in two fresh contexts, including retained CPU-font upload and original glyph drawing. The original-only v0.4 APK passes build/signature/ZIP checks.
- [Host logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40144610382/artifacts/12579983748), artifact `12579983748`.
- [Android/GLES/package logs](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40144610382/artifacts/12579998911), artifact `12579998911`.
- [Original-only Renderer Test v0.4 CI APK](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40144610382/artifacts/12579729247), artifact `12579729247`. CI uses a different development key from the direct local download.

The local direct-download v0.4 APK remains SHA-256 `17e03f102b7887b6ba446ff2b6a6762e0879616a010890c53adc3cb243caf925` (2,470,523 bytes), with the same debug certificate as the local v0.3 delivery. It contains no original engine code/assets. Its new 12-check framebuffer/presentation/resize path requires owner Fold runtime evidence. Original source font rendering has host runtime and ARM64 build evidence only. M3–M5 remain unachieved; no original menu/mission launch is claimed. The final evidence-only documentation commit changes no tested code.



## Owner v0.4 validation and native bitmap loading

Owner Fold5 report: SM-F946U, API36, Adreno740, context generation2, surface1692x666. All12 v0.4 public adapter GPU checks PASS;2074 archive entries/99,631,656 payload bytes and cursor decode PASS. Raw report: `logs/bitmap-loading/fold-v04-owner-report.txt`. This replaces the pending device gate for v0.4 only; it does not test original engine initialization.

New native TGA/PIC framing feeds original Pix/CopyLine; no upstream implementation was copied into this repository. Actual base.zwp:111 TGA plus453 PIC images load (564 total,11,605,888 pixels).106 true-color TGA files match the independent existing decoder; the remaining real formats have successful load evidence plus synthetic golden vectors, not a Windows oracle. Host ASan/UBSan:17 CTest targets PASS, including288 loading checks. See BITMAP_LOADING_PROGRESS.md for supported formats and remaining scope. No original menu or gameplay launch is claimed.

Local follow-through: all40 GPU checks PASS in each of two software EGL contexts. NDK r27c/API26 ARM64 builds the new loader, test executable and private diagnostic package successfully; ELF header reports AArch64. This establishes compilation, not ARM64 execution of the new loader. The private diagnostic package is not a distributable engine APK. Logs: `docs/logs/bitmap-loading/`. CI results for this change are recorded separately when available.

CI confirmation: code commit `5803b5e58cf3f55c19b26d173bd4c79f1de84645`, [run40153363872](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40153363872), host job121049381396 and Android job121049381468 both SUCCESS. CI confirms17 host tests (default leak checking enabled), ARM64 build/private package verification,40 software GPU checks per context across two contexts, and the original-only public v0.4 package build. Logs artifacts:12583248880(host),12583761620(Android/GPU/package); public renderer-test artifact12583307361. That artifact does not contain or test the new source-derived bitmap loader. Code-derived binaries/assets are not published.


## Original font registration integration

Original FontSys registration now runs behind native adapters: hash-pinned Create/GetFont/DeleteAll/OnModeChange and original NBinTree, with native per-file atlas ownership. All18 host ASan/UBSan tests pass; registration has20 checks. Actual interface_standard_fonts.cfg produces nine aliases/six shared files through original PTree/FScope. All41 software GPU checks pass per context across two contexts, including configured alias/reload drawing. See FONT_REGISTRY_PROGRESS.md. This is not complete IFace startup or a menu/engine launch.

Local NDK r27c/API26 build/package verification PASS, including dr2_font_registry and AArch64 font_registry_smoke/font_registry_probe. ELF verification and package logs are retained in docs/logs/font-registry. These source-derived registry binaries were compiled, not executed on Android; the unchanged public v0.4 adapter remains the latest owner-tested APK.

CI verified: code commit `53027af214f5103c6eddf622ea9c9c782f8b0c7c`, [run40154496926](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40154496926) SUCCESS. Host job121052859375:18 tests PASS with default leak checking. Android job121052859564: ARM64 build/private package checks,41 software GPU checks in each of two contexts, public original-only v0.4 package build PASS. Evidence artifacts:12583708068(host logs),12583603418(Android/GPU/package logs),12583388571(public v0.4 APK). That APK contains no original font registration/engine implementation and is not a new menu build.


## Original interface metrics integration

Original IFace InitMetrics/ConfigureMetrics/GetMetric/UpdateScreenMetrics now run behind scoped native viewport/state adapters. Same-parse font/metric configuration resolves actual standard-font TitleFont=WindowTitle and ThinTitleFont=System. Host ASan/UBSan:19 CTest targets PASS, including33 metric checks. Software GLES:42 checks PASS in each of two recreated contexts, including configured font-reference/shadow rendering. See INTERFACE_METRICS_PROGRESS.md and docs/logs/interface-metrics. This does not launch the original menu or engine.

Local Android verification PASS: NDK r27c/API26 builds the original metric source slice, native adapter, AArch64 interface_metrics_smoke and private diagnostic package. ELF header/package verification logs are retained. The new metric/registry path has not executed on Android and is absent from the unchanged public v0.4 APK.

CI verified for code `41898b8af98cff073a1f53a77c677188cc90822d`: [run40157490540](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40157490540) SUCCESS. Host job121061885763 passes19 CTest targets including33 original metric checks with default leak checking. Android job121061885568 passes ARM64/private package verification,42 software GPU checks per context across two contexts, and the unchanged original-only public v0.4 package build. Artifacts:12584667237(host logs),12584757331(Android/GPU/package logs),12584812211(public v0.4 APK). Public APK excludes the original metric/registry implementation; no original menu or engine launch is established.


## Original bitmap manager integration

Original Bitmap::Manager Setup/Create/Find/FindCreate and ReleaseDD/OnModeChange loops now execute against native TGA/PIC image and optional real GPU ownership callbacks. Host ASan/UBSan:20 tests PASS, including29 manager checks. Real base.zwp:564 requests,563 entries/initial reads (smoke.tga reuses smoke.pic),563 successful restore reads. Software GLES:46 checks pass per context in two contexts; the manager persists across context loss and recreates textures in the second context. Two fixture failures (missing aliases, then missing blend bits) were corrected without weakening original Vid validation. See BITMAP_MANAGER_PROGRESS.md and docs/logs/bitmap-manager. No engine/menu launch is established.

Local Android NDK r27c/API26 compilation/package verification PASS, including original/native bitmap manager libraries, AArch64 bitmap_manager_smoke and bitmap_manager_probe. ELF/package logs are retained. The new manager has not executed on Android and is excluded from the unchanged public v0.4 APK.

CI verified for code `e6a10e65e8cef36983263a09567f22be10d4991a`: [run40159070494](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40159070494) SUCCESS. Host job121066594264 passes20 tests, including29 manager checks with default leak checking. Android job121066594462 passes ARM64/private package verification,46 software GPU checks in each of two contexts, and the unchanged original-only public v0.4 package build. Artifacts:12585855099(host logs),12585349878(Android/GPU/package logs),12585554581(public v0.4 APK). Public APK excludes the original manager implementation; this is not an engine/menu runtime result.


## BMP corpus and control dependency checkpoint

Native BMP framing now feeds original CopyLine and the original manager fallback path. All63 real24-bit BMPs across88 archives match Pillow12.3.0 exactly (1,118,208 pixels); none skipped. Base image loading now covers567 requests/566 original-manager entries, all566 reloaded. Host ASan/UBSan:21 tests PASS, including213 BMP checks. Software GLES:51 checks PASS in each of two contexts. Local NDK r27c/API26 build/private package verification PASS. No new original-derived Android runtime test, original menu or mission is established. See BMP_PROGRESS.md and docs/logs/bmp-loading.

CI verified for code `707eea2f49b8b98f3446d58259ef8f603e095283`: [run40161670719](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40161670719) SUCCESS. Host job121074355361 passes21 tests including213 BMP checks with default leak checking. Android job121074355513 passes ARM64/private package verification,51 software GPU checks in each of two contexts, and the unchanged original-only public v0.4 package build. Artifacts:12586458917(host logs),12586668717(Android/GPU/package logs),12586648836(public v0.4 APK). The63-image Pillow comparison uses the owner's installation locally, not assets in CI. No original menu/engine runtime or new original-derived Android execution is established.


## Runtime recovery and requested menu gate — 2026-10-25

Requested checkpoint: original menu APK at minimum. NOT ACHIEVED. Recovered original tracking, event dispatch and typed-variable runtime modules plus a synthetic kernel-handoff test. Local observations before workspace disconnection:25 ASan/UBSan tests passed; new services compiled for AArch64/API26; existing51 GPU checks passed per context. Original IControl syntax probes failed at vendor/graphics/math integration; no control link, engine/menu initialization or gameplay occurred. The workspace disconnected before raw logs/upload. Source was reconstructed through GitHub; fresh CI must verify the committed bytes. See RUNTIME_PROGRESS.md. No new device APK is delivered as a menu build.


## Restored workspace, decompiler verification and control types — 2026-10-25

Workspace restored: isolated REA6.8.0/Ghidra12.1.4/full JDK21.0.13+9 installed. Official archive hashes pass; standalone Ghidra decompiled the owner executable startup at 0x004c3930 and verified the seven ordered WinMain registrations. REA doctor/decompile/providers fail at process ownership because namespace PIDs disagree with /proc; no REA integration claim. Original control declarations/five coordinate-border methods now compile; 26 host tests pass with LeakSanitizer disabled (default leak checks fail in Work), 15 new UI-type checks, ARM64 compilation PASS. Full IControl/IFace/Main/GameGod/menu are unlinked; no new menu APK. See REA_SETUP.md and CONTROL_PROGRESS.md.


CI verified for implementation `8e73b55c49823e2d3758e8712d731685d3f8687d`: [run40198264863](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40198264863) SUCCESS. Host job121183725463 passes all26 CTest targets including15 original control-type checks with runner default leak checking. Android job121183725807 builds the original control declaration/coordinate library and test for ARM64, verifies the private diagnostic package and passes the software-GLES target across two contexts. CI records vendor, opaque-video and native-control-declaration frontend compile failures as diagnostics; it does not claim a successful full frontend link. [Host logs artifact12600499572](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40198264863/artifacts/12600499572), [Android/GPU/package logs artifact12600759005](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40198264863/artifacts/12600759005). The public v0.4 renderer artifact remains unchanged in scope and is not a menu test. REA/Ghidra verification was performed locally in Work, not by this CI run. This evidence-only update changes no compiled code.


# Campaign/Login boundary — 2026-10-26 UTC

Original campaign/progress/difficulty/catalog, campaign selector, preview metadata,
drop lists and debug viewers now compile. **52/52 host tests pass**, including
31 catalog/preview and 60 live-control checks. The real installation catalog finds
60 missions, 9 groups, 2 campaigns and 4 difficulty settings. Startup reaches
statement 120 and **Game::Login**, with 8 bitmaps loaded. No main-menu APK yet.
[Implementation, test scope and remaining boundaries](CAMPAIGN_UI_PORT.md).
The previous settings increment passed [CI run 40312030783](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40312030783);
this increment's CI is pending.

# Settings-control checkpoint — 2026-10-26 UTC

Original display/graphics settings, save/load, color picker and sound-settings
controls now compile. **51/51 local host tests pass; 56 live control/configuration
checks pass.** Real installation startup reaches statement 112, blocked by
**Game::CampaignSelection**. Audio is explicitly disabled; save-game services
remain a required binding. No original main-menu APK is ready.
[Implementation, limits and actual failures](SETTINGS_CONTROL_PORT.md).
The preceding UI-presenter CI passed host, ARM64 compilation and software-GLES
pixel checks in [run 40310661006](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40310661006).
CI for this settings increment is pending.

# Ordered UI presentation — 2026-10-26 UTC

Fixed background/font/foreground submission order and added a native GLES UI
presenter with font/bitmap uploads and frame recovery. Fresh-directory host tests:
**51/51 PASS** after repairing damaged local build artifacts. Software-GLES pixel
checks are added for CI and are not yet verified for this increment.
[Details and limitations](UI_PRESENTATION.md), [host tests](logs/ui-presenter-host-tests.log).
Original Login remains blocked at Vid::Options; no main-menu APK yet.

# Startup console increment — 2026-10-26 UTC

Original UI sound configuration, Edit/ConsoleEdit/ConsoleViewer, TabGroup and
TipWindow compile. **51/51 host tests pass**. Real Login config reaches statement
107 and stops at **Vid::Options**. Audio playback and a main-menu APK remain
unverified. [Implementation and limitations](STARTUP_CONSOLE_CONTROLS.md),
[probe](logs/startup-console-probe.log), [tests](logs/startup-console-tests.log).

# Original list/menu widgets — 2026-10-26 UTC

Original Listbox, ListSlider, Slider and Menu now compile and link. Live profile
selection/menu-item checks pass; **51/51 host tests pass**. Real Login configuration
reaches statement 104, stopping at UI Sound configuration. No main-menu APK yet.
[Scope and remaining dependencies](LIST_MENU_CONTROLS.md),
[actual probe](logs/widget-menu-probe.log), [tests](logs/widget-host-tests.log).

# Cursor configuration increment — 2026-10-26 UTC

Original cursor and console-color code now builds; **51/51 host tests pass**.
Real Login configuration reaches statement 96 with 46 cursors, 9 font aliases,
4 bitmaps and one default-cursor draw batch. Next blocker: original Listbox class.
No main-menu APK yet. [Implementation and evidence](CURSOR_PORT.md),
[probe output](logs/cursor-menu-probe.log), [tests](logs/cursor-host-tests.log).

# Menu configuration increment — 2026-10-26 UTC

Original Main/IFace configuration dispatch and key-name configuration now build.
Host CTest: **50/50 PASS** (local ASan/UBSan, LeakSanitizer disabled).
The real Login configuration loads **128 key names and 12 modifiers**, then stops
at unported **CreateCursor**. No original main-menu APK is demonstrated.
[Implementation, reproduction and next dependencies](MENU_CONFIGURATION.md).
[Host evidence](logs/configuration-host-tests.log),
[actual asset-backed probe output](logs/menu-configuration-probe.log).

# Live original UI increment — 2026-10-26 UTC

The ten-object staged UI now links with zero unresolved symbols and no section
GC. Live original root/button/static/modal message-box tests pass: 20 checks,
21 geometry batches, actual notification callback and modal/death cleanup.
Host suite: **47/47 PASS**, local ASan/UBSan with LeakSanitizer disabled.
Independent host and ARM64/GLES/package CI passed for d468c50f in
[run 40267250825](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40267250825).
[Actual provider output and artifacts](logs/live-controls-ci.json). Earlier console
commit 1d5c34d2 passed both jobs in run 40266263136 (host 121389256848,
Android 121389257046). No real menu configuration or original-menu APK yet.

[Scope, fixes, failure evidence and remaining menu work](LIVE_FRONTEND_CONTROLS.md).
[Host tests](logs/live-controls-host-tests.log),
[complete link result](logs/live-controls-full-link.json).

# Console dispatch increment — 2026-10-26 UTC

The original console parser/VarSys command dispatch now builds on host. Focused
checks pass (21); the full host suite passes 45/45 with local LeakSanitizer off.
A missing execute bit again prevented one initial test process from starting;
restoring that bit allowed the complete suite to pass. CI remains the independent
clean build check. The full staged-control link has three unresolved services:
IFace::MsgBox, IFace::Sound::Play and movie deactivation. No menu runtime yet.
[Implementation and limits](CONSOLE_DISPATCH.md), [host test log](logs/console-host-tests.log).

The preceding localization/Babel commit b4f2b1e passed both host (44/44 tests)
and ARM64/GLES/package CI: run 40265745495, host job 121387664105,
Android job 121387664003. Console ARM64 verification is pending.

# Localization lookup and Babel increment — 2026-10-26 UTC

Original MultiLanguage lookup and Babel substitution now compile on the restored
host workspace. Native checked formatting preserves UTF-16 menu strings and
variadic integer/float/string arguments. Host ASan/UBSan tests: **44/44 PASS**
with local LeakSanitizer disabled. Focused tests: 20 lookup and 39 Babel checks.
The first run could not execute two files whose execute bits were absent;
restoring those bits allowed the full suite to pass. Both logs are retained.
ARM64 CI for this increment is pending; no new engine/menu APK is claimed.

The eight-object full control link now has four unresolved services:
Console::ProcessCmd, IFace::MsgBox, IFace::Sound::Play and movie deactivation.
This is a staged-control link boundary, not a count of all menu startup work.
See [implementation, compatibility limits and reproduction](LOCALIZATION_LOOKUP.md),
[host tests](logs/localization-babel-host-tests.log), and
[link evidence](logs/localization-babel-full-link.json).

# Latest inspected menu initialization evidence

The supplied community-fixed installation was recovered without replacing its
files; dr2.exe still hashes to the previously analyzed build. The original
runcode_main_shell.cfg issues commands that launch the shell mission, so a menu
is not established merely by drawing themed controls. Asset-backed native tests
now parse Login then Shell-menu configuration and execute only the theme-related
statements: 21 color groups, 14 skins and 2 images; 183 other statements are not
executed. See REAL_MENU_THEME.md and its metadata-only logs.

Original-source staging now also covers menu expressions, message-box events,
nine-piece skin rendering, color/theme configuration and texture configuration.
ARM64/GLES evidence for the theme increment is CI run 40224830286. Native pointer
input and the real-theme probe are the following increment. The complete staged
control link remains incomplete; no engine/menu/gameplay success is claimed.

Historical audit follows.

# Source port audit — implementation evidence

Date: 2026-10-24. This expands the initial audit; it does not establish redistribution rights or claim a complete engine build.

## Source identity and scope

- Primary checkout: `grasmanek94/darkreign2` at `9ce9da5f21604310a997f0c41e9cd383f5e292c3`.
- Additional reference: `medwards/darkreign2` at `fad7b9fb3f50339c81c6d11a09144fca99d03fb7`.
- Both retrieved with git into separate temporary checkouts. Primary has 2,136 tracked files and 20 vcxproj files. `dr2.sln` has 19 solution projects; the additional project is the Miles proxy.
- `medwards` has root and `source/` trees. Its README describes a Google Code fork and a VC6 build. That is provenance information, not a verified license grant.
- Root `system/crc.cpp` and `system/random.cpp` in the two references are identical after newline normalization. Their byte hashes differ due to line endings. The CMake harness validates primary-source byte hashes before compiling.
- `upstream-inventory.json` records project dependencies, source counts, symbols and line locations. Reproduce with `python3 tools/audit_upstream.py /tmp/dr2 --output inventory.json`. The scan includes comments and inactive code; counts are candidates, not active-call counts.

## Executable startup: inspected control flow

1. `appdr2/winmain.cpp::WinMain`: reset floating-point state; call `Main::Init(hInst, cmdLine)`.
2. `main/maininit.cpp::Main::Init`: retain instance/command line, set active state, call `LowLevelSystemInit`, register QUIT run code. Low-level init sets debug instance, version, mono/debug services and FixMe. Some clock/log/debug calls are commented out in this fork; do not infer they execute.
3. WinMain registers KeyCheck, Intro, Login, Shell, Mission and Outro; Studio is registered in non-DEMO builds. It enters `Debug::Exception::Handler(GameGod::Start)`.
4. `game/gamegod.cpp::GameGod::Start`: reset/mask FPU exceptions; install video mode callback; call `MultiPlayer::PreInit`, then `Main::CoreSystemInit`.
5. `main/maininit.cpp::CoreSystemInit`: register emergency shutdown; clear video state; parse command line; enable watchdog; obtain/create HWND; seed non-synchronized RNG from Clock; initialize TrackSys, VarSys, commands, FileSys, EventSys, MultiLanguage, FontSys, Console; run `Setup::StartupConfiguration`; initialize Vid; show window; initialize Input after activation; start frame timing.
6. `GameGod::Init`: CD discovery and command registration; initialize Sound/Miles, GameSound, Orders; execute game config; initialize IFace, GameTime, game run codes, Sync, Difficulty, Campaigns, Missions, User, Demo, SaveGame, Sides, Mods and MultiPlayer; register UI controls; select Intro (CRC `0xA521E37B`, verified by our test).
7. `Main::MessagePump` in maininit.cpp drains PeekMessage/dispatches messages and processes run codes while active. GameGod then shuts down game and core systems; WinMain calls Main::Done, which also has optional external patcher/process launching.

The portable diagnostic does **not** execute this full sequence. It isolates original primitives first; redirecting WinMain straight into Android would drag in all the above services.

## Build graph and original configuration

`appdr2/appdr2.vcxproj` links coregame, coregame_ai, coregame_interface, coregame_objects, coregame_orders, coregame_particles, coregame_tasks, game, graphics, interface, main, multiplayer, sound, styxnet, system, util and wonlib. The application also directly compiles `graphics/meshmrm.cpp`. `appmesh` is a separate tool/application target.

The inspected application project uses Win32 Debug/Release, v140, Windows SDK 8.1, forced `std.h`, fastcall, pre/post-build tools, and Windows resources. `3rdparty/dxlib.h` hardcodes x86 Windows SDK **10.0.14393.0** library paths, so even the nominal SDK 8.1 configuration is not self-contained. No Windows compiler/SDK/MSBuild is installed in this Linux environment. A complete Windows build was not attempted or verified. Original project files remain unchanged in the separate upstream checkout.

## Platform and architecture findings

| Boundary | Inspected evidence | Port implication |
|---|---|---|
| Global headers | `system/std.h` forces win32.h, mmsystem.h, dxinc.h, conio.h | Replace forced platform-heavy prelude incrementally; no giant fake windows.h |
| Integer ABI | `system/defines.h` defines U32/S32 as unsigned/signed long | LLP64/ILP32 assumptions break on ARM64 LP64; fixed-width types required |
| Unicode | CH is unsigned short | Preserve 16-bit units; do not substitute Android wchar_t |
| x86 | system/utils.cpp, clock.cpp, debug*.cpp, util/mathtypes.cpp and graphics bitmap primitives contain assembly/intrinsics | Replace operations individually; preserve rounding and deterministic simulation semantics |
| FPU | GameGod startup and Utils::FP use x87 control state | ARM floating-point/rounding parity needs explicit fixtures before simulation claims |
| Pointers | `system/dlgtemplate.cpp:77`, `debug_memory.cpp:2343-2346` narrow pointers to U32 | Pointer storage must use uintptr_t; serialized fields remain 32 bits |
| Layout | PACK header uses packed U32 fields; binary block/terrain/mesh I/O | Validate serialized records individually; never globally widen fields |
| Files | system/file.cpp and filemap.cpp call CreateFile/CreateFileMapping | Introduce native handles, bounded mapping, file offsets, search and lifetime semantics |
| Input | interface/input.cpp:689 calls DirectInput8Create | Preserve Input event contract while replacing acquisition/key/mouse backend |
| Network | styxnet/win32_socket.cpp uses WSA* and ws2_32; win32_event/critsec/thread wrappers | Port socket/event/thread abstractions; single-player still initializes multiplayer |
| Platform coupling | simulation uses system/util/templates and shared forced header | Gameplay is a preservation target, not currently an independently portable library |

Lexical inventory, excluding 3rdparty and won: assembly/compiler constructs in 14 files (80 matching lines), selected Windows services in 17 files (38 lines), selected DirectX symbols in 16 files (42 lines), Miles/Bink in 14 files (131 lines), socket markers in 5 files (577 lines). These are deliberately narrow patterns, not an exhaustive Win32 inventory.

## Rendering investigation

`3rdparty/dxinc.h` includes DirectX 7 headers. `graphics/vid_enumdx.cpp` uses DirectDrawCreateEx and IDirectDraw7; `graphics/vid.cpp` creates the D3D device on the back surface. `graphics/vidrend.cpp` submits DrawPrimitive/DrawIndexedPrimitive with fixed-function-era vertices and render state. Bitmap storage, texture surfaces, locks, blits, display modes and Bink playback cross the renderer boundary. Porting draw calls alone will not preserve behavior.

Provisional lowest-risk approach: retain camera, visibility, terrain, mesh, bucket and software transform code; implement a bounded GLES backend behind the existing Vid/Bitmap contracts and validate with captured Windows output. GLES shaders must reproduce original texture stages, alpha/fog/depth, coordinate conventions and blend modes. GLES3 diagnostic setup is implemented, **not an engine rendering backend**. Vulkan remains an alternative if profiling/correctness requires it. A D3D translation layer is not selected: it would also need Android ARM64 D3D7/DirectDraw support, compatible COM/surface services and licensing validation. No such layer has been integrated or tested. SDL3 is deferred until its lifecycle/input/audio coverage is weighed against the preserved engine interfaces; this harness uses NDK NativeActivity directly.

## Asset contract

- FileSys defaults to `.zwp` archives (`system/filesys.cpp:28-29`). Pack sources index filenames using case-insensitive CRC rather than full paths.
- `system/packdef.h`: NORK signature, version 1, a 56-byte header with 32-bit little-endian fields; variable-length file and directory tables. Header layout is compiled from the original header with a static assertion.
- **Implementation outranks the header's illustrative offsets:** `filesrcpack.cpp` maps `info->offset` directly as an absolute archive offset. `apps/zwagapack/packtool.cpp` writes file and directory tables. Our original read-only inspector follows these implementations and is tested with synthetic files, not proprietary assets.
- `PTree`/`FScope` implement structured configuration parsing; startup configuration establishes streams/search paths. Case-folding and search precedence must survive Android's case-sensitive filesystem.
- `graphics/meshload.cpp` uses `.god` compiled meshes; `.xsi` references appear in meshconfig; bitmap lookup uses `.pic`, `.tga`, `.bmp`, and Bink integration. Terrain data serializes through BlockFile (`coregame/terraindata.cpp`). Audio supports sampled files and MP3 via Miles calls.
- No user game archive was supplied for this execution. Real asset import, complete pack compatibility, startup config and missions remain unverified. No copyrighted asset is included in this repository or diagnostic package.

## Proven and fixed issues

Bare CRC compilation fails because U32/U8/CH arrive through the legacy forced header; using that header fails at windows.h. The harness instead force-includes original fixed-width type declarations and a C++ rotate adapter. CRC and RNG source files remain byte-for-byte unchanged.

A new high-bit byte test reproduced `Crc::CalcStr` indexing `toLower` with negative signed char (`system/crc.cpp:176`), caught by ASan/UBSan. The fix applies `-funsigned-char` **only** to crc.cpp. Regression tests cover bytes 1..255, ASCII folding, existing Intro CRC, bitwise CRC oracle, all streaming split points, UTF-16 ordering and RNG golden states. This is a verified build-adapter correction, not a claim that all Unicode paths are safe.

## Remaining audit limits

Licensing is unresolved; see DEPENDENCIES. No exhaustive runtime call graph, complete serialized-type review, entire simulation compile, complete Windows build, real asset compatibility run, or Android execution has been completed. The native port is at primitives/platform bring-up, not gameplay.

## Continuation with supplied community installation

See INSTALLATION_AUDIT.md for actual file/PE/archive/config observations. The former statement that no game archive was supplied describes the first execution only. Source TBuf/StrBuf now compile on GCC and NDK ARM64; the original tokenizer processes all 1,165 archived and 16 non-player loose text configurations with no errors. Native ZWP access and the original CRC agree across all 3,763 archive entries. The full original PTree/FScope/FileSys chain and game initialization still do not compile or run as a whole. A separate original-only asset diagnostic is buildable without an upstream checkout, so user testing can proceed without distributing the original engine implementations.

## Configuration/runtime and renderer continuation

See CONFIG_RENDER_PROGRESS.md for the original PTree/FScope/StdParse/BlockFile and RunCodes/Main::Exec port slice. The previous io.h compile blocker is bypassed at a narrow native File/DataFile boundary; it is not a complete original FileSys port. Original J01 type, object-placement and strategic trees now parse with actual includes and values, without executing mission logic. Initial GLES TL rendering passes independent pixel fixtures, but original Vid/Bitmap and full engine startup remain unported.

Additional provenance evidence: the archived Google Code project metadata at https://storage.googleapis.com/google-code-archive/v2/code.google.com/darkreign2/project.json declares `license: mit`. Its description attributes source acquisition to a former developer. This is stronger provenance than a press label but does not establish that the uploader had authority to license the Pandemic/Activision code. The inspected mirrors still lack a clear root grant. No original implementation is copied into this repository; distribution practice is unchanged pending that authority question.


## Inspected original Vid boundary continuation

The isolated original stage-zero state and indexed-submit functions in graphics/vidrend.cpp now compile through an external SHA-pinned overlay, with original Vertex.h flag values. Original SetTextureDX terminates the next stage explicitly; stage-zero RS_TEX_ADD is modulation in the inspected implementation, while stage-one behavior differs. Tests follow that implementation rather than assuming additive stage-zero color. The narrow adapter does not initialize original Bitmap/BucketMan. The original debug range-log block in DrawIndexedPrimitive is replaced by unconditional native input validation; the DODXLEANANDGRUMPY path is retained. See VID_STATE_PROGRESS.md. Clang exposed an original BlockFile::ReadBlockAlloc new[]/delete mismatch; reviewed overlay ownership now uses unique_ptr<char[]>.


## Inspected bitmap/bucket portability findings

The original ratio-based BucketMan::GetMem can place U16 indices at an odd offset; UBSan reproduced this with a 1,000-byte pool slice. The external overlay aligns region splits and reservations to four bytes and bounds index rebasing. Original TranBucketMan::Flush uses a 444-pointer scratch array; the native overlay sizes it to the actual descriptor count while retaining selection order, tested with 450 active buckets. Original Bitmap::CopyLine treats 16-bit source as ARGB1555 and expands its alpha bit to 128; indexed palette alpha is ignored. Those inspected behaviors are preserved. Three 24-bit type-punned stores become memcpy. The full original Bitmap manager/readers and InitBuckets orchestration remain unported; original FlushTex resets unrelated queued state and needs call-site analysis before native texture eviction is attached. See BITMAP_BUCKET_PROGRESS.md.


## Inspected Font and IFace continuation

The complete current interface/font.cpp::Draw emits indexed rectangles through IFace::GetVerts/SetIndex, despite the unused DOTRIFAN define. This corrects the earlier inference that font support requires triangle fans. Source-derived v1/v2 FFF extents and two-byte glyph images are validated before the original pointer reader; unaligned loads and zero-length-array dependence are removed without changing disk sizes. Original glyph packing and original IFace capacity/texture flushes are retained. Original system/utils.cpp::FtoL explicitly selects x87 truncation and restores the control word; opacity 255*0.5 therefore becomes 127, not 128. The adapter follows that implementation.

Real supplied base.zwp fonts all load (6 fonts, 1,351 glyphs, 12 atlases). The first actual font GPU test showed a missing top row with the old renderer. A logical offscreen target and final presentation flip fixes the retained fixture without editing original glyph positions. This is host GPU evidence, not a D3D7 Windows capture or new Android engine-boot evidence. Details and raw failures: FONT_PROGRESS.md and docs/logs/fonts/.



### Bitmap loader investigation and owner v0.4 evidence

Inspected primary `graphics/bitmap.cpp` ReadTGA/ReadPIC, PIC headers/channel enums and `graphics/bitmap_manager.cpp` Create/Find/FindCreate/DisposeAll/ReleaseDD dependencies. TGA indexed8/packed16 and PIC RGB plus separate alpha occur in the supplied base archive. Native bounded framing now loads564 TGA/PIC images through original CopyLine; full original manager/surface/mipmap methods remain unported. True-color TGA RLE and mirrored origins are explicit supported extensions beyond the original reader. PIC unknown encodings fail explicitly. See BITMAP_LOADING_PROGRESS.md.

Owner v0.4 Fold report passes all12 public adapter pixel checks including integer glyph coverage, presentation orientation and target resize, on context generation2. It contains no original game implementation and establishes no engine/menu/mission startup.


## Original font registration integration

Original FontSys Init/Done/Create/GetFont/DeleteAll/OnModeChange and NBinTree now execute in a scoped native registration slice. Inspected utiltypes.h confirms MAX_GAMEIDENT64. Actual interface_standard_fonts.cfg declares nine aliases referring to six files; parser and registration results match. IFace Metrics/control/frame/startup handlers remain unported. See FONT_REGISTRY_PROGRESS.md for ownership/ABI boundaries.


## Original interface metrics integration

Inspected iface.cpp InitMetrics/ConfigureMetrics/GetMetric/UpdateScreenMetrics and iface_types.h MetricType. Extracted original code preserves the reserved sentinel slot and original default/key tables. Actual standard-font config executes its Metrics section and resolves its two original font aliases. Native viewport/state adapters do not initialize IFace controls or graphics. See INTERFACE_METRICS_PROGRESS.md for exact scope/evidence.


## Original bitmap manager integration

Original graphics/bitmap_manager.cpp Setup/Create/Find/FindCreate/ReleaseDD/OnModeChange, Bitmap reload/resource contracts, Utils::FindExt, Dir::PathExpand and BuffString limits were inspected and hash-pinned. The native slice preserves extension-independent cache lookup, fallback priority, numbered frame rings and release/restore iteration. Actual archive smoke.tga/smoke.pic requests demonstrate a cache reuse; directory-qualified keys demonstrate the original Setup/Find mismatch. Neither is treated as asset corruption. Full Bitmap/movie/mipmap integration is still missing.


## BMP corpus and control dependency checkpoint

Inspected original ReadBMP: DIB40, DWORD row stride, signed-height orientation, palette handling, CopyLine, and unfinished BI_BITFIELDS mask reading. Inventory finds63 BMPs in88 archives, all24-bit BI_RGB, no loose BMPs. All63 independently match Pillow output. Game::Preview uses terrain.bmp; this does not execute a mission. IFace::Init and IControl construction/destruction establish EventSys/Input/Main/TrackSys/VarSys/colors/control dependencies. util/event.h packs native pointers inside Event; ARM64 alignment/runtime-layout and serialization consumers need further audit. No event/control initialization is claimed.


## Runtime recovery and requested menu gate — 2026-10-25

Original event/queue, tracking/reapers, String and VarSys family inspected/staged. Native pointer layout is runtime-only (Event40 bytes/interface offset8, tracking Info16/offset8). Original dispatch/work-queue flow retained; native queue corrects the inspected Full/Empty/Flush defects. Binary array cleanup and string/path bounds fixed. Full IControl compilation remains failed at vendor/graphics/math integration; actual Login/Shell initialization is unachieved. RUNTIME_PROGRESS.md distinguishes local observations from reconstructed-source CI verification after workspace loss.


## Restored workspace, decompiler verification and control types — 2026-10-25

The supplied dr2.exe SHA256 5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250 was independently analyzed with Ghidra12.1.4. Startup at 0x004c3930 registers KeyCheck/Intro/Login/Shell/Mission/Outro/Studio in source order and returns zero: strong WinMain identity evidence, not proof of whole-build source equivalence. Standalone decompilation works; REA provider startup fails at the Work PID/procfs mismatch. Native full IControl declarations compile after isolating original Area/Color/PaintInfo types from Windows/rendering includes. Full translation-unit errors and unchanged engine/menu limitations are in CONTROL_PROGRESS.md.


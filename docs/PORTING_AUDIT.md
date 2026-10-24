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

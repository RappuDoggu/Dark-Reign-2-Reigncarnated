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

# Current menu dependency checkpoint — 2026-10-25

Original UI drawing, expressions, dialog events, theme configuration and static
texture loading now have native bridges and runtime tests. Pointer events use
the native EventLoop through a scoped input producer. UI text conversion offers
explicit ASCII/Windows-1252 handling; whole-engine localization/encoding remains
unfinished. These changes do not initialize a live menu.

The full staged-control link has five missing service symbols: Babel translation,
console command processing, message-box creation, interface sound playback and
movie deactivation. This is a subset link, not a complete game dependency count.
Additional widget classes, game command handlers, Login/Shell lifecycle and
scene rendering still require integration. Optional installation recovery uses
system libarchive. See FRONTEND_INPUT.md and REAL_MENU_THEME.md for evidence.

Earlier dated entries below record the progression and may describe superseded
boundaries.

# Dependencies and provenance

Inspected 2026-10-24 at the revisions in PORTING_AUDIT. Public availability and press descriptions of "open source" are not treated as a license grant.

| Dependency | Evidence in implementation/tree | Status and migration |
|---|---|---|
| Original Pandemic game | Copyright notices across settings.h and engine sources; no root redistribution license found in either inspected tree | Unresolved. Keep source in temporary checkout; publish original harness/metadata only. No engine binaries/APKs uploaded. |
| Win32/MSVC | winmain, system/win32.h, std.h, fastcall, SEH, resources, compiler pragmas | Replace platform boundaries; preserve original VS files externally. Windows baseline unverified. |
| DirectX 7/DirectInput 8 | dxinc.h, Vid device/surface code, Input::Init | Bundled SDK material is not evidence of rights to redistribute. Remove Android reliance, retain Windows reference. |
| Miles Sound System 6.0f | 3rdparty/mss/mss.h version/date 24-Apr-00; sound/sound.cpp calls AIL_startup; digital/stream/redbook use AIL APIs | Proprietary middleware; Android rights/implementation absent. Preserve Sound public contract, replace mixer/stream backend after behavior audit. This is the primary audio path; do not assume a DirectSound swap is sufficient. |
| Bink 1.0i | 3rdparty/bink/bink.h version/date 2000-04-05; graphics/bitmap.cpp calls BinkOpen/decoding functions | Proprietary middleware; no Android library or rights established. Isolate movie/animated-texture path, evaluate licensed or compatible decoder separately. |
| zlib 1.1.3 | 3rdparty/zlib/zlib.h includes permissive license and version; wrapper links bundled zlib library | License text observed. Prefer supported system/NDK-built zlib after format/parity tests; not linked in current harness. |
| ENet-based replacement networking | wonlib project compiles won/host.c, peer.c, protocol.c, enetpp.cxx and messages | Audit component-specific notices/version and adaptation history before reuse. Directory name alone does not establish legacy WON service behavior. Not linked in harness. |
| cereal | won/cereal/cereal.hpp contains BSD-style 3-clause notice | License observed in header. Inventory transitive code before distribution. |
| RapidJSON / RapidXML | won/cereal/external/*/license.txt | Separate bundled license files present; not used by this harness. Detailed transitive review pending. |
| Intel Katmai / Max SDK | 3rdparty/katmai and Max | Architecture/tooling-specific SDK material; no redistribution grant established. Disable/replace SSE-era specializations for ARM; exporter tooling is outside first executable. |
| MSS32 proxy | 3rdparty/Mss32proxy contains naked exported stubs and VS project | Not a native audio implementation; no reason to carry stub DLL exports into Android. |
| C++ standard library / NDK | Original platform code uses C++17; Android built with NDK r27c (27.2.12479018) Clang/libc++ | Toolchain installed separately. Preserve bundled notices if later distributing binaries. |
| Android EGL/GLES/NativeActivity | Original android/native_activity.cpp uses system android/log/EGL/GLESv3 | Linked successfully at API 26; runtime not tested. No additional engine renderer dependency chosen. |

The harness does not confer rights over upstream material. CI fetches a pinned upstream revision into the runner's temporary directory, verifies five file hashes, compiles privately, and publishes only logs. Source-derived libraries, native executables and APKs are deliberately not artifact uploads. A distribution decision requires rights clarification covering both source and compiled derivatives, plus middleware/assets as applicable.

Original materials added here: CMake harness, type/rotate declarations, platform services, diagnostic lifecycle/GLES shell, tests, metadata tools, workflow and authored reports. No copied upstream implementations or game data are committed.

## Supplied installation cross-check

PE version resources confirm Miles 6.0f and Bink 1.0i, matching the inspected headers. The community dr2.exe is x86 with a 2020 embedded build date; dgVoodoo 2.64 supplies its DirectDraw/Direct3D compatibility wrappers. None are added to Android. Distribution of the new asset-test APK is limited to original porting code and standard Android/C++ runtime dependencies: it links no upstream game implementation or proprietary middleware and includes no game assets. Engine-derived artifacts remain excluded.

## Additional source provenance check

The official Google Code archive project metadata (https://storage.googleapis.com/google-code-archive/v2/code.google.com/darkreign2/project.json) records an MIT license selection. Its description says source was acquired through a former developer. The metadata does not confirm uploader/rightsholder licensing authority, and a matching root license grant is absent in the inspected mirrors. Treat the MIT selection as evidence to investigate, not as a verified blanket grant over code, middleware or assets. The current builds keep original code in external temporary overlays; the distributed renderer test uses original adapter code only.

Host GLES tests use installed Mesa/EGL and externally staged Khronos SDK headers. No SDK header copy, Mesa binary or proprietary middleware is committed. Android uses system EGL/GLES and the pinned NDK C++ runtime. Standard TGA decoding and the GLES adapter are original porting components.


## Original Vid state/submission slice

`dr2_vid_state` compiles selected original graphics/vidrend.cpp functions and original graphics/Vertex.h flag definitions from an external hash-checked overlay. The original porting `reign_vid_device` library implements a limited numeric D3D7 state API and requires a real synchronous draw sink; host tests connect that sink to reign_gles. No Windows COM, DirectDraw, original Bitmap manager or hardware lighting is provided. Only transformed/lit indexed triangle lists and stage-zero combinations are implemented. Stage-one disable is accepted, active multitexturing is rejected. New shaders cover 2x/4x modulation, diffuse/texture alpha variants, independent framebuffer factors and texture addressing. See VID_STATE_PROGRESS.md for tested boundaries and unported dependencies.


## Bitmap pixels and opaque/translucent buckets

`dr2_buckets` compiles original PrimitiveDesc, Bucket, BucketMan and TranBucketMan plus original intrusive NList from a SHA-pinned external overlay. The native boundary provides aligned allocation, bounded request conversion, a real TL-only draw sink and sequential indexing for supported nonindexed triangles. Original state grouping, merging, sort comparisons and flush transitions remain. Active multiple texture stages reject. `dr2_bitmap_pixels` retains original Pix setup/integer packing and Bitmap::CopyLine behind a conversion-only facade; this does not compile the full DirectDraw/Bink Bitmap class. The public adapter validates buffer sizes, palette and masks. Native TGA decoding still explicitly excludes five formats present in supplied base.zwp. See BITMAP_BUCKET_PROGRESS.md for behavior retained, source edits, tests and unported managers/loaders.


## Original font/IFace and native atlas storage

`dr2_fonts` retains original Font reader/measurement/draw, selected FontSys atlas allocation/lookup and IFace batching from an external hashed overlay. NativeFont validates serialized extents, isolates source types, scopes atlas ownership and requires explicit GPU upload/draw consumers. BitmapSurface provides real RGBA8 CPU memory, lock/write/publication and repeatable atlas upload data; it is not the full DirectDraw Bitmap manager. Original FtoL truncation is reproduced with a C-compatible cast. Original Font::Draw uses indexed rectangles; its stale DOTRIFAN define is not an active rendering branch. Full FontSys aliases/registration and interface startup remain unported.

The GLES backend now requires core GLES3 framebuffer/renderbuffer/blit support. It owns a reusable RGBA8/depth16 logical target, then presents once with a vertical flip. Resize allocation, readback orientation and lost-context cleanup are tested locally; v0.4 device evidence is still required. No extra text-shaping/font middleware or source/game assets are added to public APKs. See FONT_PROGRESS.md.



### Bitmap file loading checkpoint

Native bounded TGA/PIC framing now feeds the separately compiled original Pix/CopyLine functions. The supplied base archive's111 TGA and453 PIC images load. The inspected `graphics/bitmap.cpp` methods establish channel/encoding rules; unknown PIC chains, grayscale/interleaved TGA remain rejected. Full original Bitmap registry, DirectDraw replacement, mipmaps and reload services are still dependencies. See BITMAP_LOADING_PROGRESS.md; these readers do not initialize Main/GameGod or IFace.


## Original font registration integration

The original FontSys file/alias registry and CreateFont FScope handler now compile against original NBinTree, CRC/StrCrc and the existing native-backed original Font implementation. Per-file CPU atlas ownership replaces old global DirectDraw texture-table ownership at this boundary. Metrics/control/event startup remains a separate IFace dependency; this is not full FontSys/IFace ABI integration. See FONT_REGISTRY_PROGRESS.md.


## Original interface metrics integration

Original IFace metric defaults, CRC table, FScope configuration and reads now compile in the external dr2_iface_metrics slice. Native InterfaceMetrics plus FontRegistry share the original parse and resolve font references by original FontSys CRC lookup. Native viewport dimensions/32-bit RGBA render depth replace Vid::backBmp dimension reads. Actual controls, colors/skins, events, frame setup and Main/GameGod services remain missing. See INTERFACE_METRICS_PROGRESS.md.


## Original bitmap manager integration

The scoped original bitmap manager now uses original NBinTree and CRC naming, fallback/animation/release/mode-change loops, with native TGA/PIC decoding and actual optional GPU upload/delete/unbind callbacks. The full Bitmap class, mipmaps/reduction, BMP, Bink, color-key policies and nonzero stages remain unported. Original Setup/Find directory-key discrepancy is retained and tested. See BITMAP_MANAGER_PROGRESS.md.


## BMP corpus and control dependency checkpoint

BMP decoding now supports bounded DIB40/BI_RGB8/16/24/32 and routes through original Pix/CopyLine; manager .bmp fallback is operational. BI_BITFIELDS remains explicitly unsupported because the inspected original mask reader is unfinished. Pillow is only an optional private24-bit pixel-oracle dependency. Original event/tracking/VarSys/color/control services, full Bitmap allocation/UV/mipmap/transparency and engine startup remain dependencies. See BMP_PROGRESS.md.


## Runtime recovery and requested menu gate — 2026-10-25

dr2_tracking, dr2_events and dr2_vars now stage original runtime implementations externally. Native synchronization/string helpers support the engine services; non-ASCII Windows ACP remains unsupported. Original control/graphics/math/font/frame ABI integration, root configuration, User/Login/Shell and Android input/lifecycle remain menu dependencies. No menu-capable APK is available. Recovered source requires fresh CI evidence after the workspace disconnected.


## Restored workspace, decompiler verification and control types — 2026-10-25

New optional analysis tooling: official rea-agents6.8.0 (MIT), Ghidra12.1.4, Temurin full JDK21.0.13+9, supported Node24.22.1 and Linux procps. Exact official origins, archive hashes/npm integrity, installed versions and actual failure output are in REA_SETUP.md and logs/rea-workspace/installed-toolchain.json. Their tool licenses do not confer redistribution rights on Dark Reign2 or decompiler output. Existing analysis stays separate; executables, assets, Ghidra database and pseudocode are excluded from commits.


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

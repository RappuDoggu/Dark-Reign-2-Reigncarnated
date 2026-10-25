# Original frontend control compilation checkpoint

Date: 2026-10-25. **No original menu APK is ready.**

The new external staging recipe hash-checks original Area/Point, Color, Float2Int, TextureInfo/ColorGroup/PaintInfo/TextureSkin, IFace enums, promotion macros, and the full IControl class declaration. It compiles the original ScreenToWindow, ScreenToClient, ClientToScreen, InClientFromWindow and GetAdjustmentRect implementations in a native static library. The upstream checkout is unchanged; no upstream implementation is committed.

Changes to the staged source are narrow: remove Win32 RECT reinterpretation operators from Area, supply ClipRect=Area<S32>, normalize the legacy `sizeof TextureSkin` expression to `sizeof(TextureSkin)`, and replace forced platform includes with the existing native fixed-width/variable/event adapters. Original rectangle arithmetic, packed ARGB, Float2Int exponent trick and class members remain original. Native utility declarations supply bounded float-to-int conversion and standard min/max/swap; this does not establish general simulation floating-point parity.

First compile failures showed missing imported VarSys declarations, ClipRect/IFaceVar and the old sizeof syntax. A mistaken namespace alias was corrected because VarSys is a class, not a namespace. A test initially referred to a nonexistent dTrack.ptr; the actual original member is trackPos. Logs retain initial errors and final build/test results.

## Verified

- Full existing host suite plus new type target: **26/26 CTest targets pass** with GCC13.3, ASan/UBSan and `ASAN_OPTIONS=detect_leaks=0`.
- Default LeakSanitizer fails in this workspace because its process inspection cannot run correctly. This is recorded as a limitation, not silently treated as a passing leak check. The tracking test's locally generated executable initially lacked execute permission; correcting the local artifact permission allowed it to execute. CI evidence is separate.
- New control type target: **15 checks**, including original inclusive edge/corner behavior, offsets/clipping/sorting, fractional UVs, ARGB channel layout, double-exponent fractional color rounding, native pointer alignment and default paint/skin state.
- NDK r27c/API26 arm64-v8a: the new static library and control_types_smoke compile/link; ELF reports AArch64. **No physical Android execution of this target.**
- Coordinates/border methods compile, but the smoke executable does not instantiate full IControl or pull those methods from the archive. Their unresolved IFace metric service must be supplied during full frontend integration. The 15 checks test original type behavior, not a running control tree.

Logs: [host tests](logs/control-types/host-tests.log), [ARM64 build](logs/control-types/android-build.log), [full control boundary](logs/control-types/full-control-boundary.log). Existing CI automatically builds the new target and runs the added host test; results must be reported only after the run completes.

## Concrete remaining menu work

Full original interface/icontrol.cpp still fails compilation. After supplying the native control declarations, the compiler reaches IFace renderer contracts (Pix/VertexTL), original Input HWND/HINSTANCE declarations, the legacy font PathString dependency, and overlapping utility/Float2Int declarations pulled through stdload/mathtypes. The native EventSys alias and shared VarSys include guard now prevent duplicate legacy variable/event declarations. These require a consistent shared frontend type/backend boundary; adding fake platform handles or empty drawing functions would not demonstrate the menu.

Next: reconcile these headers with the actual native bitmap/font/TL/event services, then compile/link the full IControl implementation and original root/button/window controls; connect original IFace configuration/Process/PaintAll; integrate real profile/login, Vid::InitResources and GameRunCodes Shell via Main/GameGod. The inspected original Shell::Init calls Base::Init (Vid resources) and requires User::LoggedIn, while Base::Process calls IFace Process/PaintAll. Skipping straight to Shell without these services is not an original menu launch.

The original WinMain has now also been identified/decompiled from the owner's executable at 0x004c3930 using standalone Ghidra. See REA_SETUP.md for actual output and limits. Additional source is already available; no further owner asset upload is currently needed. The unchanged public v0.4 APK remains a renderer diagnostic and is not offered as a new menu test.

Reproduce the remaining full-translation-unit failure after CMake stages the native control types:

```bash
python3 tools/probe_original_frontend.py /tmp/reign-upstream /tmp/reign-upstream-runtime-overlay /tmp/reign-upstream-config-overlay /tmp/reign-frontend-native-types --opaque-video-declarations --native-control-types /tmp/reign-upstream-control-overlay
```

This exits 1 and records exact argv, compiler status and false linked/engine/menu flags in result.json. Opaque Bink declarations are only a compile probe; no movie implementation is supplied. The complete full-control probe does not pass.

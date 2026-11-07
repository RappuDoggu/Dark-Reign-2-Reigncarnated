# Full original frontend control compilation — 2026-10-25

## Actual result

CMake target `dr2_frontend_controls` compiles six COMPLETE original translation units to object files with GCC13.3 on Linux x86_64 and NDK r27c Clang for Android arm64-v8a/API26:

- `interface/icontrol.cpp`: base control construction/destruction, geometry, configuration, drawing calls, events and notifications.
- `interface/icroot.cpp`: original root control.
- `interface/icbutton.cpp`: original button behavior.
- `interface/icwindow.cpp`: window and window-title behavior.
- `interface/ifvar.cpp`: control/VarSys bindings.
- `interface/icsystembutton.cpp`: original window system buttons.

This advances beyond the earlier five extracted coordinate functions. The complete implementations compile, but are NOT linked into a runnable frontend. Six objects per architecture were checked with readelf. Existing 26 host tests pass with ASan/UBSan and local LeakSanitizer disabled for the previously documented procfs limitation. These tests do not instantiate the newly compiled controls. No new Android execution, menu rendering or APK launch is established.

## Reviewed changes

`tools/prepare_frontend_controls.py` checks input hashes and stages all upstream material externally. Original source checkout remains clean. The repository contains the recipe, original adapter declarations and evidence, not generated game implementation.

- Replace dependence on legacy precompiled headers with explicit native declarations and original interface headers.
- Reuse existing native Event/VarSys/tracking types; prevent neighboring headers from redeclaring the incompatible legacy Event.
- Correct Array's forward declaration to include its original alignment template parameter; retain the original Array implementation in external staging. Its allocation functions remain unresolved dependencies.
- Preserve integer Color overload selection with explicit S32 values instead of Windows-sized `long` literals.
- Make Reaper-to-pointer conversion explicit in an ambiguous conditional; make a read-only string-search pointer const.
- Stage only needed original StdLoad declarations/Range and the original multilingual buffer-size macro, avoiding unrelated Windows and math dependencies. These declarations do not implement the loaders.
- Exclude the unused Windows Input::Init declaration from the control header. Preserve legacy keyboard scan codes for Tab, Return, Space and Escape; native input-event production still needs integration.
- Move the active Bink-texture shutdown operation behind `reign::deactivate_control_movie(Bitmap*)`. It is deliberately UNDEFINED, not an empty success stub. A real implementation or explicit supported-media policy is required before linkage.
- Retain original global Font declarations. Connecting them to the existing namespaced native font implementation remains outstanding; compilation does not prove ABI compatibility.

## Failures and limits

The first attempt encountered duplicate Event declarations, missing Unicode utility overloads, incomplete Array/Bitmap, ambiguous Color and Reaper expressions, and dependencies formerly supplied by precompiled headers. Further attempts exposed missing message-box, interface-state and system-button declarations; explicit original headers resolved these. An initial build command could not find the new target until CMake was reconfigured.

The actual executable-link attempt includes all six objects and existing host static libraries. It fails on unresolved IFace rendering/state/event functions, Font methods, Input state, Babel/StdLoad and utility services, remaining control classes and the movie adapter. The linker output is saved, not replaced by success stubs. Compiler warnings about legacy font multicharacter constants and assertions in noexcept destructors remain; neither proves a runtime failure, and both need integration review.

REA retry: `ps -axo pid=,ppid=,pgid=,uid=,stat=,command=` still reports `fatal library error, lookup self`. Creating a private corrected PID/procfs namespace fails with `unshare: write failed /proc/self/uid_map: Operation not permitted`. No host remount/reset or process-check bypass was performed. Continue standalone Ghidra/source porting.

## Reproduce

Use the existing separate pinned upstream checkout and normal project SDK configuration:

```sh
cmake -S . -B build-host -DREIGN_UPSTREAM=/absolute/external/dr2-upstream -DREIGN_SANITIZE=ON
cmake --build build-host --target dr2_frontend_controls control_types_smoke
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure
cmake --build build-android --target dr2_frontend_controls control_types_smoke
```

Configure `build-android` first using the existing Android instructions in ANDROID_PORT_PLAN.md. The target is included in the default host/Android build, so existing CI will compile it. No source-derived object or APK is uploaded by this change. Local evidence: [result](logs/frontend-controls/result.json), [host build](logs/frontend-controls/host-build.log), [Android build](logs/frontend-controls/android-build.log), [tests](logs/frontend-controls/host-tests.log), [link errors](logs/frontend-controls/link-failure.log). The exact local link command is in result.json; its `link-main.cpp` is simply `int main(){return 0;}`. No link-check executable was produced.

## Next integration work

Resolve the actual IFace implementation and shared global Font/Bitmap services, then link and exercise an original interactive control tree with configuration and events. Carry that into Main/GameGod, real profile/login and Shell initialization before packaging a menu APK. Existing source-derived library slices alone do not satisfy those runtime dependencies.


## CI verification

Run [40203206699](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40203206699) passed host and Android jobs on implementation commit `c098b4758549bdbfea23b0d554509b904f14575d`. Both job logs explicitly report `Built target dr2_frontend_controls`; host reports 26/26 tests passing. The renderer APK artifact remains the unchanged v0.4 diagnostic, not a main-menu build. [Machine-readable evidence](logs/frontend-controls/ci-result.json).

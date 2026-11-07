# Frontend font integration — 2026-10-25

The original UI translation units now consume an original porting adapter named Font, preloaded before legacy font headers. Its Height, Width and Draw operations delegate to NativeFont, which executes the already staged original font reader, layout, drawing and IFace batching. Global FontSys::GetFont overloads resolve through the existing FontRegistry, whose registration/alias logic is also staged original code. There is no replacement text renderer or dummy lookup result.

FrontendFonts binds a registry, frame viewport/shadow alpha and required batch consumer. A draw without a configured frame or active binding fails explicitly. Each emitted batch carries its NativeFont identity to the consumer, allowing separate fonts with atlas index zero to use distinct textures. The consumer still has to upload/bind those atlases and submit the batches to GLES; the complete UI frame loop is not implemented here.

The adapter is not binary-compatible with the old global Font layout. All six frontend translation units are rebuilt against the same preloaded declaration. No cast between legacy and native font objects is used. Scope restoration is exception safe. Font wrappers retain shared immutable NativeFont instances, so reload creates new wrappers and old controls retain valid previous fonts until the bridge is destroyed. This intentionally retains older font versions for the session lifetime; a future controlled UI teardown can reclaim them. Destroy controls first, then bridge, then registry; serialize access. The existing 65,535-code-unit NativeFont input limit remains explicit.

## Verified

- 40 adapter checks: alias/CRC lookup, shared identity, absent font, width/height, empty/negative lengths, invalid input rejection, required frame setup, full vertex/index/state equivalence with direct original drawing, explicit clipping, nested context exception restoration, draw-consumer failure recovery, reload and clear lifetimes.
- All 32 host tests pass with ASan/UBSan. Local LeakSanitizer remains disabled for the known procfs limitation; CI uses its normal sanitizer environment.
- The full frontend executable link still fails, now with 34 unresolved symbols versus 38 before this increment. The four font symbols resolve through this adapter. No symbol count is interpreted as percent complete.
- No genuine control tree, original menu launch, new on-device execution or gameplay result is claimed.

Reproduce: configure the existing host CMake build, build `frontend_font_smoke`, then run the full CTest suite. [Build](logs/frontend-font/build.log), [focused execution](logs/frontend-font/test.log), [suite](logs/frontend-font/suite.log), [full-link failure](logs/frontend-font/link.log), [exact link command and remaining symbols](logs/frontend-font/result.json).

Next dependencies include frontend drawing primitives, bitmap/texture/skin integration, IFace data/metrics/frame state, input and script-expression services. A real root window must link and run before menu startup can be tested.

## CI verification

Commit `4614983d45d46327e44995e3ef4b1093395416f1` passed both jobs in [run 40216274121](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40216274121). Host execution passed 32/32 tests, including 40 font adapter checks, with normal sanitizer settings. Android built reign_frontend_fonts and frontend_font_smoke for ARM64. GLES checks and existing diagnostic APK packaging passed. [Provider output and artifact metadata](logs/frontend-font/ci-result.json). No new device runtime or menu launch is claimed.

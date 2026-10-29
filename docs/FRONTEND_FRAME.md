# Frontend metrics and frame state — 2026-10-25

FrontendFrame connects the original controls' global IFace API to the existing InterfaceMetrics instance and explicit engine timing. GetMetric calls the staged original bounds-checked GetMetric implementation by MetricType index. ScreenWidth, ScreenHeight and ScreenDepth read the current native viewport; they no longer require the legacy DirectDraw back-buffer object. The existing viewport adapter describes a 32-bit target and must be resized by the renderer owner when its actual target changes.

ElapsedMs, TimeStepMs and TimeStepSec return caller-supplied engine values. There is no independent timer or automatic frame advancement. Original U32 millisecond wraparound and a separately supplied float second delta are preserved. Timing access fails until set_time has been called. Nonfinite or negative second deltas are rejected before mutation; zero deltas are valid. Metrics can be queried before timing is initialized.

Bindings restore the previous session even after exceptions. Metrics must outlive the frame adapter and calls must be serialized on the UI thread. This does not start an engine loop, create a GPU surface, construct a root window or initialize IFace::data.

## Verification

43 checks pass: all original metric indices, invalid index rejection, initial viewport, resize visibility, timing-not-supplied errors, exact supplied values, zero delta, U32 counter wrap, invalid seconds preserving previous state, nested-session isolation and exception restoration. The full host suite passes 33/33 with ASan/UBSan; local LeakSanitizer remains disabled for the existing procfs issue.

The full frontend link still fails with 30 unresolved symbols, down from 34. GetMetric, ScreenWidth, ScreenHeight and TimeStepMs now resolve. Fonts were connected in the prior increment. Remaining dependencies still include UI drawing, texture/skin integration, IFace data, input, sound and script-expression services. No original window or main-menu APK is established.

Reproduce using the existing CMake host configuration, build frontend_frame_smoke, and run CTest. [Build](logs/frontend-frame/build.log), [focused test](logs/frontend-frame/test.log), [suite](logs/frontend-frame/suite.log), [full link failure](logs/frontend-frame/link.log), [exact link command and unresolved list](logs/frontend-frame/result.json).

## CI verification

Both jobs passed for `0d46e07800dfb7504aefcfb7019ff4e499dbdb2d` in [run 40220853532](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40220853532). Host tests passed 33/33 with normal sanitizer settings, including 43 frame checks. Android NDK compiled reign_frontend_frame and frontend_frame_smoke for ARM64. Software GLES checks and existing diagnostic packaging passed. [Provider output and artifact metadata](logs/frontend-frame/ci-result.json). No new device execution or menu launch is claimed.

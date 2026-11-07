# Original runtime services and menu gate — 2026-10-25

The requested original-menu APK gate is NOT achieved. No replacement menu or refreshed diagnostic is delivered as satisfying it.

## Implemented services

The external, hash-verified runtime overlay compiles original DTrack/Reaper/TrackSys, EventSys, String, VarSys/VarScope/VarItem/VarPtr. No upstream implementation or assets are vendored. Native wrappers provide event contexts, synchronization, fixed-width helpers and bounded formatting/path copies.

Tracking preserves original32-bit IDs/cache/liveness and native pointers. Its native Info layout is asserted16 bytes, pointer offset8. Event has a runtime-only natural40-byte layout with the interface union at offset8. Generic parameters remain32-bit; interface pointers are not narrowed. This defines no wire/save format. Full source producer/serialization integration remains future work.

Event registration, context lookup and dispatch are original code. The original work queue retains127 usable entries and deferred callback-posted delivery. The native128-slot mutex queue replaces Windows synchronization. Source SafeQueue Full always returns FALSE, Empty confuses wrapped/full indices, and Flush is a no-op. The native boundary corrects those defects. Original FlushEvents still ignores its type argument: the native flush clears all pending types. Register/modify/consume on one thread; producers may enqueue after registration. Blocking timeouts and dispatch reentry are unsupported.

VarSys preserves typed values, scopes, clamping, notifications, commands, substitutions and death-tracked handles. Binary cleanup changes scalar delete to delete[]; String::Make formatting and integer/path buffers are bounded. Type-name strings are const-correct. Windows ACP conversion is implemented only for ASCII; other bytes fail explicitly. Preferences/VarFile and full control integration remain unported.

## Observed local results before workspace disconnection

25 host ASan/UBSan tests passed:12,506 tracking checks,1,072 event checks,19 variable checks and an event/tracking/variable/deferred-command handoff fixture. That fixture uses a synthetic target, not IControl or a menu. NDK r27c/API26 compiled/linked the new AArch64 executables, including native-layout assertions. The existing51 GLES checks passed per context in two contexts. No new original-derived Android execution occurred.

Actual local failures: missing GameIdent during initial variable compilation; a fixture used IsValid instead of Alive; another selected the ident-string overload instead of void* context (fixed by explicit cast and teardown). A private regression object restoring only the original scalar deletion reproduced ASan alloc-dealloc-mismatch. The fixed module passed. Stale Android cache paths after temporary-tool cleanup were resolved by a clean build directory.

## Failed original frontend probe

The original interface/icontrol.cpp was probed using real ported variable/event headers. Normalized external vendor header copies still failed RAD/Bink platform detection. A second declaration-only opaque-HBINK probe (no decoder implementation) exposed Area/graphics/math integration, conflicting utility declarations and calling-convention errors. Neither attempt linked or executed controls. The12-error limit does not enumerate the full remaining graph. The probe tool reports the compiler exit code and leaves linked/engine_initialized/menu_rendered false.

Source inspection confirms normal GameRunCodes Base startup calls Vid::InitResources, including material/light/mesh setup. Processing calls IFace::Process/PaintAll, and Shell requires a valid User. IFace initialization also needs original controls, colors/skins, sound, commands/cursors, root/overlay objects, root-scope dispatch and input/frame/lifecycle integration. Existing subsystem tests do not satisfy those combined paths.

## Recovery and evidence limits

The workspace disconnected before the local files/logs could be committed or uploaded. The implementation was reconstructed from this session through the GitHub integration. Committed bytes must be assessed against fresh CI, not assumed identical to the disconnected workspace. Local raw logs were not recoverable at commit time; the observations above are session notes, not fabricated log artifacts. CI evidence for the recovered commit is recorded separately in BUILD_STATUS.

Next: compile/link actual IControl/ICRoot against complete native Bitmap/color/math/font/frame interfaces, initialize original root configuration and Login/Shell/user state, then wire Android input/lifecycle. A useful device APK must display the original menu with original controls/configuration. Gameplay additionally needs world/mission/simulation integration. No additional assets, disc install or unchanged diagnostic retest is requested.

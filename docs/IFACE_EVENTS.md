# Original interface state and events — 2026-10-25

## Implemented

`dr2_iface_events` now compiles and links 24 original IFace methods from the inspected `interface/iface.cpp`: system flags; direct/queued events and notification helpers; event ID; focus, capture, modal and tooltip state; and the original queued-event handler. Original implementations are hash-checked and staged outside the repository. State references are redirected into a scoped native session while original diagnostics/comments are retained.

`reign::InterfaceEvents` supplies the session binding. It can own a diagnostic queue or borrow the engine's existing EventLoop, preserving a shared input/interface ordering model. Original methods execute inside `run`; the central engine queue can invoke the registered interface handler without a separate UI process loop. `process` uses the original EventSys work-batch behavior. `empty` includes pending work-batch events and, when sharing, other event types.

The session must be used on its owning thread, with controls destroyed and event producers stopped before shutdown. A borrowed event loop must outlive the session. Shutdown removes only this interface's queued Reapers from producer and interrupted work queues, preserves other event types in FIFO order, clears the callback, and disposes modal-stack Reapers. Additional IFace porting must reuse this state rather than define a second independent focus/modal/event state.

## Actual fixes

- The upstream `iface.h` PostNotify declaration omits the recipient parameter present in its implementation. The staged header now matches the actual implementation. The first test compilation exposed the mismatch; neither the upstream checkout nor its Windows build files were edited.
- Original PostEvent allocated Reapers while holding the new native queue reservation. Allocation/validation now happens before reservation with unique ownership, so an exception cannot strand that mutex reservation. Full-queue rejection remains explicit, and ownership transfers only after publication.
- The original handler frees its Reapers on normal return. The native dispatcher also owns them on an exception path, releasing its guards after a successful original-handler return. Shutdown cleans notifications left in the original work queue following a failed handler.
- The original control object files now use function/data sections. The narrow event test links real original control objects and uses linker section collection to retain genuine IControl RTTI for UBSan while discarding unreachable drawing/control-construction paths. No dummy RTTI or fake IControl implementation is supplied. The full-control link probe deliberately does NOT use section collection.

## Verified execution and limits

The interface test passes **178 checks**: independent state bindings and exception restoration; flag changes; initially empty focus/capture/modal; absent-focus/modal notifications; rejection of a null direct receiver; 128 queued dead-recipient notifications; full-queue rejection; original 127-event processing batches; normal shutdown cleanup; interrupted-work-batch cleanup; central shared-queue dispatch; and preservation of unrelated event order during both shutdown paths.

These tests execute original queued-notification and dead-recipient handling. They do **not** construct a live IControl/window, exercise focus transitions between live controls, draw the interface, or initialize the original main menu. All 24 methods compile, but not every live-control branch is covered.

The entire host CTest suite passes **30/30**, with ASan/UBSan including vptr checks. Local LeakSanitizer remains disabled because of the documented procfs restriction; CI uses the normal sanitizer environment. A deliberately malformed queued event exercises the original handler assertion and leaves pending Reapers for shutdown cleanup.

The full-control executable link still fails: **49 unique unresolved symbols**, compared with 62 in the previous probe. This is not a percentage-complete estimate. Remaining services include real control creation/registration, global Font/Bitmap integration, drawing, Input, timing, Babel and other startup dependencies. No main-menu APK or new on-device result exists.

## Failures preserved

The initial test could not compile against the incorrect PostNotify declaration. The next link exposed missing IControl RTTI under UBSan; linking the real sectioned original objects resolved it without disabling vptr checks. The full suite initially could not execute an existing bitmap-manager test artifact; it lacked execute permission and then proved to have invalid ELF magic. Re-linking that generated executable, verifying its ELF signature and rerunning the suite restored 30/30. No cause for that local artifact corruption is asserted and no source change was made to bypass its test.

## Reproduce

```sh
cmake -S . -B build-host -DREIGN_UPSTREAM=/absolute/external/dr2-upstream -DREIGN_SANITIZE=ON
cmake --build build-host -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure
```

Use the existing Android NDK configuration for the same default targets. Original source, objects and source-derived executables remain outside published artifacts. CI verification is recorded after completion.

Evidence: [interface test](logs/iface-events/interface-test.log), [suite](logs/iface-events/suite.log), [build](logs/iface-events/build.log), [full link errors](logs/iface-events/link.log), [result and exact link command](logs/iface-events/result.json), [initial declaration error](logs/iface-events/initial-declaration-failure.log), [RTTI link error](logs/iface-events/rtti-failure.log).

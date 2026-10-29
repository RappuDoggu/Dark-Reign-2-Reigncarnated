# Startup sound and console controls — 2026-10-26 UTC

Externally staged original IFace sound configuration now resolves named effects
through an explicit native loader and commits each effect only after volume
validation. The real installation's sound references load in the configuration
probe; attempting playback there throws because no audio device is attached.
This is configuration validation, not audible playback or an audio backend.

Added original Edit, ConsoleEdit, ConsoleViewer, TabGroup and TipWindow translation
units. Original console ring buffer, filtering, enumeration and clear functions
are staged. Native hook registration retains full pointer keys, replacing the
original truncation to U32. Console colors now belong to the scoped theme, allowing
the original viewer to use their lookup during rendering. Native font average
width and standard-cursor lookup support Edit; caret state follows frame timing.
Text wrapping now rejects a missing font instead of dereferencing null.

Limitations remain explicit: clipboard operations, non-ASCII edit conversion,
console completion/history, list directory enumeration and non-ASCII list sorting
are not integrated. Those paths throw when invoked. No replacement gameplay or
fake successful service result is provided. Caret notification dispatch is not
integrated; the frame bridge exposes the polled state.

Host ASan/UBSan: 51/51 tests pass (local LeakSanitizer disabled). Console tests now
cover 27 checks including filtering, hooks and 100-row rollover; live frontend
configuration has 21 checks including ConsoleViewer message receipt/rebuild.
Effect routing/configuration has 15 checks. Initial test fixtures lacked filter
cleanup and the viewer's selected-item variable/font; corrected fixtures pass.
Two executables initially lacked execute bits; the repaired full rerun passed.

Actual original Login config: statement 107, 25 registered classes, 9 font aliases,
4 bitmaps, 46 cursors, one cursor geometry batch. Next blocker: Vid::Options base
class. The source uses Windows display/card/mode state and needs an Android-aware
adapter. Original Login/Shell lifecycle, main-menu rendering and a main-menu APK
are still not demonstrated. No additional user assets are currently required.

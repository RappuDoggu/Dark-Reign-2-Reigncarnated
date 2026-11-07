# Original list and menu controls — 2026-10-26 UTC

Added externally staged original ICListBox, ICListSlider, ICSlider and ICMenu
translation units. Source/header hashes are pinned. ReaperList is staged with
standard dependent-name qualifications and binds to the existing original tracked
pointer implementation. Original BreakText now uses the native font-width bridge.
Added bounded UTF-16 duplication, explicit ASCII-only case-insensitive comparison,
color loading and native right-button identity. These are source compilation and
platform adaptation; no executable decompilation is required for these functions.

Unsupported operations fail explicitly: FillFromPath requires directory enumeration;
non-ASCII list sorting requires Windows-compatible case mapping. They must be
implemented before claiming complete widget behavior. The original menu's static
callback initialization state remains serialized, as in the current engine host.

Live tests add two profile items, select ProfileB, clear the list, construct an
original ICMenu and add an action. The configuration smoke test passes 17 checks;
full local ASan/UBSan suite passes 51/51 with LeakSanitizer disabled. One test
initially lacked execute permission; after repair the full suite passed. Retained
compiler failures show the template/header-order and missing API issues resolved.

The real Login configuration advances to statement 104 with 19 control classes,
9 font aliases, 4 bitmaps and 46 cursors; its default cursor produces a geometry
batch. Next explicit stop: UI Sound configuration. No original Login/Shell lifecycle,
main-menu screen or main-menu Android APK has been demonstrated.

The preceding cursor commit c0cf4e8 passed clean host and Android CI in run
40301713007 (provider conclusion success). This widget increment requires its own
CI result. Remaining included control bases also include TabGroup, TipWindow,
SoundConfig, Vid::Options and Game::SaveLoad. Login itself requires Game::Login
and user-profile services. Then the actual lifecycle must be attached to the
Android surface/input host. Source availability avoids rediscovering these
implementations; it does not remove their platform dependencies.

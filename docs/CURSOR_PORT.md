# Cursor and console-color source port — 2026-10-26 UTC

The original CursorSys classes, creation, derived cursors, selection, animation
and geometry generation now compile through a hash-verified external stager.
Native scoped ownership replaces global cursor storage. Exception cleanup prevents
partially configured cursor/frame leaks; animation interval arithmetic avoids
signed overflow. The original console-color configuration now owns its tables in
the scoped interface configuration service.

Actual asset-backed Login probe: 96 interface statements attempted, 7 registered
control classes, 9 font aliases, 4 bitmaps, 46 cursors, 128 key names and 12
modifiers. Selecting/drawing the configured default cursor produced one real
textured geometry batch. Next explicit blocker: Base control class [Listbox] not
defined. This does not demonstrate menu display, original Login lifecycle or APK.

Host suite: 51/51 PASS with ASan/UBSan, local LeakSanitizer disabled. Cursor tests
include 15 checks of ownership, invalid references, animated bitmap texture
coordinates, drawing, and maximum frame interval. Initial failures exposed missing
texture/color declarations and original RenderRectangle default argument handling.
An empty IControl object and a damaged runtime_handoff executable also required
recompilation/relinking; missing execute permissions were restored. Logs retain
both failures and the passing rerun. No cause of artifact corruption is asserted.

The preceding configuration commit 6ab072a passed clean host/Android workflow
40299812334 (provider conclusion: success). New cursor changes require their own
CI verification. Assets and original source remain outside the repository.

Next source dependencies inspected: ICListBox requires ICListSlider and ICSlider,
UTF-16 text comparison/duplication and BreakText, plus Dir enumeration for
FillFromPath. The Login profile list also requires original user-profile lifecycle
services. These are real runtime dependencies, not a packaging-only task.

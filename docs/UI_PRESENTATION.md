# Original UI presentation integration — 2026-10-26 UTC

Fixed the submission order between original UI geometry and font batches.
Previously geometry was collected until the draw callback returned, while fonts
could be submitted immediately: a later background submission could cover text.
Both now enter one ordered collection. Deferred font commands retain their font
owner and batch data; nested contexts restore the enclosing collection. A host
regression test checks background, font and foreground callback order.

Added FrontendPresenter to send original UI and font batches to LegacyGles with
alpha blending, actual texture identity, address mode and nearest/linear filtering.
It caches GPU uploads within each frame and releases them before the next frame.
This favors correctness during integration; no persistent-cache performance claim
is made. Frame cancellation and explicit context-loss abandonment are supported.
Source font/bitmap owners must survive submission; GPU context must survive normal
destruction. No proprietary images or font assets are committed.

The new software-GLES test draws original font coverage over a red background,
then a green foreground rectangle, verifies mixed pixels, repeats a frame and
checks recovery after a draw exception. It runs in CI; local host compilation alone
does not establish its GPU result. Existing Android diagnostic packaging remains
unchanged and does not include this as an original-menu runtime.

Host sanitizer suite: 51/51 PASS in a new build-ui-integration directory, local
LeakSanitizer disabled. The incremental directory had a stale test object after a
render-state layout change. The clean build also produced missing execute bits
and a damaged console executable; permissions were restored and that exact target
was relinked before the passing rerun. Failure logs are retained, and clean CI is
required as independent evidence. No filesystem fault cause is asserted.

The preceding startup-controls commit 3130e1d passed clean host and Android jobs
in workflow 40306961621. Real Login configuration remains stopped at Vid::Options.
No main-menu APK, audible sound output or gameplay is demonstrated by this change.

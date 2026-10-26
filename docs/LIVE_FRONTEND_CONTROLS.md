# Live original frontend controls — 2026-10-26 UTC

The complete ten-object staged UI target now links without unresolved symbols,
without section garbage collection. This establishes the staged target's link
closure, not full engine startup or execution of all linked methods.

A native host test now constructs an original ICRoot, registers the original
ICButton and ICStatic factories, creates a button through IFace::CreateControl,
activates the tree, looks up the button by name, draws it, and shuts it down.
It then constructs the original modal message-box window with three buttons and
static text, draws through original controls and font batching, sends an actual
IFace NOTIFY event, executes the selected MBEventCallback, deletes the marked
window and verifies modal release/death tracking. The test uses synthetic font
and text fixtures. It does not load the owner's menu or invoke Shell gameplay.

Focused result: 20 live-control checks, 21 geometry batches. Whole host suite:
47/47 PASS with ASan/UBSan and local LeakSanitizer disabled. Original sound routing
has 11 focused checks. The static texture test also checks the deactivation path.

## New code and boundaries

- Original iface_messagebox.cpp and icstatic.cpp are staged externally with
  content-hash checks. Original sheet rendering/layout/loading helpers accompany
  the static control. Sheet images use the existing scoped static image manager;
  controls must die before that manager and its cached wrappers.
- Message-box bounds are checked before indexing its 20-line array. The original
  three-button loop read events[3] before checking its bound; that order is fixed.
  Trailing CR/backslash no longer advances past the terminating NUL. Overlong text
  and gaps in the supplied button list fail explicitly.
- The static timer uses bounded Babel formatting instead of a Windows wchar_t
  cast. Nonfinite, negative or excessive timer values fail explicitly.
- IFace::Sound::Play is the original routing body. A scoped native table forwards
  configured effects and their volume to required playback callbacks. Empty
  entries preserve original Init behavior. A retained owner allows a callback to
  clear its own table entry safely. This is routing, not audio decoding/device
  playback; sound asset Configure/Load and Android output are still unimplemented.
- The current Bitmap type only contains a decoded static CPU surface and cannot
  hold a movie. Its deactivation adapter therefore follows the original
  no-Bink branch. Movie-capable textures remain unsupported; adding that
  capability must also implement actual movie shutdown. This is not a working
  Bink implementation or a blanket no-op for an arbitrary original Bitmap.
- The real control target now explicitly links the existing RunCodeHost adapter,
  which requires an actual Main::ScopeHandler consumer when that path executes.

Sheet rendering and timer paths compile as part of the whole original ICStatic
class but are not individually runtime-validated by this increment. Message-box
allocation/configuration exceptions may leave children attached to the root;
failed startup sessions must tear down their control tree before other services.
There is no claim of transactional menu initialization.

## Reproduction and observed failures

Use the existing sanitized host CMake build and CTest. Run
`python3 tools/probe_frontend_link.py BUILD EXTERNAL_OUTPUT --sanitize` for the
full link check. The probe records linked=true while deliberately retaining
engine_initialized=false and menu_rendered=false.

The live target first exposed a missing explicit dr2_runcodes dependency for
Main::ScopeHandler, which the all-libraries diagnostic had included automatically.
Static sheet staging required an explicit control_stdload.hpp include. Both are
fixed in the actual target. A test attempted to inspect a protected control list;
it now verifies destruction through the real Reaper instead.

The local workspace also produced an invalid font object and an invalid theme
test executable; missing execute bits prevented some initial test starts.
Invalid files were preserved outside the repository and rebuilt/relinked;
initial and final test logs are retained. Their cause is not established.
Independent clean CI is the authoritative cross-check, not an assumption that
those failures were harmless.

## Remaining main-menu work

The test installs root pointers only for its scoped lifetime. A production
frontend session still needs owned root/overlay lifetime, complete configuration
execution, remaining menu widget registration, Main/GameGod command handlers,
Login/Shell transitions, real installation resources and Android frame/lifecycle
integration. The shell configuration also launches a shell mission, so rendering
these synthetic controls does not establish a working original main menu.

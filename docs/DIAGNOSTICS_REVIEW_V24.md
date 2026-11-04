# APK 24 campaign diagnostic review

The owner supplied a private diagnostics ZIP from source
`fa43ad3a7fa0fb750ff6a7721dafd5e2a44b766a`. Its embedded native hashes match
the delivered APK 24 receipt. Raw reports, device identifiers and platform trace
bytes stay outside the public repository; this file records only relevant facts.

## Current J01 failure

- Campaign J01 passes object creation, sight, team/player post-load, client
  preparation and entry into the simulation state in about 350 ms.
- The next frame returns the caught exception `Empty engine path`. The report
  contains zero simulation cycles and no completed frame sample. This is a
  stopped render loop, not evidence of a blocked disk import or slow map load.
- The old readiness rule reported READY on entry into the simulation state,
  before a simulation cycle. A subsequent exception kept that stale flag.
- AAudio is running at 48 kHz, effects volume 1, with no callback error. The
  mixer has zero effect requests/starts and no decoded clips in this session.
  This report cannot validate attack/hit sound playback because gameplay never
  progresses to combat. Music reports seven tracks and active playback.

`Empty engine path` originates in the native OS-path converter. The uncaught
filename-splitting path (`Dir::PathExpand`) incorrectly calls that converter.
The original `_splitpath` accepts an empty string, returning empty components;
`Bitmap::Manager::FindCreate` then handles the optional/extensionless name through
its original no-texture return path. Splitting also must preserve case, slash
spelling and drive components without probing the filesystem.

An I/O regression and a real original bitmap-manager integration check both
reproduce the same exception before the fix. The fix separates lexical splitting
from OS lookup, preserving the original optional-name behavior. Actual File::Open
still rejects empty OS filenames and unmapped Windows drive paths; arbitrary
rendering failures and unsupported operations are not swallowed. The report does
not contain a throw-site stack identifying the precise J01 texture caller, so
retail J01 must be retested on the phone.

The new readiness rule waits for a nonzero simulation cycle. Fatal JNI startup/
frame exceptions record FAILED, clear readiness and remain visible for diagnostic
export. Nonfatal operation errors remain explicit and do not stop simulation.

## Historical native crash

The platform trace is from an older main-process exit roughly 65 minutes before
this collection. Its captured fatal thread is in MenuActivity.openNative, not
MissionActivity or the current J01 frame. The menu ELF build ID matches the
retained unstripped library. Symbolication identifies FileSys::Close/DataFile
buffer destruction, called by BlockFile::Open/PTree nested configuration loading.
It is distinct evidence and is not attributed to the current campaign exception.
The trace is capped at 256 KiB; complete backtrace fields preceding truncation
were decoded. This review does not establish the underlying corruption source
or claim a fix for that historical main-menu fault.

The mission linker now emits a deterministic GNU build ID so future platform
traces can be matched to the exact retained stripped/unstripped mission ELF.
The source receipt still records exact SHA-256 native hashes.

## Validation and remaining checks

The old source fails the new empty-name I/O and original bitmap-manager checks.
The patched source passes 12 foundation tests under ASan/UBSan, including lexical
split cases and FAILED-after-READY/reset checks. The real original bitmap-manager
lookup, reference Instant Action and synthetic campaign lifecycle each pass;
both mission paths load, simulate, pause/resume and close twice. The integration
loop asserts that zero simulation cycles never reports READY. The fixture is
not retail J01; no retail campaign completion, Android lifecycle, combat audio
or new phone FPS result is claimed.

Retest J01 first. If it stops, collect a fresh mission ZIP; if it proceeds, also
collect after attacking/being attacked so sound resource counters are meaningful.
A new main-menu crash should be collected after reopening the app, keeping its
historical trace distinct from any current mission error.

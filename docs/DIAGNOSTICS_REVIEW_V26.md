# APK 26 diagnostic review: camera handoff, JDA evidence and placement

The owner's private archive matches source
`7254c6418b7bdf80575bc8f9d39eb57634a92f38` and APK 26's exact native hashes.
The owner reports Sprawler's camera now reaches the start, a map-pan/camera
error afterward, JDA still stopping during Chapter 1, and working Instant Action.
Raw reports, screenshots, platform traces, game packs and signer remain private.

## Current evidence

Sprawler S01_Intro records EndCineractive at cycle 945, then continues simulating
to cycle 1401. It reports cinematic=0 and movie=0, but the current camera remains
Viewer::Action::Cinematic. Pan then emits repeated Standard-camera errors. This
is a real handoff gap, separate from the old letterbox rendering boundary.

Source inspection also finds the original automatic next-mission selection calls
Main::runCodes.Set(Mission), whereas this native runner owns Game::RC directly
and has no desktop outer Mission/Shell loop registered. Those requests were not
reaching Android. The fix routes original Mission/Shell requests to a deferred
Android handoff after a complete engine frame. Original campaign ordering,
selected mission, result/progress rules and difficulty policy remain; there is
no hardcoded Sprawler mission skip. Android snapshots/closes the old runtime,
opens the selected target, or returns to the menu at campaign end.

At a cinematic end, if its camera still owns the view, the app restores the
original default camera and inherits its view. It does not override a script's
explicit switch to another camera. Touch pan is disabled while cinematics or
handoffs own the camera, rather than submitting invalid controls every move.
Unknown native control/rendering operations still report errors explicitly.

The ZIP's last native report is Instant Action 2siege; the previous one is
Sprawler. JDA was pushed out of the two-report ring. No current JDA caller stack
remains in the archive, so the exact invalid variable/root cause is still unknown.
The old nested manual mission ZIP is still from APK 24 and is not used as current
throw-site evidence. APK 27 keeps the last two distinct failures separately from
the two ordinary reports. Successful missions cannot overwrite failure evidence.
Each report carries mission, timestamp and source receipt, bounded to 128 KiB.

Instant Action reports 350 simulation cycles, 120 FPS and native audio starts/
audible callbacks. This corroborates the owner's working Instant Action/audio
report at the captured scope; it is not a new APK 27 benchmark. The Sprawler
report also retains rejected geometry with invalid UVs, including NaN. That
unresolved rendering evidence remains explicit; this patch does not sanitize or
silently discard those operations. An old audio-driver error string also remains
in cumulative diagnostics; individual missing/invalid sound assets are unclaimed.

## Chosen placement scheme

The owner explicitly selected: tap to anchor, more single taps to rotate,
double-tap to confirm. APK 27 implements this instead of three-finger rotation.
After anchoring, tap down/up never updates the native pointer; rotation and
confirmation use the saved position. A pending single rotation waits 300 ms,
and a double tap cancels it before confirming. Drag deliberately repositions;
hold does not confirm placement. Ordinary left click/drag and hold-to-choose a
build control remain. Two-finger pan remains, with no pinch/twist zoom.

## Validation and limits

55 Java gesture checks and 13 report-retention checks pass. They verify native
cursor coordinates, no extra rotation on double tap, reposition, cancellation,
normal input, and JDA failure evidence surviving later successful missions with
its original source receipt and stack text. All Android Java compiles for SDK 35.

12 engine foundation tests pass. The authored original cinematic test switches
to the real cinematic camera, verifies pan is locked while active and restored
afterward, checks fade/letterbox pixels and shroud/input/HUD restoration. A
private two-map campaign fixture uses original NextMission selection, delivers
a deferred Followup target, closes/loads that target and routes the final mission
to the menu. These campaign/navigation and reference Instant Action lifecycle
checks pass twice, including simulation and pause/resume/cleanup. They are not
retail JDA/Sprawler completion or Android JNI/lifecycle execution.

The pinned 1,791-source-file verification passes. The changed original mission
unit and ARM64 adapters are rebuilt and linked with original flags; the verified
unchanged menu/renderer and original objects are reused. Native ABI layouts,
10 Hz simulation and fenced append-only unsynchronized streaming remain. The
reproduction command rebuilds everything from source. Retest Sprawler and JDA,
then collect a new ZIP; JDA's failure should now survive later Instant Action.

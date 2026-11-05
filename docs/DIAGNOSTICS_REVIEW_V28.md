# APK28 Instant Action diagnostic review

Reviewed the owner's private `Reigncarnated-diagnostics _4_.zip`. Its current
source is `69469fe5983f2d06ea91faf25db64cf6539532b5`, matching APK28, not APK29.
Raw reports, process logs, traces and screenshots stay outside Git.

The current saved mission is `missions/multiplayer/2siege`, one AI, seed 1,
difficulty 1. It reaches 6,570 original simulation cycles (roughly 657 seconds),
reports 119.9 FPS and 5.3 ms native frame time before the UI failure. All 1,840
PCM mixer requests start; there are no callback errors or output-off failures.
The cumulative `Invalid RIFF/WAVE bounds` error is consistent with the speech
adapter fixed in APK29, but this archive cannot verify that later fix.
The owner reports that Instant Action appears completable; the attached mission
report contains no victory/result receipt, so completion remains owner-reported.

The retained fatal error is `Var is not an integer`, after opening Video in the
original desktop options dialog. Reproduction on the real host mission first
logs `Var [vid.dialog.detail.low] not found`, then throws from
`IFaceVar::GetIntegerValue` -> `ICButton::Notify` -> `IControl::ActivateVar` ->
`ICButton::Activate`. This is missing desktop graphics configuration, not a lost
campaign variable and not a renderer draw failure. Desktop mode/driver settings
cannot configure Android's EGL surface. APK30 routes `Sys::Options` activations
to the existing Android audio/display/touch dialog before native activation.
The same original `iface.activate Sys::Options` command is used by the release
main-menu Configuration button and in-game minimap/keyboard entry points.
The native Gameplay/Video/Sound/SaveLoad tab dialog is replaced by Android settings;
Android Back returns to the main menu. Saving/restoring games remains unimplemented
and is tracked as a release blocker, rather than exposed as a working tab.

Textured draw rejections are still present: 192 + 11 + 12 + 23 recorded draws
have referenced NaN U coordinates, with `textureActive=1`. These are not unused
texture coordinates. Keep strict validation, investigate the producer and verify
its visible effect; do not turn off checks to hide it. The shell collection
contains synchronized-mode samples with very slow uploads; its final synchronized
snapshot is diagnostic overhead and not representative of the saved 120 FPS
mission window. Do not combine different screens, stream modes or timestamps.

The older JDA failure nested in the ZIP identifies APK27, source `99efd4b...`;
it is not a new failure on APK28. The previous Sprawler S01 report is APK28,
reaches cycle 2,570 and records additional cinematic actions after the opening.
Neither report establishes end-to-end campaign completion.

Starting the authored host capture with one real AI exposed an independent
`new Mod*[...]` / scalar `delete` mismatch in `Mods::GetRandomMod`. APK30 stages
`delete[]` for that exact owned array. The random choice/order is unchanged;
ASan allocation checks remain enabled. A real AI gameplay capture and options
lifecycle checks validate this path after the correction.

The same AI capture reached normal shutdown after 200 additional cycles, where
ASan exposed deletion of a private polymorphic `Strategic::Location::Base` through
a non-virtual destructor. The private Base/Offset implementations now have virtual
destructors. Both already had a vptr and are defined only in that translation
unit; Location's public pointer-only layout and the engine build flags remain
unchanged. This is exercised by clean AI mission shutdown with allocation/type
mismatch checks still enabled.

A subsequent cleanup reproduction identified Placement::Manager::ClusterSet's
ClusterInfo array (`new ClusterInfo[...]`, scalar `delete info`). The staging
patch uses `delete[]`. The final real one-AI run saves 300 frames over 200 cycles
and completes original shutdown with zero open files and no sanitizer failure.
The reference capture is a bounded lifecycle check, not late-game acceptance.

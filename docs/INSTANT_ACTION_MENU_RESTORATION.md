# Original Instant Action flow and controls

The owner reports that v17 removes pillar artifacts and starts the original AI.
The next requested milestone is full Instant Action selection and its original
main-menu transitions. The existing Android dialog is an interim chooser.

## Current implementation and device feedback

Version 19 introduced a transition, but chose Main::Exit on entry and Main::Enter
on Cancel. Version 20 fixed startup and version 21 fixed backup/re-import paths;
neither restored the original setup controls. The owner now confirms animation
plays but Instant Action menus still do not display, and the animation should
move to a different city location. This is an open implementation gap, not a
completed menu milestone.

`State::beginInstant()` in `android/menu/native_menu.cpp` explicitly selects
Main::Exit. After the path finishes it switches to Choose and emits
@instant-ready. The native renderer draws MenuFrontend only in Main, so Choose
does not display an original setup panel. MenuActivity's only replacement is the
Android map/AI/difficulty/seed dialog. No GameSetup::Instant activation is wired.
The reported absence of that interim dialog additionally requires checking the
ready callback, catalog result/error and Activity lifecycle on the device; the
animation alone does not prove the catalog or original controls were activated.

## Confirmed original Instant Action sequence

Read the interface/objective scopes from the decoded GitHub microDR2 shell pack,
without executing its Windows binary or copying owned configuration into Git.
Its bookmark names agree with all 18 bookmark names previously recorded from
the owner's full installation. The reduced pack is reference evidence; compare
with the newly supplied full shell before packaging a change.

- InstantAction verification sends Instant::Enter to Shell::EventHandler.
- Instant::Enter sets gamegod.flow.state to Instant, hides the previous control,
  selects GameSetup::Instant as the pending control, starts Cineractive::Multi::Enter
  and sends GameSetup::Message::Created to the setup control.
- The objective selects bookmark Multi::Enter at time zero. At 2.0 seconds it
  sends CineractiveCompleted, whose handler activates the pending setup control.
  Activation is an objective event; it does not wait for the whole 5-second
  bookmark to end. The camera continues moving behind the setup controls.
- Cancel/Back sends Instant::Exit, clears the flow state, selects Shell::Main and
  starts Cineractive::Multi::Exit. The reference objective activates Main after
  2.0 seconds while that 4-second camera path can continue.
- Main::Exit belongs to Quit/Logout. It is the wrong entry path for setup.
- Returning from a mission is a separate flow: action Return/state Instant uses
  Main::Enter; action Setup/state Instant uses Multi::Return and recreates setup.

The correction must restore this event/control flow and the real setup controls,
with explicit support for their commands. Replacing one bookmark name alone
would leave the original menus missing. Existing shell-camera completion tests
cover generic path playback; they do not validate objective-driven activation
or the complete Instant Action screen. Preserve the original 10 Hz simulation,
ABI configuration, fenced geometry stream and deferred controller scope.

## Remaining original setup bindings

The source setup script is `interface_setup_instant.cfg`; earlier real-data
inspection recorded that it selects a random mission and adds an AI. The
original mission selector and preview are in `game_missionselection.cpp`,
`game_preview.cpp` and `multiplayer_controls_mission.cpp`. The full setup must
retain mission-group filtering, map selection/preview, valid starting positions,
and Back/Cancel transitions. Do not silently keep a start position that becomes
invalid after changing maps.

The original `multiplayer_private.h` Options struct provides placement,
difficulty policy, random seed, ruleset, starting credits, unit limit and normal
or regenerating resources. Team setup in `multiplayer.cpp` additionally binds
side, color, start position, alliance group, AI/personality and per-team
difficulty. Current v17 supports map, zero/one AI, difficulty and seed only.
Exposing the remaining controls requires their actual engine application and
validation; visible controls must not save an ignored value. Multiple AI slots
must respect the selected map's playable-team count. Fixed-side maps must retain
their original restrictions. Original mission rules remain the default.

## Attack sound investigation

The native digital driver reports Claimed3D=false and rejects Play3D. This alone
does not explain the reported attack sounds: the 3D sound-FX processing block in
pinned `coregame/soundfx.cpp` is commented out. The active Object::Process uses
Play2D/Update2D with the original distance, panning, visibility and volume logic.
An initial attribution to the disabled 3D path was corrected after following the
active implementation. Do not enable legacy 3D handling as an unverified fix.

Historical real-data host evidence decoded all 519 owned PCM effects; see
`docs/logs/instant-action/native-phase8-tests.log`. This does not establish that
every current imported asset is present or that weapon FX trigger on Android.
The remaining investigation should distinguish missing records, decode errors,
zero/low spatial volume, repeat suppression, owner replacement, voice exhaustion
and missing FX requests. The current driver retains audioError internally but
does not include a named per-resource failure summary in the mission collector.
Bounded audio request/failure counters and resource names would let a fresh
collection distinguish these cases without copying owned sound data.

One source discrepancy to test independently: the original Output::Request
steals only a strictly lower-priority voice, whereas AudioMixer::Play currently
also replaces an equal-priority voice. Do not claim it caused the device report
without evidence. Preserve voice ownership, looping and the original 10 Hz
simulation while checking audio changes.

## Control choices

The owner subsequently chose a minimal immediate scope: two-finger map rotation
without zoom, building rotation while selecting/placing, left click and quick
deselect. Controller changes are explicitly deferred. Version 18 implements that
scope; see `CONTROLS_BACKLOG.md` for usage and remaining device checks. The broader
questions below remain unselected, rather than authorizing a controller scheme
or StarCraft gameplay changes.

Questions presented:

- Touch: context-sensitive taps with visible commands, an explicit Select/Command
  mode, or long-press orders; all retain two-finger pan/pinch.
- Controller: camera on the left stick/cursor on the right, the reverse mapping,
  or unit/target snapping with a command wheel.
- Groups: visible recall/store/center buttons, a group wheel, or defer groups.
- First phone/controller to test and preferred StarCraft behaviors.

Recommended starting point: separate selection from explicit Attack-move and
Stop commands, support additive selection and same-type selection, keep Cancel
and deselect predictable, and provide quick group recall. Building placement
should show a preview and separate rotate/confirm/cancel actions. Queued orders
should have an explicit modifier. Bind these to original DR2 selection and order
APIs; do not assume StarCraft's unit behavior, selection limits or gameplay rules.
Phone controls need generous touch targets and should not require precise
double-taps for basic operations. Controller commands need visible focus/cursor
and remappable actions. Preferences are pending; no scheme is recorded as chosen.

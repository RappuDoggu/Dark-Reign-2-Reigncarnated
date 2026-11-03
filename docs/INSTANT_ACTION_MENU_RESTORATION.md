# Original Instant Action flow and controls

The owner reports that v17 removes pillar artifacts and starts the original AI.
The next requested milestone is full Instant Action selection and its original
main-menu transitions. The existing Android dialog is an interim chooser.

Version 19 connects both Instant Action entry points to imported Main::Exit
playback and opens the chooser only when it completes. Cancel/Back selects the
imported Main::Enter path and enables main controls after completion. This uses
the real bookmark curves and their durations. No Multi/InstantAction bookmark is
invented. The remaining work includes the full source setup controls, original
objective/cineractive scheduling and the other options listed below. Device
validation of v19's transition is pending. The earlier source notes below record
the pre-v19 gap.

## Original source boundaries

`src/menu_frontend.cpp` currently routes `gamegod.verify
Shell::Main.InstantAction` directly to `@instant`; Android immediately opens
`missionOptions(false, ...)`. `ShellScene` currently samples only `Main::Init`
and `Main::Enter`. The source shell's other camera paths and event completion
are not yet connected to selection navigation. Do not invent an Instant Action
bookmark: the recorded original shell inventory contains Multi::Enter,
Multi::Exit and Multi::Return, but no InstantAction::Enter. Resolve the actual
release interface/objective event sequence before assigning a camera path.

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

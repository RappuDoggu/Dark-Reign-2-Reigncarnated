# Offline 0.1 acceptance and multiplayer recovery

This is a release-readiness record, not a declaration that 0.1 is published.
The owner's Instant Action completion report is encouraging but has not yet
been repeated as a controlled victory/defeat/restart/return-to-menu test. APK28
telemetry does confirm an approximately eleven-minute, 120 FPS mission session.

## Offline release gates

| Gate | Evidence and remaining acceptance |
| --- | --- |
| Main menu and settings | APK30 routes the native Configuration, minimap Options and Escape entry points to the same Android settings as the toolbar. Check Apply/Cancel, music/effects, brightness, keep-awake, FPS display and pan speed from main menu and a running match, then after restart. Original desktop display modes are not implemented as Android settings. |
| Instant Action completion | Owner says matches appear completable. Repeat victory, defeat, surrender/back, replay and fresh match against AI; confirm results, no leaked audio, menus and input restoration. Check more than one map and both factions. |
| Complete campaigns | The owned-package receipt lists 31 campaign/intro/transition targets: 11 JDA, 20 Sprawler. Asset presence is not completion. Use `CAMPAIGN_ACCEPTANCE.csv` for every sequence/objective/dialogue, defeat/retry, next-mission route and final ending. Complete each faction continuously from its real first entry, not only by launching individual maps. Full retail assets are not available in this workspace. |
| Save, resume and campaign persistence | Native save currently reaches the explicitly unsupported offline `MultiPlayer::Save` boundary. Implement/version local saves, restore player/team/orders/tasks/AI/view/sight/clock state, test mid-mission restore, background/resume, process death, profile progress and campaign continuation after app restart. Do not label this working. |
| Dialogue and music | APK29 adds MP3 speech and soundtrack suspension. Opening Sprawler dialogue still needs an APK29-or-later device diagnostic; current supplied archive is APK28. Check JDA, mission objectives, unit responses, combat, movie audio, mute/focus/headphones and pause/resume. |
| Rendering | Active textured draws contain NaN UVs in the APK28 IA report. Identify/fix their producer with captures before accepting visual completeness; preserve strict draw validation. Check shroud, lighting, transitions and representative effects on both factions/worlds. |
| Stability and performance | AI personality/placement array deletion and private location polymorphic destruction are corrected in APK30 after ASan reproductions. Run long matches, a late-game battle, repeated menu/mission transitions, background/resume and thermal tests on multiple phones. Current phone 120 FPS evidence belongs to APK28; do not advertise it as an APK30 benchmark. |
| Distribution | Keep the same application ID/signer for updates, private key backup and private game-data import/export. Verify fresh install/update/data retention, exact source receipt, reproducible build instructions, third-party notices and no proprietary asset bundles in the APK. Choose and test the supported Android/device range. |

Campaign acceptance requires all 31 listed routes to be verified; a startup smoke
test or automatically forcing mission success cannot prove that objectives are
playable. Retain build/source, mission result, timing and diagnostic evidence for
each completed row. A campaign playthrough should exercise real objectives and
show that final progress/ending survives returning to the menu and restarting.

## Multiplayer: recoverable architecture, separate port

The upstream source includes ordered commands (`coregame_orders/orders.cpp`),
per-cycle sync CRCs (`game/sync.cpp`), synchronized session data
(`multiplayer/multiplayer_data.cpp`), and StyxNet client/server, session and
migration code. Orders record/test cycle and sync CRC. This is useful original
logic to preserve. It is not currently connected to another device:
`engine/platform/session_native.inc` explicitly reports offline, dispatches
messages locally, and returns no synchronized network packet.

The original StyxNet client/server rely on Windows socket events, threads,
mutexes and wait lists; the client uses TCP and the server also has UDP discovery.
Port those boundaries to Android/POSIX event-driven sockets and lifecycle handling.
Audit every serialized packet/order layout, fixed-width field, alignment and
bounds before expecting ARM64 compatibility. A determinism test must verify
initial state, seeded RNG and cycle checksums across peers; the existing 10 Hz
simulation must remain fixed. ARM64-to-PC crossplay is a separate compatibility
claim requiring x86/ARM floating-point and protocol validation.

Recommended proof sequence:

1. Replay the same order stream in two independent native instances and compare
   cycle CRCs over a long mission, including combat/building/pathfinding.
2. Restore real transport for two Android peers on one LAN. Start with direct
   address join; exchange version, asset/rules hashes, seed, faction/team and map.
   Confirm lobby ready/start, synchronized victory and a clean leave.
3. Exercise latency, jitter, packet boundaries, disconnection, app backgrounding,
   timeout and resynchronization behavior. Add LAN discovery after basic join.
4. Then assess internet hosting: NAT/relay, discovery/lobby replacement, reconnect,
   authentication/abuse boundaries and operational infrastructure. Original online
   services cannot simply be presumed available. Public internet support and
   original-PC crossplay have no acceptance evidence in this build.

Keep multiplayer outside the offline 0.1 gate unless the release scope explicitly
requires it. The first multiplayer deliverable should be a two-device LAN proof,
with retained checksums and reproducible steps, before exposing a working online
button. No network implementation or cloud service was published by this change.

## Trailer mockup

Landscape, approximately 30 seconds, requested by the owner. Use real gameplay
capture, clear work-in-progress labeling and a source receipt. Current capture is
from the original engine host GLES runner with the port's current source and a
privately owned reference map; it is not phone footage or an FPS demonstration.
Any soundtrack bed is authored placeholder audio. Do not claim complete campaigns,
multiplayer, shipping availability or device performance in the mockup. Replace
this footage with representative Android combat/building capture before a public
launch trailer. Raw map data and device diagnostics stay private.

## APK31 testing access

The main menu now separates **Start campaign** from **Choose individual mission /
cutscene (testing)**. All 31 declared entries can appear in the testing catalog
without progression locks; missing imported assets are marked and cannot launch.
Individual tests use separate progress and return to the menu on mission
transitions. They must not be counted as evidence that a continuous campaign is
completable. Keep using CAMPAIGN_ACCEPTANCE.csv for actual playthrough evidence.

Instant Action also offers a player-only starting-resource override for testing,
including zero or map default. AI stockpiles and campaign resources are unchanged.
Testing with edited starting resources is useful for investigation; final match
acceptance should also use the original map defaults.

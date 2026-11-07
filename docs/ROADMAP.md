# Reigncarnated roadmap

This lists priorities, not promised dates or completed features. 0.1 is an offline
early-access release. Track evidence by source/build and device.

| Priority | Work | Acceptance |
| --- | --- | --- |
| 1 | Campaign correctness and spoken dialogue | Play both factions continuously through every objective, intro, transition and ending; verify retry/defeat and return to menu. Use CAMPAIGN_ACCEPTANCE.csv; direct selection does not count as completion. |
| 1 | Rendering correctness | Trace active-texture NaN UV producers, verify fog/shroud/lighting and effects against the original, retain strict diagnostics and compare before/after captures. |
| 1 | Save, resume and campaign persistence | Implement versioned local save/restore; verify orders, tasks, AI, player/team, camera, sight and clock state, restart/background/process death, and campaign progress. |
| 2 | Instant Action completion/stability | Repeat victory, defeat, replay and new matches across maps/factions; test late-game battles, long sessions, audio/focus and thermal behavior on multiple devices. |
| 2 | Touch and accessibility | Refine gesture discoverability, HUD sizing, placement confirmation and configurable tolerances, preserving tested phone controls. |
| 2 | Release automation | Exercise clean reproducible builds, APK/source/relink checks, signed update retention and current CI. Keep device acceptance separate from host tests. |
| Later | Optional controller, keyboard and mouse polish | Define mappings with user feedback, then test without regressing touch. Controller changes remain deferred. |
| Later | Multiplayer recovery | First compare deterministic order streams/cycle checksums in independent instances, then two Android peers on a LAN. Test ready/start, latency, timeout, leave/reconnect and lifecycle behavior before internet services. |
| Research | Internet play and original PC crossplay | Audit wire ABI, x86/ARM arithmetic, authentication/lobby/relay needs and operational support. No current compatibility claim. |

StarDroid is a useful example of release/download/source packaging, separate game
imports, a player guide and explicit limits. Its Bluetooth/Wi-Fi Direct,
controller, replay, save and EUD features are not Reigncarnated features merely
because they appear in that repository. See the [repository review](RELEASE_AUDIT_0_1.md).

# Menu audio and device checkpoint — 2026-10-26

User reports original Login rendering, keyboard name entry and Login button input on Galaxy Fold 5. No transition or audio in v0.1, matching its unimplemented services. No screenshot available.

This increment connects the original interface effect callbacks to a bounded WAV cache/event queue and Android SoundPool. PCM RIFF chunks, lengths, channels, sample rates, alignment and duration are checked. Original effect volume is multiplied by the saved digital volume. Android preloads effects, handles asynchronous load completion, requests audio focus, pauses on focus loss, and releases playback on Activity pause. This provides menu effects, not music, spatial audio or CD playback. Android runtime playback is not yet verified.

Local validation: menu_audio_smoke passes 22 checks; complete sanitizer host suite passes 56/56 with ASAN_OPTIONS=detect_leaks=0. Android compilation/packaging and on-device sound remain pending. The previous device-tested APK is unchanged.

Next main-menu milestone: original shell controls with an explicitly temporary backdrop and unsupported actions visibly disabled. This is a scoped frontend milestone, not the original animated mission. No shell UI implementation is committed in this increment. Inspection confirms missions/system/shell.zwp contains the actual release interface_shell.cfg; base.zwp's interface_shell_mainmenu.cfg is an older layout and must not be presented as the release menu. The release menu uses Shell::EventHandler and cinematic completion events to activate controls. Full original world/cinematics remain unfinished.

No new executable byte-address issue was found. Inputs recovered from the online archive; no desktop files required.

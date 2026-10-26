# Main menu and audio v0.2 — 2026-10-26

The device-tested v0.1 stopped at Login because `sys.runcode shell` was unbound.
It was not a username/network authentication failure. The original shell run
code enters a mission and cinematic sequence, which the native engine cannot
run yet. This increment provides an explicitly scoped static frontend handoff.

## Implemented

- Bind the original Login notification to a deferred shell transition after
  input dispatch returns. Missing shell data reports an error and leaves Login
  usable; importing the archive and retrying is supported.
- Parse the owner's release `interface_shell.cfg` in `missions/system/shell.zwp`.
  Instantiate its original `Shell::Main` controls and relevant theme definitions,
  with the existing Login wallpaper as a temporary static backdrop. Other shell
  mission/network/cinematic scopes are deliberately not dispatched.
- Route Options and Logout through a native shell event receiver installed
  before controls resolve their event targets. Logout returns to the original
  profile selection. Quit closes the Activity. Repeated login is supported.
- Disable Campaigns, Load Mission, Instant Action, Multiplayer and Credits.
  This is a main-menu milestone, not playable gameplay or animated shell.
- Android imports `base.zwp` and `shell.zwp` separately through the file picker.
  Shell assets overlay the base archive. Game assets are not embedded in the APK.
- Original interface WAV effects use the existing validated cache and SoundPool
  bridge. Options changes the original profile's saved effects volume.
- A separate Music picker accepts an owned audio file (tested packaging with
  the installation's Track02.ogg as a companion). MediaPlayer loops it, handles
  async prepare/errors, obeys focus, and is released on pause. Music volume is
  saved separately. Playback restarts from the beginning on resume.
  This is host music playback, not restored CD/Redbook track scheduling or 3D audio.

## Validation

- 57/57 sanitizer host tests pass with ASAN_OPTIONS=detect_leaks=0, including GLES.
- Persistent-menu test: 26 checks, including missing shell recovery, disabled
  action rejection, Options dispatch, logout/relogin and saved effects volume.
- Real archives: original pointer/IME-style profile creation, Login button to
  shell, Options, Logout and relogin pass across two sessions. Main menu submits
  24 geometry and 9 font batches; original WAV cache/events validate.
- Software GLES: identical real-menu pixels across two frames in each of two
  newly created contexts; 292,954 nonblack pixels. Render visually inspected.
- ARM64 APK compilation, signature, alignment, ZIP and metadata checks are
  recorded in the package log. Android device input/audio remain unverified.
- Three local host test executables lost execute permission; restoring +x
  resolved the initial BAD_COMMAND failures. No source failure was involved.

See `logs/main-menu-*`. No new executable byte-address issue was encountered.

## Device test

Install Reigncarnated Menu v0.2. It uses package `org.reigncarnated.menu.v02`
and can coexist with v0.1: the previous signing key was not available in this
workspace, so this build must not claim in-place update compatibility.
Choose your existing base.zwp, then the supplied shell.zwp. Create a profile,
close the keyboard, and press Login. Check the main menu, Options volume,
Logout/relogin and Quit. Select Track02.ogg with Music for background playback.
Use Share report if startup, rendering or audio fails. Profiles are app-private
and not shared with v0.1. Full mission loading is the next engine milestone.

## Private test downloads

- [APK](https://chatgpt.com/library?file_id=libfile_839a1c315e308191a93ec09eff93856a&account_id=personal)
- [shell.zwp](https://chatgpt.com/library?file_id=libfile_ea5eca05fb3081919b95c94a140be086&account_id=personal)
- [Track02.ogg](https://chatgpt.com/library?file_id=libfile_b355bb66c69c8191877f8d802c4ae084&account_id=personal)

The owner's existing base.zwp is still required. Companion asset links are
private recovery copies, not public distribution. APK hash and companion hashes
are in `logs/main-menu-files.json`.

# Original Login and private profile checkpoint

Verified 2026-10-26 against pinned upstream
`9ce9da5f21604310a997f0c41e9cd383f5e292c3`.

The stager compiles complete original User, Stats and Game::Login source outside
this repository. Native ProfileStorage binds private writable files, existing
original DOCB serialization, campaign progress and explicit disabled audio settings.
No game assets or original source are committed. Existing installation profiles
are never used by the probe: it creates and removes its own temporary directory.

## Verified behavior

- 53/53 host tests pass with ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`).
- Profile test: 18 checks covering original binary save/reopen, automatic login,
  count and floating-point statistics, campaign completion, user switching,
  deletion, invalid deletion paths, stat type mismatch and unavailable addons.
- Actual `runcode_main_login.cfg` completes: 120 interface statements, 58 registered
  classes, 9 fonts, 8 bitmaps, 46 cursors, 128 key names and 12 modifiers.
- Actual Login and backdrop activate; original draw produces 72 geometry batches
  and 4 font batches. These are CPU submissions, not a rendered screenshot.
- `--exercise-login` dispatches original Create/Login notifications. A temporary
  pilot is created and reopened by the original code. The configured LoggedIn
  event reaches `sys.runcode shell`, which remains unbound. This run reports
  BLOCKED, with 73 geometry and 5 font batches.

Original IFace activation/deactivation/toggle command cases are staged from the
hash-checked source. System button and slider-thumb mappings match inspected
`interface/iface_controls.cpp`.

`register_standalone_launch_commands` explicitly supports only a launch without
host/join options. In upstream `multiplayer_cmd.cpp`'s `multiplayer.login` case,
all user/network effects are conditional on those options, followed by
`CmdLine::Reset`. The native adapter accepts neither option, leaves the user
unchanged and exposes `isLobby=0`; it rejects a lobby flag or command arguments.
It does not claim multiplayer initialization or connectivity.

## Ownership and limitations

Campaigns must outlive ProfileStorage. Call `close()` explicitly to save. The
destructor discards unflushed records after exceptions. Writes use a sibling
binary temporary file and atomic rename per file; info.cfg and data.cfg are not
a single transaction. Original Create rollback after IO failure needs further
work before production profile editing. Addon initialization/load/setup require
explicit callbacks for any installed addon; no silent fake success is provided.
Audio remains disabled, not implemented.

The shell configuration selects the original shell mission and enters the
mission run code. It is not merely another static dialog. Full run-code lifecycle,
shell mission loading/rendering, reusable Android UI host, touch/IME input and
Android surface lifecycle remain. No main-menu APK or gameplay is demonstrated.

## Reproduce

```sh
cmake -S . -B build -DREIGN_UPSTREAM=/path/to/separate/upstream -DREIGN_SANITIZE=ON
cmake --build build --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build --output-on-failure
ASAN_OPTIONS=detect_leaks=0 build/profile_storage_smoke
ASAN_OPTIONS=detect_leaks=0 build/menu_configuration_probe /owned/packs/base.zwp
ASAN_OPTIONS=detect_leaks=0 build/menu_configuration_probe /owned/packs/base.zwp runcode_main_login.cfg --exercise-login
```

The last command intentionally exits 2 at the unimplemented shell transition.
See logs in `docs/logs/profile-*`.

One local build failed because mission_catalog.cpp.o was zero bytes; removing
that generated object and rebuilding resolved linkage. Three otherwise intact
ELF tests had lost execute permission; restoring it yielded 53/53. Root cause of
workspace artifact damage is unknown. Initial failures are retained. An initial
system-button header include preceded portable guards and selected the legacy
bitmap header (`bink.h` unavailable); placing it after the portable control
headers fixed the compile. A probe variable lookup required original `$newUser`
syntax. No engine algorithm was changed for that probe correction.

Campaign predecessor CI 40316306212 passed host job 121549643702 and Android job
121549643511. CI verified commit `487e3b9a974f2742b20a4b95a41d9479ac4ca207` in
[run 40319219515](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40319219515):
host 53/53, Android ARM64 compilation (including User/Stats/Login), and software-GLES
presenter checks all pass. [Provider excerpts](logs/profile-ci-verification.log).
This does not validate original menu runtime on Android.



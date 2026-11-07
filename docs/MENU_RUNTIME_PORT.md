# Persistent original Login runtime — 2026-10-26

This increment keeps the original interface, configuration, console, variables,
profiles and input routing alive across frames. It is not the full main-menu shell
or gameplay. Upstream source remains external and hash-pinned.

## Implementation

- `MenuFrontend` owns one original frontend session, with ordered teardown and
  explicit configuration, font, geometry and sound providers.
- `ConsoleSession` preserves original callbacks and scrollback across calls.
- Original hover/click routing and `$` control-relative variable substitution
  are staged from reviewed `interface/iface.cpp`. Empty global shortcut tables
  do not consume keys; fullscreen/screenshots and gameplay shortcuts are not implemented.
- Printable ASCII and original key events feed the original editor. Unicode
  names are rejected explicitly because the original editor consumes byte characters.
- `FrontendPresenter` accepts live batches and preserves ordering. Presentation
  letterboxes the original 640x480 logical UI into the actual Android surface.
- Private Android package `org.reigncarnated.menu` loads the original Login from
  a user-selected base.zwp copied to app storage. Profiles use separate app storage.
  No proprietary assets are bundled. Audio is explicitly disabled.
- Touch cancellation releases outside controls to avoid accidental clicks.
  GPU recreation and profile saving are wired to the Activity lifecycle; device
  validation remains required. Rendering is on demand, not continuous animation.

## Failures and resolutions

The first real Create click failed resolving `$<.newUser`: the original interface
substitution registration had been omitted. Restored registration/unregistration.
The next sanitizer run found `strncpy-param-overlap` in `Utils::Strmcpy`, reached
from `IControl::ExecItem` -> `StrCrc<64>::Set`. The original substitution can return
its input buffer. The adapter now measures before writing and uses `memmove`,
retaining bounded NUL termination and padding. Tests cover self-copy, overlap in
both directions and truncation. This is a source-level issue, not a new guessed
binary patch address.

A local build also produced an empty object file and several executables without
execute permission. These are workspace artifact failures. Rebuild damaged outputs
and validate independently in CI; do not bypass tests or change engine behavior.
The first isolated string regression test included a private legacy header and
failed finding `strbuf.h`; the own string adapter was separated into its own header.

## Reproduce

Follow RECOVERY_ARCHIVES.md to restore inputs outside this repository.

```sh
cmake -S . -B build -DREIGN_UPSTREAM=/path/to/darkreign2-9ce9da5 -DREIGN_SANITIZE=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
build/menu_runtime_probe /path/to/packs/base.zwp
# For GPU probe add REIGN_GLES_BACKEND=ON and REIGN_KHRONOS_HEADERS
LIBGL_ALWAYS_SOFTWARE=1 build/menu_gpu_probe /path/to/packs/base.zwp /tmp/login.ppm
# With pinned Android SDK/NDK and REIGN_UPSTREAM set:
bash tools/package_menu.sh
```

Local procfs restrictions require `ASAN_OPTIONS=detect_leaks=0`; address/undefined
behavior checks remain enabled. CI supplies independent clean compiler evidence.

## Main-menu boundary and decompilation references

Original `game/gameruncodes.cpp` shows Login and Shell using Base resource
initialization, input/paint and teardown. Shell additionally checks a logged-in user.
The real shell configuration selects the shell mission and requests the mission
run code. A menu-shaped replacement would not implement that original path.
Remaining work includes `sys.runcode` lifecycle dispatch and original mission load,
world/material/mesh/terrain dependencies. Login actions reaching that unbound
transition currently return an explicit error.

Previously saved source-correlated x86 addresses: `0x004C3930` startup run-code
registration; `0x00545620` Login Init; `0x005456D0` Shell Init; `0x00558D20` video
resource initialization; `0x005E7610` interface process; `0x005E7160` paint.
No new decompilation was run in this increment and no new byte-address mismatch
was found. These addresses refer only to the recorded executable hash, not ARM64.
REA remains unavailable; original source compilation does not depend on REA.

## Validation

Current results and CI receipts are recorded in BUILD_STATUS.md and docs/logs.
Do not interpret an APK build as Android runtime or main-menu validation.

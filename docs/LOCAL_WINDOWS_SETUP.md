# Reigncarnated: local Windows setup

This guide prepares the existing project for local editing, builds and phone
testing. It does not turn the current source into a playable Instant Action APK.
See [current mission status](INSTANT_ACTION_BRINGUP.md). Keep the working v0.5
installation on your phone while the mission engine is being integrated.

Use Windows 11 or a supported Windows 10 installation on an Intel/AMD PC.
The existing packaging scripts require x86_64 Linux, so use WSL2 Ubuntu for
builds. A desktop shortcut can open the folder in Explorer; the files themselves
should live in WSL's Linux filesystem for build performance. These instructions
have not been executed on your PC. Their build versions match the working
cloud toolchain; USB connection and WSL setup need local verification.

## 1. Install WSL2

Open **PowerShell as Administrator** and run:

```powershell
wsl --install -d Ubuntu-24.04
```

Restart if requested. Open **Ubuntu 24.04** from Start and create your Linux
username/password. Check the distribution name and version in PowerShell:

```powershell
wsl --list --verbose
```

It should show version **2**. If it says 1, convert that distribution with
`wsl --set-version Ubuntu-24.04 2` (use its exact listed name).

Reference: [Microsoft WSL installation](https://learn.microsoft.com/en-us/windows/wsl/install).

## 2. Install build prerequisites inside Ubuntu

Paste into the **Ubuntu terminal**, not PowerShell:

```bash
sudo apt update
sudo apt install -y git curl ca-certificates unzip zip build-essential \
  cmake ninja-build python3 openjdk-17-jdk pkg-config \
  libegl1-mesa-dev libgles2-mesa-dev libarchive13t64 gh
mkdir -p "$HOME/Reigncarnated"
cd "$HOME/Reigncarnated"
```

This workspace will contain `repo`, `upstream`, `sdk`, `ffmpeg`, `owned`,
`build-host`, `build-menu`, `exports`, `logs` and `signing`. Only `repo` is the
Reigncarnated Git checkout. Keep game data, SDK files and signing keys outside it.
The setup also creates `build-engine-host` and `build-engine-android` for the
separate original-engine ABI foundation; neither is currently a playable game.

## 3. Sign in to GitHub and clone the working branch

Still in Ubuntu:

```bash
gh auth login
```

Choose **GitHub.com → HTTPS → browser login** and sign in to the account that
can access `RappuDoggu/Dark-Reign-2-Reigncarnated`. Then:

```bash
gh auth setup-git
git clone --branch dev/native-arm64-primitives \
  https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated.git repo
cd repo
git status
```

If the folder already exists, inspect its changes before updating it. Do not
clone over it or reset/discard local work.

## 4. Install the pinned toolchain and run host checks

```bash
bash tools/setup_local.sh "$HOME/Reigncarnated"
source "$HOME/Reigncarnated/env.sh"
```

The script downloads the pinned original source separately, Android NDK
27.2.12479018, Build Tools 35.0.0, Android platform 35 and FFmpeg 6.1.1; builds
the host/Android decoder; configures the host build and runs its tests. Allow
several GB of free space for downloads, extracted SDKs and build output.

Read the applicable Android SDK terms before using the downloaded tools:
[Android SDK download and terms](https://developer.android.com/studio).
The setup script checks pinned SDK archive hashes. It does not install device
drivers, connect to your phone or create a new signed APK.

If interrupted, rerun the script. It reuses downloads and existing builds. If a
decoder install is incomplete, rerun its build explicitly:

```bash
source "$HOME/Reigncarnated/env.sh"
bash "$REIGN_REPO/tools/build_bink_decoder.sh" "$REIGN_WORK_ROOT/ffmpeg" host
bash "$REIGN_REPO/tools/build_bink_decoder.sh" "$REIGN_WORK_ROOT/ffmpeg" android
```

## 5. Open the folder from your desktop

In Ubuntu:

```bash
cd "$HOME/Reigncarnated"
explorer.exe .
```

Explorer opens the Linux folder. Copy its address (typically
`\\wsl.localhost\Ubuntu-24.04\home\YOUR_LINUX_USER\Reigncarnated`). On your Windows
desktop choose **New → Shortcut** and paste that address. Name it **Reigncarnated**.
This is a shortcut to the working directory, not a second copy of the project.

## 6. Give Codex access to the project

The most direct route for the current build scripts is **Codex CLI in Ubuntu**:

```bash
curl -fsSL https://chatgpt.com/codex/install.sh | sh
```

Reopen Ubuntu if necessary so the installed command is on PATH, then:

```bash
source "$HOME/Reigncarnated/env.sh"
cd "$HOME/Reigncarnated"
codex
```

Sign in with your ChatGPT account. Opening the workspace root gives the agent
access to the sibling source, data and build directories, as allowed by your
chosen permissions. Git operations should use `repo` as their working directory.

For a desktop editor, install [VS Code](https://code.visualstudio.com/) and its
WSL extension, then run `code .` from this Ubuntu workspace. Use the Codex IDE
extension configured to run in WSL, or run `codex` in the integrated terminal.

You can also open the workspace in the ChatGPT desktop app via **Add project /
Open folder**, entering `\\wsl$\` in the picker. Its default Windows-native agent
uses PowerShell; tell it to invoke the build inside WSL explicitly. Changing the
terminal dropdown alone does not guarantee the agent runs in Linux. If Windows
Git integration cannot handle the WSL folder, use the WSL CLI/editor route above.

Official guidance: [Codex WSL](https://learn.chatgpt.com/docs/windows/wsl),
[Windows app](https://learn.chatgpt.com/docs/windows/windows-app),
[Codex CLI](https://learn.chatgpt.com/docs/codex/cli).

## 7. Copy your owned game data

Use Explorer to copy your complete, legally owned `Dark Reign 2` folder into
the workspace's `owned` folder. Expected example:
`~/Reigncarnated/owned/Dark Reign 2/packs/base.zwp`.

Copy the full installation, including `missions`, `worlds`, `sides`, `packs`,
music and movies. The small menu-data ZIP is insufficient for Instant Action.
The Windows executable is not needed by the Android source port.

Verify the current mission data path in Ubuntu:

```bash
source "$HOME/Reigncarnated/env.sh"
ASAN_OPTIONS=detect_leaks=0 "$REIGN_WORK_ROOT/build-host/instant_action_data_probe" \
  "$REIGN_WORK_ROOT/owned/Dark Reign 2"
```

Expected result for the repeatable `2crazed` fixture: **Instant Action data
preflight: PASS**, 100×100, 125 object definitions. It also explicitly reports
that mission boot/selection/construction are not implemented by this probe.

## 8. Preserve the APK signing identity

Download your existing [v0.2 build recovery ZIP](https://chatgpt.com/library?file_id=libfile_d4a951db49b08191b4a78404f284b4b3&account_id=personal)
and extract it into `signing`. Locate its `debug.keystore` and copy it to
`build-menu/debug.keystore` **before packaging**. Do not commit that key.

The package script otherwise generates a different development key. Android
will reject an update signed with a different key; do not uninstall the working
app just to bypass that failure, because it can remove imported data and saves.
After setup, the agent should compare signer certificate fingerprints before
attempting an in-place update.

The existing launch baseline can be rebuilt with:

```bash
source "$HOME/Reigncarnated/env.sh"
bash "$REIGN_REPO/tools/package_menu.sh"
```

This currently produces **Reigncarnated-Native-Launch-v05.apk**, not a playable
mission APK. A mission package will be named and documented separately when its
engine integration is ready.

## 9. Connect your Fold through Windows ADB

Download **SDK Platform-Tools for Windows** from
[Google's official page](https://developer.android.com/tools/releases/platform-tools).
Extract the `platform-tools` directory into `C:\Reigncarnated-Tools` so that
`C:\Reigncarnated-Tools\platform-tools\adb.exe` exists.

On the phone, enable Developer options and **USB debugging**. Connect a USB data
cable, unlock the phone and approve the computer's RSA authorization prompt.

In **PowerShell**:

```powershell
& 'C:\Reigncarnated-Tools\platform-tools\adb.exe' devices -l
```

Expect a serial followed by `device`. `unauthorized` means the phone authorization
is pending. If no device appears, try another data cable/USB port and the official
[Samsung USB driver](https://developer.samsung.com/android-usb-driver).

Keep USB/ADB on Windows initially; the build runs in WSL. This avoids needing USB
passthrough into WSL. From Ubuntu, the agent can invoke the same Windows executable:

```bash
/mnt/c/Reigncarnated-Tools/platform-tools/adb.exe devices -l
```

Official instructions: [Android Debug Bridge](https://developer.android.com/tools/adb).

## 10. Install, launch and collect logs when an APK is ready

Copy the ready APK from the WSL build folder into `C:\Reigncarnated-Tools\exports`
using Explorer; create that folder if needed. The example below uses the known
launch baseline. Replace the filename only when a new build has been provided.

In PowerShell:

```powershell
$dr2Adb = 'C:\Reigncarnated-Tools\platform-tools\adb.exe'
& $dr2Adb install -r 'C:\Reigncarnated-Tools\exports\Reigncarnated-Native-Launch-v05.apk'
& $dr2Adb shell am start -n org.reigncarnated.menu.v02/org.reigncarnated.menu.MenuActivity
& $dr2Adb logcat -v threadtime | Tee-Object -FilePath 'C:\Reigncarnated-Tools\exports\dr2-device.log'
```

Reproduce the problem, then press **Ctrl+C** to stop recording. If several
devices are attached, add `-s SERIAL` after `$dr2Adb` for each command. Device
logs can include other apps' messages; share only the relevant portion.

## 11. Start the local continuation

Paste this into the local Codex session:

> Continue Reigncarnated from `repo` on `dev/native-arm64-primitives`.
> Read `docs/INSTANT_ACTION_BRINGUP.md`, `docs/LAUNCH_PROGRESS.md` and
> `docs/LOCAL_WINDOWS_SETUP.md`. The workspace root is the parent of `repo`;
> `env.sh` sets toolchain paths. Owned game data is under `owned/Dark Reign 2`.
> Preserve the confirmed v0.5 baseline and signing key. Target an original
> Instant Action mission with no opponent, touch unit selection and original
> structure construction. Data preflight is not gameplay. Keep the original
> simulation and adapt platform boundaries. Use Windows adb.exe for the phone.
> Verify paths and build prerequisites, then continue implementation without
> replacing the original gameplay with a visual mock-up.

For daily work, open Ubuntu, `source ~/Reigncarnated/env.sh`, go to
`~/Reigncarnated`, and run `codex`. Existing chat history is not assumed to move
automatically between clients; the repository checkpoint is the handoff.

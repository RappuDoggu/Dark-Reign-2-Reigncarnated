# Android asset test v0.2

This test exercises native ARM64 archive access. It does not launch Dark Reign 2.

1. Extract the supplied installation archive on your PC.
2. Copy **`packs/base.zwp`** (about 95 MiB) into **Downloads** on your Fold 5 or Thor.
3. Install **Reigncarnated-Asset-Test-v02.apk** and open **Reigncarnated Asset Test**.
4. Tap **Choose base.zwp**, then select that file using Android's file picker. Do not choose the `.7z` installation archive.
5. Wait for the report. Tap **Share test report** and paste the text into this conversation. If installation or launch fails, send the exact error or a screenshot instead.

Expected for this supplied base pack:

```text
PASS — native ARM64 archive access
Archive entries: 2074
Payload bytes read: 99631656
Configuration files: 465
GOD meshes: 334
Textures: 567
```

Elapsed time varies by device and is not a benchmark. The report also records the device/Android API. Test once, close/reopen the app, and repeat selection to check that the file-picker flow remains usable. A Fold 5 success is sufficient for the next development step; Thor dual-screen operation is not being tested yet.

The app requests no internet or broad storage permission. The selected file is copied to app-private cache (512 MiB diagnostic cap), tested off the UI thread, and deleted afterward. If Android kills the process, its cache copy may remain until Android clears it or the app is uninstalled. No game files or code are bundled in this diagnostic. The APK requires ARM64 Android API26+ and is debug signed; it is a development test, not a store release.

Build with a full JDK17, CMake, NDK r27c and SDK35/build-tools35.0.0:

```bash
export ANDROID_SDK_ROOT=/path/to/sdk
export ANDROID_NDK_HOME="$ANDROID_SDK_ROOT/ndk/27.2.12479018"
bash tools/package_asset_probe.sh
```

This target explicitly builds with `REIGN_UPSTREAM` empty and links only `reign_platform`. The original-engine diagnostic target remains separately restricted and is not published. Do not confuse the asset-test package `org.reigncarnated.assetprobe` with full engine startup.

CI build and log artifacts: [verified run 40117369440](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40117369440). The direct delivered APK and CI APK use separate debug keys; choose one delivery for this test rather than installing one over the other. If switching between them, uninstall the existing Asset Test first. This does not affect the game installation file in Downloads.

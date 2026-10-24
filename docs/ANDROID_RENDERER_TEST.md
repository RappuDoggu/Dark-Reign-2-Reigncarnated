# Renderer Test v0.3 — next device checkpoint

This tests the new rendering boundary, not the original game's menu or a playable mission.

1. Install Reigncarnated-Renderer-Test-v03.apk on the Fold 5. The locally delivered build uses the same debug key/package as the previous local test and should update it.
2. Open Reigncarnated Renderer Test. The GLES pixel checks should all report PASS; the initial colored panel only shows that the renderer is alive.
3. Tap Choose base.zwp and select the same file already in Downloads. The original cursor atlas should replace the colored panel. Archive counts should still match the previous report.
4. Press Home, reopen the app, then select the pack once more. This checks graphics context recreation around activity/file-picker transitions.
5. Tap Share test report and paste the text here. Include a screenshot if the preview is blank, incorrectly colored or visibly corrupted.

The report includes Android/device and GL/GPU information. Timing is diagnostic, not a benchmark. No game data or original engine code is bundled; the selected pack is temporarily copied to private cache and deleted after reading. The decoded cursor image remains only in process memory for preview. API26+ / ARM64 / GLES3 required.

If switching from a CI-signed test build to the local delivery causes Android to report a signature conflict, uninstall only the earlier test app and retry. The base.zwp file in Downloads is unaffected.

Build evidence: [passing CI 40121657469](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40121657469). The implementation is on `dev/native-arm64-primitives` in PR #1. Package validation includes NDK runtime notices, ARM64 ELF/ZIP 16KiB alignment, APK signatures and ZIP entry CRCs. Actual Android GPU/rendering behavior is the remaining test for this build.

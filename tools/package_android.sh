#!/usr/bin/env bash
set -euo pipefail
: "${ANDROID_SDK_ROOT:?Set ANDROID_SDK_ROOT}"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME (r27c tested)}"
: "${REIGN_UPSTREAM:?Set REIGN_UPSTREAM to separate checkout}"
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
build=${REIGN_BUILD_DIR:-$repo/build-android}
tools=${REIGN_BUILD_TOOLS:-$ANDROID_SDK_ROOT/build-tools/35.0.0}
cmake -S "$repo" -B "$build" \
 -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
 -DREIGN_UPSTREAM="$REIGN_UPSTREAM" -DREIGN_ANDROID_SHELL=ON
cmake --build "$build" --parallel 2
mkdir -p "$build/apk/lib/arm64-v8a"
cp "$build/libreign_diagnostic.so" "$build/apk/lib/arm64-v8a/"
# The NDK defaults to a static C++ runtime in this single-library diagnostic.
"$tools/aapt2" link -o "$build/unsigned.apk" \
 -I "$ANDROID_SDK_ROOT/platforms/android-35/android.jar" \
 --manifest "$repo/android/AndroidManifest.xml"
python3 - "$build" <<'PY'
import pathlib, sys, zipfile
p = pathlib.Path(sys.argv[1])
with zipfile.ZipFile(p / 'unsigned.apk', 'a') as z:
 z.write(p / 'apk/lib/arm64-v8a/libreign_diagnostic.so', 'lib/arm64-v8a/libreign_diagnostic.so')
PY
"$tools/zipalign" -f -P 16 4 "$build/unsigned.apk" "$build/aligned.apk"
if [ ! -f "$build/debug.keystore" ]; then
 keytool -genkeypair -keystore "$build/debug.keystore" -storepass android \
  -keypass android -alias androiddebugkey -dname 'CN=Reigncarnated Development' \
  -keyalg RSA -keysize 2048 -validity 3650
fi
"$tools/apksigner" sign --ks "$build/debug.keystore" --ks-pass pass:android \
 --out "$build/reigncarnated-diagnostic.apk" "$build/aligned.apk"
"$tools/apksigner" verify --verbose "$build/reigncarnated-diagnostic.apk"
"$tools/zipalign" -c -P 16 4 "$build/reigncarnated-diagnostic.apk"
printf '%s\n' "LOCAL DIAGNOSTIC ONLY: $build/reigncarnated-diagnostic.apk" \
 'Contains original upstream primitives. Do not redistribute until rights are resolved.'

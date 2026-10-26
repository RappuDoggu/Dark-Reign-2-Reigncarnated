#!/usr/bin/env bash
# Private original-engine Login package; contains no proprietary assets.
set -euo pipefail
: "${ANDROID_SDK_ROOT:?Set ANDROID_SDK_ROOT}"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME}"
: "${REIGN_UPSTREAM:?Set REIGN_UPSTREAM to separate pinned source}"
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
build=${REIGN_MENU_BUILD_DIR:-$repo/build-menu}
tools="$ANDROID_SDK_ROOT/build-tools/35.0.0"
platform="$ANDROID_SDK_ROOT/platforms/android-35/android.jar"
cmake -S "$repo" -B "$build" \
 -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DREIGN_UPSTREAM="$REIGN_UPSTREAM" -DREIGN_ANDROID_MENU=ON
cmake --build "$build" --target reign_menu --parallel 2
mkdir -p "$build/classes" "$build/dex"
if command -v javac >/dev/null 2>&1; then
 compiler=(javac)
else
 compiler=(java com.sun.tools.javac.Main)
fi
"${compiler[@]}" --release 8 -classpath "$platform" -d "$build/classes" \
 "$repo"/android/menu/org/reigncarnated/menu/*.java
mapfile -d '' classes < <(find "$build/classes" -name '*.class' -print0)
"$tools/d8" --lib "$platform" --min-api 26 --output "$build/dex" "${classes[@]}"
"$tools/aapt2" link -o "$build/unsigned.apk" -I "$platform" \
 --manifest "$repo/android/menu/AndroidManifest.xml"
python3 - "$build" "$ANDROID_NDK_HOME" <<'PY'
import pathlib, sys, zipfile
p = pathlib.Path(sys.argv[1])
ndk = pathlib.Path(sys.argv[2])/'toolchains/llvm/prebuilt/linux-x86_64'
with zipfile.ZipFile(p/'unsigned.apk','a') as z:
 z.write(p/'libreign_menu.so','lib/arm64-v8a/libreign_menu.so',compress_type=zipfile.ZIP_STORED)
 z.write(p/'dex/classes.dex','classes.dex',compress_type=zipfile.ZIP_DEFLATED)
 z.write(ndk/'NOTICE','assets/notices/llvm.txt',compress_type=zipfile.ZIP_DEFLATED)
 z.write(ndk/'sysroot/NOTICE','assets/notices/android-runtime.txt',compress_type=zipfile.ZIP_DEFLATED)
PY
"$tools/zipalign" -f -P 16 4 "$build/unsigned.apk" "$build/aligned.apk"
if [ ! -f "$build/debug.keystore" ]; then
 keytool -genkeypair -keystore "$build/debug.keystore" -storepass android -keypass android \
 -alias androiddebugkey -dname 'CN=Reigncarnated Private Original Menu' -keyalg RSA -keysize 2048 -validity 3650
fi
"$tools/apksigner" sign --ks "$build/debug.keystore" --ks-pass pass:android \
 --out "$build/Reigncarnated-Main-Menu-v02.apk" "$build/aligned.apk"
"$tools/apksigner" verify --verbose "$build/Reigncarnated-Main-Menu-v02.apk"
"$tools/zipalign" -c -P 16 4 "$build/Reigncarnated-Main-Menu-v02.apk"
"$tools/aapt2" dump badging "$build/Reigncarnated-Main-Menu-v02.apk"

python3 - "$build/Reigncarnated-Main-Menu-v02.apk" <<'PYZIP'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as apk:
 assert apk.testzip() is None, 'APK ZIP entry checksum mismatch'
 assert 'classes.dex' in apk.namelist()
 assert 'lib/arm64-v8a/libreign_menu.so' in apk.namelist()
 print('APK ZIP structure and entry checksums: PASS')
PYZIP

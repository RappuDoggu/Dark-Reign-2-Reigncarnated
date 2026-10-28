#!/usr/bin/env bash
# External, pinned LGPL-only FFmpeg build. No source or generated libraries enter Git.
set -euo pipefail
root=${1:?Usage: build_bink_decoder.sh external-build-root host-or-android}
mode=${2:?Select host or android}
mkdir -p "$root"
root=$(cd "$root" && pwd)
archive="$root/ffmpeg-n6.1.1.tar.gz"
if [ ! -f "$archive" ]; then
 curl -fL https://codeload.github.com/FFmpeg/FFmpeg/tar.gz/refs/tags/n6.1.1 -o "$archive"
fi
python3 - "$archive" <<'PY'
import hashlib,sys
assert hashlib.sha256(open(sys.argv[1],'rb').read()).hexdigest()=='7c1ebea95d815e49c1e60c7ee816410dec73a81b8ac002b276780d2f9048e598','Unexpected FFmpeg source archive'
PY
mkdir -p "$root/source" "$root/$mode"
if [ ! -f "$root/source/configure" ]; then tar -xf "$archive" --strip-components=1 -C "$root/source"; fi
args=(--prefix="$root/$mode/install" --disable-everything --disable-programs --disable-doc --disable-network --disable-autodetect --enable-pic --enable-decoder=bink,binkaudio_dct,binkaudio_rdft --enable-demuxer=bink --enable-protocol=file --enable-swscale --enable-swresample)
case "$mode" in
 host) args+=(--disable-x86asm);;
 android)
  : "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME}"
  llvm="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
  args+=(--target-os=android --arch=aarch64 --enable-cross-compile --disable-asm --cc="$llvm/aarch64-linux-android26-clang" --ar="$llvm/llvm-ar" --ranlib="$llvm/llvm-ranlib" --strip="$llvm/llvm-strip");;
 *) exit 2;;
esac
cd "$root/$mode"
"$root/source/configure" "${args[@]}"
make -j "${REIGN_BUILD_JOBS:-4}"
make install
cp "$root/source/COPYING.LGPLv2.1" install/

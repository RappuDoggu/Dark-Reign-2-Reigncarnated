#!/usr/bin/env bash
# Minimal Linux SDK/NDK installation for this harness, independent of runner images.
set -euo pipefail
: "${1:?usage: bash tools/install_android_sdk.sh /absolute/sdk/root}"
mkdir -p "$1"
sdk_root=$(cd -- "$1" && pwd)
cache=${REIGN_DOWNLOAD_CACHE:-$sdk_root/downloads}
mkdir -p "$cache" "$sdk_root/platforms" "$sdk_root/build-tools" "$sdk_root/ndk"
fetch() {
 local name=$1 expected=$2
 if [ ! -f "$cache/$name" ]; then
  curl --fail --location --retry 3 --output "$cache/$name.part" "https://dl.google.com/android/repository/$name"
  mv "$cache/$name.part" "$cache/$name"
 fi
 printf '%s  %s\n' "$expected" "$cache/$name" | sha256sum --check --status
}
fetch android-ndk-r27c-linux.zip 59c2f6dc96743b5daf5d1626684640b20a6bd2b1d85b13156b90333741bad5cc
fetch build-tools_r35_linux.zip bd3a4966912eb8b30ed0d00b0cda6b6543b949d5ffe00bea54c04c81e1561d88
fetch platform-35_r02.zip 0988cacad01b38a18a47bac14a0695f246bc76c1b06c0eeb8eb0dc825ab0c8e0
if [ ! -d "$sdk_root/ndk/27.2.12479018" ]; then
 unzip -q "$cache/android-ndk-r27c-linux.zip" -d "$sdk_root/ndk"
 mv "$sdk_root/ndk/android-ndk-r27c" "$sdk_root/ndk/27.2.12479018"
fi
if [ ! -d "$sdk_root/build-tools/35.0.0" ]; then
 unzip -q "$cache/build-tools_r35_linux.zip" -d "$sdk_root/build-tools"
 mv "$sdk_root/build-tools/android-15" "$sdk_root/build-tools/35.0.0"
fi
if [ ! -d "$sdk_root/platforms/android-35" ]; then
 unzip -q "$cache/platform-35_r02.zip" -d "$sdk_root/platforms"
fi
printf 'Verified SDK root: %s\nNDK: %s/ndk/27.2.12479018\n' "$sdk_root" "$sdk_root"

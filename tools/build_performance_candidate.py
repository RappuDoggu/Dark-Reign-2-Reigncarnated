#!/usr/bin/env python3
"""Rebuild the optimized original engine, menu, and signed ARM64 test APK.
Inputs remain external: pinned upstream, NDK/SDK, minimal Android FFmpeg,
original private v05 APK, and optional existing candidate test signing key.
"""
import argparse,shutil,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ('upstream','sdk','ffmpeg','base-apk','output'):p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--ndk',type=Path);p.add_argument('--signing-key',type=Path);p.add_argument('--jobs',type=int,default=4)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
ndk=(a.ndk or a.sdk/'ndk/27.2.12479018').resolve();staged=out/'staged';original=out/'original';foundation=out/'foundation';menu=out/'menu';package=out/'package'
if repo==out or repo in out.parents:p.error('Use an external output directory')
def run(*cmd):subprocess.run([str(v) for v in cmd],check=True)
run(sys.executable,repo/'tools/prepare_engine_full.py',a.upstream.resolve(),staged)
run(sys.executable,repo/'tools/build_engine_gameplay.py',a.upstream.resolve(),staged,original,'--compiler',ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++','--source-list',repo/'cmake/performance-engine-sources.json','--jobs',a.jobs)
common=['-G','Ninja','-DCMAKE_TOOLCHAIN_FILE='+str(ndk/'build/cmake/android.toolchain.cmake'),'-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=26','-DANDROID_STL=c++_static','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -g -fno-strict-aliasing -fwrapv','-DCMAKE_POSITION_INDEPENDENT_CODE=ON','-DREIGN_UPSTREAM='+str(a.upstream.resolve()),'-DREIGN_FFMPEG_ROOT='+str(a.ffmpeg.resolve())]
run('cmake','-S',repo/'engine','-B',foundation,*common)
run('cmake','--build',foundation,'--parallel',a.jobs)
run('cmake','-S',repo,'-B',menu,*common,'-DREIGN_ANDROID_MENU=ON')
run('cmake','--build',menu,'--target','reign_menu','--parallel',a.jobs)
run(sys.executable,repo/'tools/link_mission_android.py','--staged',staged,'--foundation',foundation,'--ffmpeg',a.ffmpeg.resolve(),'--ndk',ndk,'--objects',original,'--output',package)
run(ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip','--strip-unneeded','-o',package/'libreign_menu.so',menu/'libreign_menu.so')
if a.signing_key:shutil.copyfile(a.signing_key,package/'mission-debug.keystore')
run(sys.executable,repo/'tools/package_mission_candidate.py','--base-apk',a.base_apk.resolve(),'--native-library',package/'libreign_mission.so','--menu-library',package/'libreign_menu.so','--sdk',a.sdk.resolve(),'--output',package)

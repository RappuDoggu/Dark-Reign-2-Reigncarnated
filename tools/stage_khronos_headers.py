#!/usr/bin/env python3
"""Stage SDK-owned standard Khronos headers externally for Linux GLES validation."""
from pathlib import Path
import argparse,shutil
p=argparse.ArgumentParser();p.add_argument('ndk',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==r or r in out.parents:raise ValueError('Keep SDK headers outside the repository')
sysroot=a.ndk/'toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include'
for name in ['GLES3','GLES2','KHR','EGL']:
 shutil.copytree(sysroot/name,out/name,dirs_exist_ok=True)
print(out)

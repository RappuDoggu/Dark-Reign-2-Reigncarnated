#!/usr/bin/env python3
"""Link the original mission engine and JNI entry point into an ARM64 library.
Prepare the pinned source with prepare_engine_full.py and compile original
units with build_engine_gameplay.py first. Foundation libraries come from the
separate engine/CMakeLists.txt build. No unresolved symbols are permitted.
"""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['staged','foundation','ffmpeg','ndk','output']:p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--object-map',type=Path);p.add_argument('--objects',type=Path,action='append',default=[])
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
ndk=a.ndk.resolve()/'toolchains/llvm/prebuilt/linux-x86_64/bin';cc=ndk/'aarch64-linux-android26-clang++';ar=ndk/'llvm-ar';mapping={}
if a.object_map:mapping={k:Path(v) for k,v in json.loads(a.object_map.read_text()).items()}
for directory in a.objects:
 for unit in json.loads((directory/'results.json').read_text())['units']:
  if unit['exit_code']==0:mapping[unit['source']]=directory/(unit['source'].replace('/','__')+'.o')
if not mapping:p.error('Provide compiled original objects')
for source,obj in mapping.items():
 with obj.open('rb') as file:header=file.read(20)
 if header[:4]!=b'\x7fELF' or header[18:20]!=b'\xb7\x00':raise ValueError('Not an ARM64 ELF object: '+source)
include=[repo/'engine/platform',repo/'engine/platform/dx7',repo/'include']+sorted({x.parent for x in a.staged.rglob('*.cpp')})+[a.staged/'3rdparty',a.staged/'3rdparty/dx7',a.staged/'3rdparty/won']
flags=['-O2','-fno-strict-aliasing','-fwrapv','-std=c++17','-DDEVELOPMENT=1','-DNO_ASM_MATH','-DMONO_DISABLED','-ffunction-sections','-fdata-sections','-fPIC','-fvisibility=hidden']
for directory in include:flags+=['-I',str(directory)]
flags+=['-include',str(a.staged/'system/std.h')]
source_files=[repo/'engine/platform/mission_app.cpp',repo/'android/mission/native_mission.cpp',repo/'engine/platform/device_gles.cpp',repo/'src/legacy_gles.cpp',repo/'engine/platform/debug_native.cpp',a.staged/'graphics/vid.cpp',a.staged/'graphics/vid_cmd_dialog.cpp',a.staged/'util/perfstats.cpp']
objects=[];compiled=[]
for number,source in enumerate(source_files):
 obj=out/(str(number)+'-'+source.name+'.o');cmd=[str(cc),*flags,'-c',str(source),'-o',str(obj)]
 result=subprocess.run(cmd,capture_output=True,text=True);(out/(obj.name+'.log')).write_text(result.stdout+result.stderr)
 if result.returncode:raise RuntimeError('Compilation failed: '+str(source)+'; see '+str(out/(obj.name+'.log')))
 objects.append(str(obj));compiled.append({'source':str(source),'command':cmd})
archive=out/'original_engine.a';archive.unlink(missing_ok=True);subprocess.run([str(ar),'rcs',str(archive),*[str(mapping[name]) for name in sorted(mapping)]],check=True)
exports=out/'exports.map';exports.write_text('{ global: Java_org_reigncarnated_menu_MissionActivity_*; local: *; };\n')
library=out/'libreign_mission.unstripped.so'
cmd=[str(cc),'-shared','-static-libstdc++',*objects,'-Wl,--gc-sections','-Wl,--no-undefined','-Wl,--error-limit=0','-Wl,-Bsymbolic','-Wl,-z,max-page-size=16384','-Wl,--version-script='+str(exports),'-Wl,--start-group',str(archive),*[str(x) for x in sorted(a.foundation.glob('libdr2_engine_*.a'))],*[str(a.ffmpeg/'lib'/('lib'+name+'.a')) for name in ['avformat','avcodec','swscale','avutil']],'-Wl,--end-group','-llog','-lEGL','-laaudio','-ldl','-lm','-o',str(library)]
result=subprocess.run(cmd,capture_output=True,text=True);(out/'native-link.log').write_text(result.stdout+result.stderr)
report={'linked':result.returncode==0,'compiled':compiled,'link_command':cmd,'original_objects':{name:{'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()} for name,path in mapping.items()},'android_executed':False}
(out/'native-build.json').write_text(json.dumps(report,indent=2))
if result.returncode:raise RuntimeError('Native link failed; see '+str(out/'native-link.log'))
subprocess.run([str(ndk/'llvm-strip'),'--strip-unneeded','-o',str(out/'libreign_mission.so'),str(library)],check=True)
print(out/'libreign_mission.so')

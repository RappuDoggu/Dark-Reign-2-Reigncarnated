#!/usr/bin/env python3
"""Link and run display or local-session boundary tests against original host objects.

The default display test uses a fixture; engine_vid_gles_test uses real GLES.
None of these boundary tests is a mission runner.
Compile graphics/, util/mathtypes.cpp and the adapted vid.cpp with the host
compiler first. Later --objects directories override earlier source objects.
"""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('foundation',type=Path);p.add_argument('output',type=Path)
p.add_argument('--objects',type=Path,action='append',required=True)
p.add_argument('--test-arg',action='append',default=[])
p.add_argument('--khronos-headers',type=Path)
p.add_argument('--ffmpeg-root',type=Path,required=True)
p.add_argument('--test-source',default='engine_vid_test.cpp',choices=['engine_vid_test.cpp','engine_session_test.cpp','engine_sound_test.cpp','engine_vid_gles_test.cpp','engine_graphics_startup_test.cpp','engine_mission_boot_test.cpp','engine_simulation_test.cpp'])
a=p.parse_args();repo=Path(__file__).resolve().parents[1];a.output.mkdir(parents=True,exist_ok=True)
objects={};compile_command=None
for directory in a.objects:
 for unit in json.loads((directory/'results.json').read_text())['units']:
  source=unit['source'];objects.pop(source,None)
  if unit['exit_code']==0:
   objects[source]=directory/(source.replace('/','__')+'.o')
   if source=='graphics/vid.cpp':compile_command=unit['command'][:-3]
if compile_command is None:raise RuntimeError('No compiled native Vid source')
obj=a.output/(Path(a.test_source).stem+'.o');binary=a.output/Path(a.test_source).stem;archive=a.output/'original_host.a'
if a.test_source in ('engine_vid_gles_test.cpp','engine_graphics_startup_test.cpp','engine_mission_boot_test.cpp','engine_simulation_test.cpp'):
 if not a.khronos_headers:raise RuntimeError('Provide --khronos-headers for real GLES integration test')
 compile_command += ['-I'+str(a.khronos_headers),'-I'+str(repo/'include'),'-DUSE_OZONE']
compile_command += ['-fsanitize=address,undefined',str(repo/'tests'/a.test_source),'-o',str(obj)]
subprocess.run(compile_command,check=True)
archive.unlink(missing_ok=True)
subprocess.run(['ar','rcs',str(archive)]+[str(o) for o in objects.values()],check=True)
command=[compile_command[0],'-no-pie','-fsanitize=address,undefined',str(obj),'-Wl,--gc-sections','-Wl,--start-group',str(archive)]
command += [str(f) for f in sorted(a.foundation.glob('libdr2_engine_*.a'))]
command += [str(a.ffmpeg_root/'lib'/('lib'+n+'.a')) for n in ['avformat','avcodec','swscale','avutil']]
if a.test_source in ('engine_vid_gles_test.cpp','engine_graphics_startup_test.cpp','engine_mission_boot_test.cpp','engine_simulation_test.cpp'):command += ['-l:libEGL.so.1']
if a.test_source in ('engine_mission_boot_test.cpp','engine_simulation_test.cpp'):command += ['-Wl,--wrap=__cxa_throw']
command += ['-Wl,--end-group','-pthread','-ldl','-lm','-o',str(binary)]
subprocess.run(command,check=True)
subprocess.run([str(binary.resolve())]+a.test_arg,check=True)

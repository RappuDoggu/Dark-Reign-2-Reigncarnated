#!/usr/bin/env python3
"""Expose unresolved original mission-entry dependencies, without runtime stubs."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('staged',type=Path)
p.add_argument('foundation_build',type=Path)
p.add_argument('output',type=Path)
p.add_argument('--compiler',required=True,type=Path)
p.add_argument('--objects',type=Path,action='append',required=True,help='Compilation directories in override order')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
a.output.mkdir(parents=True,exist_ok=True)
objects={}
for directory in a.objects:
 report=json.loads((directory/'results.json').read_text())
 for unit in report['units']:
  # A failed newer build must not silently reuse an older successful object.
  objects.pop(unit['source'],None)
  if unit['exit_code']==0:
   source=unit['source']
   # Native input owns the adapted original event queue and replaces DirectInput.
   if source in ('util/event.cpp','interface/input.cpp'):continue
   obj=directory/(source.replace('/','__')+'.o')
   if not obj.is_file():raise RuntimeError(f'Missing successful object: {obj}')
   objects[source]=obj
archive=a.output/'original_engine.a'
archive.unlink(missing_ok=True)
subprocess.run([str(a.compiler.parent/'llvm-ar'),'rcs',str(archive)]+[str(obj) for obj in objects.values()],check=True)
cmd=[str(a.compiler),'-std=c++17','-I'+str(a.staged/'system'),'-I'+str(a.staged/'coregame'),str(repo/'tests/engine_mission_link_probe.cpp'),'-Wl,--gc-sections','-Wl,-u,RequireMissionEntryPoints','-Wl,--error-limit=0','-Wl,--start-group',str(archive)]
cmd += [str(a.foundation_build/('libdr2_engine_'+name+'.a')) for name in ['io','resources','missions','type_idents','clock','input','main_runtime']]
cmd+=['-Wl,--end-group','-o',str(a.output/'mission_link_probe')]
r=subprocess.run(cmd,capture_output=True,text=True)
(a.output/'link.log').write_text(r.stdout+r.stderr)
errors=[line.split('undefined symbol: ',1)[1] for line in r.stderr.splitlines() if 'undefined symbol: ' in line]
other_errors=[line for line in r.stderr.splitlines() if 'error:' in line and 'undefined symbol:' not in line and 'linker command failed' not in line]
(a.output/'result.json').write_text(json.dumps({'linked':r.returncode==0,'mission_booted':False,'object_files':len(objects),'undefined_symbols':sorted(set(errors)),'other_errors':other_errors,'command':cmd},indent=2)+'\n')
print(f'Link exit {r.returncode}; {len(set(errors))} reported unresolved symbols; {len(other_errors)} other errors; no mission executed.')
raise SystemExit(r.returncode)

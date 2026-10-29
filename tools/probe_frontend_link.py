#!/usr/bin/env python3
"""Record the complete staged frontend link boundary without dead-code removal.

A failed link is evidence, not a successful engine startup. Never runs the result.
Output must stay outside the project because a successful binary contains engine
code whose redistribution rights have not been established.
"""
import argparse,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('build',type=Path);p.add_argument('output',type=Path)
p.add_argument('--compiler',default='c++');p.add_argument('--sanitize',action='store_true')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();build=a.build.resolve()
if out==repo or repo in out.parents:raise ValueError('External link output required')
objects=sorted((build/'CMakeFiles/dr2_frontend_controls.dir').rglob('*.cpp.o'))
if not objects:raise ValueError('Build the original frontend controls first')
libraries=sorted(build.glob('lib*.a'))
if not libraries:raise ValueError('No built static dependencies')
out.mkdir(parents=True,exist_ok=True);main=out/'main.cpp';main.write_text('int main() { return 0; }\n')
cmd=[a.compiler]+(['-fsanitize=address,undefined'] if a.sanitize else [])+[str(main)]+list(map(str,objects))+['-Wl,--start-group']+list(map(str,libraries))+['-Wl,--end-group','-pthread','-ldl','-o',str(out/'frontend-link-probe')]
r=subprocess.run(cmd,text=True,capture_output=True);log=r.stdout+r.stderr;(out/'link.log').write_text(log)
missing=sorted(set(re.findall(r"undefined reference to [`‘](.*?)[’']",log)))
report={'compiler_exit_code':r.returncode,'linked':r.returncode==0,'engine_initialized':False,'menu_rendered':False,'objects':len(objects),'unresolved_symbols':missing,'command':cmd}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
# An unexpected toolchain failure must not look like an understood link boundary.
if r.returncode and not missing:raise SystemExit(r.returncode)

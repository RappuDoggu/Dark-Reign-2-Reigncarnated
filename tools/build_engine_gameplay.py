#!/usr/bin/env python3
"""Compile complete gameplay translation units; compilation is not a mission boot.

Inputs are the pinned project inventory and native staging tree. Failed units
retain diagnostics; no failures are hidden and no unresolved APIs are stubbed.
"""
import argparse
import concurrent.futures
import json
import hashlib
import fcntl
from pathlib import Path
import subprocess
from probe_full_engine import inventory

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('upstream',type=Path)
 p.add_argument('staged',type=Path)
 p.add_argument('output',type=Path)
 p.add_argument('--compiler',default='c++')
 p.add_argument('--jobs',type=int,default=4)
 p.add_argument('--retry-failed',action='store_true')
 p.add_argument('--all-projects',action='store_true')
 a=p.parse_args();repo=Path(__file__).resolve().parents[1]
 projects=inventory(a.upstream.resolve());root=a.staged.resolve()
 lock_file=root.with_name(root.name+'.lock').open('a')
 fcntl.flock(lock_file,fcntl.LOCK_SH)
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 directories=sorted({str(root/Path(s).parent) for project in projects.values() for s in project['sources']})
 directories += [str(root/'3rdparty'),str(root/'3rdparty/dx7'),str(root/'3rdparty/won')]
 base=[a.compiler,'-std=c++17','-DDEVELOPMENT=1','-DNO_ASM_MATH','-DMONO_DISABLED','-ffunction-sections','-fdata-sections','-c']
 for path in [repo/'engine/platform',repo/'engine/platform/dx7']+directories:
  base+=['-I',str(path)]
 fingerprint=hashlib.sha256()
 for directory in [root,repo/'engine/platform']:
  for source in sorted(directory.rglob('*')):
   if source.is_file() and not source.is_symlink():
    fingerprint.update(str(source.relative_to(directory)).encode())
    fingerprint.update(source.read_bytes())
 fingerprint.update(json.dumps(base).encode())
 fingerprint=fingerprint.hexdigest()
 previous={}
 report=out/'results.json'
 if a.retry_failed and report.exists():
  old=json.loads(report.read_text())
  if old.get('input_fingerprint')==fingerprint:previous={r['source']:r for r in old['units']}
 items=[(s,project['forced_includes']) for project in projects.values() for s in project['sources'] if a.all_projects or Path(s).parts[0] in ['coregame','coregame_objects','coregame_tasks','coregame_orders','game']]
 def run(item):
  source,forced=item
  if source in previous and previous[source]['exit_code']==0:return previous[source]
  stem=source.replace('/','__');cmd=base.copy()
  for f in forced:cmd+=['-include',f]
  object_file=out/(stem+'.o')
  object_file.unlink(missing_ok=True)
  cmd += [str(root/source),'-o',str(object_file)]
  result=subprocess.run(cmd,capture_output=True,text=True)
  (out/(stem+'.log')).write_text(result.stdout+result.stderr)
  return {'source':source,'exit_code':result.returncode,'diagnostics':stem+'.log','command':cmd}
 with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:results=list(pool.map(run,items))
 passed=sum(r['exit_code']==0 for r in results)
 report.write_text(json.dumps({'input_fingerprint':fingerprint,'compiled':passed,'total':len(results),'linked':False,'mission_booted':False,'units':results},indent=2)+'\n')
 print(f'{passed}/{len(results)} complete source units compiled; not linked or executed.')
 for r in results:
  if r['exit_code']:
   errors=[l for l in (out/r['diagnostics']).read_text().splitlines() if 'error:' in l]
   print(r['source'],errors[:2])
 return 0 if passed==len(results) else 1
if __name__=='__main__':raise SystemExit(main())

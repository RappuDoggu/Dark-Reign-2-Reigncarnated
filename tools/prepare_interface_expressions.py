#!/usr/bin/env python3
"""Stage original variable operation, comparison and switch implementations."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 output.mkdir(parents=True,exist_ok=True)
 for name,digest in json.loads((repo/'cmake/interface-expression-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed expression source: '+name)
  s=raw.decode().replace('\r\n','\n').replace('#include "stdload.h"','#include "control_stdload.hpp"')
  if name=='util/switch.cpp':
   old='RelationalOperator<F32> oper("");'
   if s.count(old)!=1:raise ValueError('Float switch fix anchor changed')
   s=s.replace(old,'RelationalOperator<F32> oper("==");')
  if name.endswith('.cpp'):s='#include "control_types.hpp"\n#include "control_stdload.hpp"\n'+s
  (output/Path(name).name).write_text(s)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
 a=p.parse_args();prepare(a.upstream,a.output)

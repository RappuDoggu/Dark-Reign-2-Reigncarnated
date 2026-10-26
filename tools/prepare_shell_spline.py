#!/usr/bin/env python3
"""Compile pinned original camera spline arithmetic in an isolated namespace."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==repo or repo in out.parents:raise ValueError('External staging required')
texts={}
for name,digest in json.loads((repo/'cmake/spline-source-hashes.json').read_text()).items():
 raw=(a.upstream/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed spline source: '+name)
 texts[name]=raw.decode().replace('\r\n','\n')
h=texts['coregame/spline.h'].replace('#include "mathtypes.h"','#include "reign/spline_types.hpp"')
h=h.replace('class CubicSpline','namespace reign::camera_detail {\nclass CubicSpline').replace('\n#endif','\n}\n#endif')
s=texts['coregame/spline.cpp'].replace('#include "spline.h"','#include "spline.h"\nnamespace reign::camera_detail {')+'\n}\n'
out.mkdir(parents=True,exist_ok=True)
for name,text in [('spline.h',h),('spline.cpp',s)]:
 path=out/name
 if not path.exists() or path.read_text()!=text:path.write_text(text)

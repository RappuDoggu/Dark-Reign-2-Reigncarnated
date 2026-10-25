#!/usr/bin/env python3
"""Stage reviewed original UI triangle generation outside the repository."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External source staging required')
 for name,digest in json.loads((repo/'cmake/frontend-draw-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed draw source: '+name)
 source=raw.decode().replace('\r\n','\n')
 a=source.index('  void RenderTriangle(');b=source.index('{',a)+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 body=source[a:b]
 output.mkdir(parents=True,exist_ok=True)
 (output/'frontend_triangle.cpp').write_text('#include "reign/frontend_draw.hpp"\nnamespace IFace {\n'+body+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
 a=p.parse_args();prepare(a.upstream,a.output)

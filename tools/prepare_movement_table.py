#!/usr/bin/env python3
"""Stage the original movement balance table outside the project repository."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==repo or repo in out.parents:raise ValueError('External staging required')
texts={}
for name,digest in json.loads((repo/'cmake/movement-table-hashes.json').read_text()).items():
 raw=(a.upstream/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed movement source: '+name)
 texts[name]=raw.decode().replace('\r\n','\n')
out.mkdir(parents=True,exist_ok=True)
for name,s in texts.items():
 if name.endswith('balancetable.h'):
  # Standard C++ requires typename for the nested type in this template.
  s=s.replace('for (NBinTree<KeyInfo>::Iterator','for (typename NBinTree<KeyInfo>::Iterator')
 if name.endswith('movetable.cpp'):s=s.replace('#include "stdload.h"','#include "control_stdload.hpp"')
 if name.endswith('movetable.h'):s='#include "control_types.hpp"\n#include "ntree.h"\n'+s
 (out/Path(name).name).write_text(s)

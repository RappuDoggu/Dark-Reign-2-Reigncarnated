#!/usr/bin/env python3
"""Stage the pinned original Terrain::Load with explicit data-only service boundaries."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==repo or repo in out.parents:raise ValueError('External staging required')
pins=json.loads((repo/'cmake/terrain-source-hashes.json').read_text())
for name,digest in pins.items():
 raw=(a.upstream/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed terrain source: '+name)
 if name=='graphics/terrain.cpp':text=raw.decode().replace('\r\n','\n')
start=text.index('  Bool Load( BlockFile &bfile)');end=text.index('  //----------------------------------------------------------------------------',start)
s=text[start:end]
def replace(old,new):
 global s
 if s.count(old)!=1:raise ValueError('Unreviewed terrain slice shape: '+old)
 s=s.replace(old,new)
replace('Bool Load( BlockFile &bfile)','Bool TerrainDecoder::Load( TerrainBlock &bfile)')
replace('bfile.OpenBlock(TERRAINKEY);','bfile.OpenBlock("terraindata");')
replace('bfile.ReadFromBlock( &version, sizeof( version));','bfile.ReadFromBlock( &version, sizeof( version));\n    if(version!=17)throw std::runtime_error("Only terrain v17 is validated at this boundary");')
replace('bfile.ReadFromBlock( &normCount, sizeof( normCount));','bfile.ReadFromBlock( &normCount, sizeof( normCount));\n    if(!normCount||normCount>256)throw std::runtime_error("Invalid terrain normal count");')
replace('    U8 mapper[256];','    if(!tcount||tcount>256)throw std::runtime_error("Invalid terrain texture count");\n    const U32 sourceTextureCount=tcount;\n    U8 mapper[256]{};')
replace('bfile.ReadFromBlock( &slen, sizeof( slen));','bfile.ReadFromBlock( &slen, sizeof( slen));\n      if(!slen||slen>sizeof(name))throw std::runtime_error("Invalid terrain texture name length");')
replace('bfile.ReadFromBlock( name, slen);','bfile.ReadFromBlock( name, slen);\n      if(name[slen-1]!=0||std::strlen(name)!=slen-1)throw std::runtime_error("Invalid terrain texture name");')
replace('      //overlays\n      bfile.ReadFromBlock( &tcount, sizeof( tcount));','      //overlays\n      bfile.ReadFromBlock( &tcount, sizeof( tcount));\n      if(tcount>65535)throw std::runtime_error("Invalid terrain overlay count");')
# Both historical cell loops retain the original assignments. Validate before indexing.
s=s.replace('cell.texture = mapper[texture];','if(texture>=sourceTextureCount||normal>=normCount)throw std::runtime_error("Invalid terrain cell index");\n          cell.texture = mapper[texture];')
s=s.replace('Vid::Var::Terrain::Sky::','environment.sky.').replace('Vid::Var::Terrain::','environment.')
replace('    Recalc();\n    SetupPerf();','    // Data-only boundary: original renderer recalculation and performance setup are not invoked.')
s='#include "reign/config_prelude.hpp"\n#include "reign/terrain_load_adapter.hpp"\n#define DOTERRAINCOLOR\nnamespace reign::terrain_detail {\n'+s+'\n}\n'
out.mkdir(parents=True,exist_ok=True);path=out/'terrain_load.cpp'
if not path.exists() or path.read_text()!=s:path.write_text(s)

#!/usr/bin/env python3
"""Stage original bitmap naming/fallback/animation/release loops externally."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Bitmap manager overlay must be external')
    texts={}
    for name,digest in json.loads((repo/'cmake/bitmap-manager-hashes.json').read_text()).items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed bitmap manager source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    source=texts['graphics/bitmap_manager.cpp']
    def function(signature):
        a=source.index(signature);start=source.index('{',a);depth=1;b=start+1
        while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
        return source[a:b]
    parts=[function(s) for s in ['void Bitmap::Manager::Setup(', 'Bitmap * Bitmap::Manager::Create(', 'Bitmap * Bitmap::Manager::Find(', 'Bitmap * Bitmap::Manager::FindCreate(', 'void Bitmap::Manager::ReleaseDD()', 'void Bitmap::Manager::OnModeChange()']]
    parts[1]=parts[1].replace('Bitmap *bmp = new Bitmap;', 'auto owned = std::make_unique<Bitmap>();\n  Bitmap *bmp = owned.get();')
    parts[1]=parts[1].replace('  return bmp;', '  state().pending.push_back(bmp);\n  owned.release();\n  return bmp;')
    movie=parts[3].index('    if (bmp->surface &&')
    end=parts[3].index('\n  }\n\n  return bmp;',movie)
    parts[3]=parts[3][:movie]+'    // Bink movie dispatch is outside this level-zero static/numbered-texture adapter.\n'+parts[3][end:]
    parts[3]=parts[3].replace('bmp->animFrame = count - 1;', 'bmp->animFrame = count - 1; bmp->record->frame = bmp->animFrame;')
    old='sprintf( texname.str, "%s%d%s", fname.str, count, fext.str);'
    if parts[3].count(old)!=1:raise ValueError('Unexpected animation formatting')
    parts[3]=parts[3].replace(old,'if (std::snprintf(texname.str, sizeof(texname.str), "%s%u%s", fname.str, count, fext.str) >= int(sizeof(texname.str))) throw std::runtime_error("Animation filename overflow");')
    block='  if (Vid::d3d)\n  {\n    Vid::d3d->EvictManagedTextures();\n  }'
    if parts[4].count(block)!=1:raise ValueError('Unexpected DirectDraw eviction boundary')
    parts[4]=parts[4].replace(block,'  // Native Bitmap::ReleaseDD owns actual resource deletion below.')
    body='\n\n'.join(parts).replace('&tree','&state().tree').replace('tree.Add(', 'state().tree.Add(').replace('tree.Find(', 'state().tree.Find(')
    output.mkdir(parents=True,exist_ok=True);(output/'ntree.h').write_text(texts['system/ntree.h'])
    (output/'manager_slice.cpp').write_text('// Selected original implementation: Copyright 1997-2000 Pandemic Studios, Dark Reign II.\n#include "reign/bitmap_manager_prelude.hpp"\nnamespace reign::bitmap_manager_detail {\n'+body+'\n}\n')
    print('Staged original Bitmap::Manager naming/fallback/animation/release loops externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

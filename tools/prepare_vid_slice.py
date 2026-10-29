#!/usr/bin/env python3
"""Stage hash-reviewed original Vid state setters externally; never vendor source."""
import argparse,hashlib,json,re
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1]
    output=output.resolve()
    if output==repo or repo in output.parents: raise ValueError('Overlay must be outside repository')
    pins=json.loads((repo/'cmake/vid-source-hashes.json').read_text())
    texts={}
    for name,digest in pins.items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    vertex=texts['graphics/Vertex.h']
    macros=vertex[vertex.index('#define RS_ADD_SHIFT'):vertex.index('enum DRAWPRIMITIVE_FLAGS')]
    flags=vertex[vertex.index('  RS_NOINDEXED'):vertex.index('// all of the supported vertex data formats')]
    # Only remove unrelated trailing comment. Enum members remain original.
    flags=flags[:flags.index('};')+2]
    source=texts['graphics/vidrend.cpp']
    body=source[source.index('  Bool SetZBufferState'):source.index('  void ValidateBlends()')]
    # Cull state requires a winding convention fixture and is deliberately excluded.
    start=body.index('  void SetCullStateD3D')
    end=body.index('  Bool SetAlphaState',start)
    body=body[:start]+body[end:]
    old='static char * blendName[8]'
    if body.count(old)!=1:raise ValueError('Unexpected blend-name declaration')
    body=body.replace(old,'static const char * blendName[8]')
    texture=source[source.index('  Bool SetTextureDX('):source.index('  void SetBucketMaterialProc(')]
    if texture.count('const Bitmap * tex')!=1 or texture.count(': NULL')!=1:raise ValueError('Unexpected texture binding shape')
    texture=texture.replace('const Bitmap * tex','const reign::TextureView * tex').replace(': NULL',': 0')
    body+=texture
    draw_start=source.index('  Bool DrawIndexedPrimitive(')
    draw_end=source.index('\n  }\n',draw_start)+len('\n  }\n')
    draw=source[draw_start:draw_end]
    start=draw.index('    // Check for errors')
    end=draw.index('#ifndef DODXLEANANDGRUMPY',start)
    # Replace Win32 debug-only range logging with unconditional bounded adapter validation.
    draw=draw[:start]+draw[end:]
    body+=draw
    names=['SetTextureDX','SetZBufferState','SetZWriteState','SetAlphaState','SetSrcBlendState','SetDstBlendState','SetTexWrapState','TexDecal','TexDecalSimple','TexDecalAlpha','TexModulate','TexModulateAlpha','TexModulateAlpha2x','TexModulateAlpha4x','TexAddAlpha','TexAddSignedAlpha','SetTexBlendState']
    declarations=[]
    for name in names:
        match=re.search(r'  (Bool|U32|void) '+name+r'\([^\n]+',body)
        if not match:raise ValueError('Missing function '+name)
        declarations.append(match.group(0).split('//')[0].rstrip()+';')
    output.mkdir(parents=True,exist_ok=True)
    declarations.append('Bool DrawIndexedPrimitive(PRIMITIVE_TYPE,VERTEX_TYPE,LPVOID,DWORD,LPWORD,DWORD,DWORD);')
    (output/'vid_slice.hpp').write_text('#pragma once\n#include "reign/vid_prelude.hpp"\n'+macros+'enum DRAWPRIMITIVE_FLAGS {\n'+flags+'\nnamespace Vid {\n'+'\n'.join(declarations)+'\n}\n')
    (output/'vid_slice.cpp').write_text('// Original selected implementation: Copyright 1997-2000 Pandemic Studios, Dark Reign II.\n#include "vid_slice.hpp"\nnamespace Vid {\n'+body+'\n}\n')
    print('Staged original Vid state setters; stage 0 only; external overlay:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

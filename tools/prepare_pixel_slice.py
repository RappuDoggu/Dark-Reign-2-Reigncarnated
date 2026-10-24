#!/usr/bin/env python3
"""Isolate original Pix and Bitmap::CopyLine without the full DirectDraw Bitmap class."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Pixel overlay must be external')
    pins=json.loads((repo/'cmake/pixel-source-hashes.json').read_text());texts={}
    for name,digest in pins.items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed pixel source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    header=texts['graphics/bitmap.h']
    pix=header[header.index('struct Pix\n'):header.index('//-----------------------------------------------------------------------------',header.index('struct Pix\n'))]
    start=pix.index('  U32 MakeRGBA( Color color)');end=pix.index('\n  }',start)+len('\n  }')
    pix=pix[:start]+pix[end:] # The original integer overload is the tested boundary.
    primitive=texts['graphics/bitmapprimitive.cpp']
    setup=primitive[primitive.index('void Pix::Setup()'):]
    # This reviewed file ends after Pix::SetName and its trailing separator.
    if setup.count('void Pix::')!=2:raise ValueError('Unexpected pixel method tail')
    bitmap=texts['graphics/bitmap.cpp']
    copy=bitmap[bitmap.index('void Bitmap::CopyLine('):bitmap.index('#pragma pack(push,1)',bitmap.index('void Bitmap::CopyLine('))]
    old='*bits++ = *((Bit24Color *)&color);'
    if copy.count(old)!=3:raise ValueError('Unexpected 24-bit aliases')
    copy=copy.replace(old,'std::memcpy(bits++, &color, 3);')
    output.mkdir(parents=True,exist_ok=True)
    (output/'pixel_slice.hpp').write_text('#pragma once\n#include "reign/pixel_prelude.hpp"\n'+pix+'\n'+
        'class Bitmap {public: int bmpDepth; Pix *pixForm;\n'
        'U32 MakeRGBA(U32 r,U32 g,U32 b,U32 a=255)const{return pixForm->MakeRGBA(r,g,b,a);}\n'
        'void CopyLine(U8 *,U8 *,int,int,RGBQUAD *);};\n')
    (output/'pixel_slice.cpp').write_text('// Selected original pixel implementation: Copyright 1997-2000 Pandemic Studios, Dark Reign II.\n#include "pixel_slice.hpp"\n'+setup+'\n'+copy)
    print('Staged original Pix/CopyLine pixel conversion externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

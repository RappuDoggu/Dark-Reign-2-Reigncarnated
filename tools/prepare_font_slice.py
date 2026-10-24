#!/usr/bin/env python3
"""Stage reviewed original font and IFace batching code outside the repository."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Font overlay must remain external')
    texts={}
    for name,digest in json.loads((repo/'cmake/font-source-hashes.json').read_text()).items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed font source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    h=texts['interface/font.h'];h=h[h.index('\nclass Font\n{')+1:h.rindex('#endif')]
    h=h.replace("'TNOF'",'0x544e4f46u').replace("'KROB'",'0x4b524f42u')
    if h.count('U8 charData[0];')!=1:raise ValueError('Unexpected flexible glyph record')
    h=h.replace('U8 charData[0];','// Pixels follow the two-byte header in the validated file buffer.')
    cpp=texts['interface/font.cpp'];cpp=cpp[cpp.index('Font::Font()'):]
    # Alignment-safe reads; byte interpretation and pointer advancement unchanged.
    cpp=cpp.replace('*(U32 *)filePtr','load32(filePtr)').replace('*(U16 *)filePtr','load16(filePtr)')
    # CPU atlas publication is an explicit boundary, not a stub GPU upload.
    if cpp.count('texture->LoadVideo()')!=2:raise ValueError('Unexpected atlas publication sites')
    cpp=cpp.replace('texture->LoadVideo()','texture->Publish()')
    # Own the temporary arrays across fatal-error exceptions at this native boundary.
    cpp=cpp.replace('CharHeader *charHeaderList = new CharHeader[numChar];','std::unique_ptr<CharHeader[]> charHeaderList(new CharHeader[numChar]);')
    cpp=cpp.replace('CharImage **charImageList = new CharImage *[numChar];','std::unique_ptr<CharImage *[]> charImageList(new CharImage *[numChar]);')
    cpp=cpp.replace('delete[] charImageList;','charImageList.reset();').replace('delete[] charHeaderList;','charHeaderList.reset();')
    cpp=cpp.replace('.vv.','.').replace('charImage->charData','reinterpret_cast<U8 *>(charImage)+sizeof(CharImage)')
    # The original source explicitly defines LOG_FONT as disabled.
    prefix='#include "font_slice.hpp"\n#define LOG_FONT(x)\nstatic const S32 Gutter=1;\n'
    fs=texts['interface/fontsys.cpp'];a=fs.index('  Bitmap *AllocTexture(');b=fs.index('  // Release all textures',a)
    fs=fs[a:b]
    fs=fs.replace('    if (currTex == MAX_TEX)','    auto &currTex=context().currTex;auto &textures=context().textures;\n    if (currTex == MAX_TEX)')
    fs=fs.replace('    textures[handle] = new Bitmap(bitmapTEXTURE | bitmapNOMANAGE);','    auto owned=std::make_unique<Bitmap>(bitmapTEXTURE | bitmapNOMANAGE);\n    textures[handle]=owned.get();context().owned.push_back(std::move(owned));')
    fs=fs.replace('    ASSERT(handle < MAX_TEX)','    auto &textures=context().textures;\n    ASSERT(handle < MAX_TEX)')
    ui=texts['interface/iface_util.cpp'];start=ui.index('  static U32 heapSize = 0;');end=ui.index('  const CH * BreakText',start)
    ui=ui[start:end]
    # Append a real cleanup path if a draw consumer throws; no partial draw is reported successful.
    ui+='\nvoid AbortBuckets()noexcept{heapSize=0;curVert=curIndex=0;bucketValid=FALSE;}\n'
    ui=ui.replace('  static U32 heapSize = 0;','  static thread_local U32 heapSize = 0;')
    globals='static thread_local VertexTL *vertList;static thread_local U32 curVert,maxVert,curIndex,maxIndex,flushCount;static thread_local U16 *indexList;static thread_local Bool bucketValid=FALSE;static thread_local Bitmap *bucketTex;static thread_local U32 bucketFilter,bucketClamp;\n'
    output.mkdir(parents=True,exist_ok=True)
    notice='// Selected original implementation: Copyright 1997-1999 Pandemic Studios, Dark Reign II.\n'
    (output/'font_slice.hpp').write_text(notice+'#pragma once\n#include "reign/font_prelude.hpp"\nnamespace reign::font_detail {\n'+h+'\nstatic_assert(sizeof(Font::CharImage)==2 && sizeof(Font::CharHeader)==7 && sizeof(Font::FontHeader)==9);\n}\n')
    (output/'font_slice.cpp').write_text(notice+prefix+'namespace reign::font_detail {\n'+cpp+'\nnamespace FontSys {const int MAX_TEX=1000;\n'+fs+'\n}\nnamespace IFace {\n'+globals+ui+'\n}\n}\n')
    print('Staged original font reader/layout/draw/IFace batching externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

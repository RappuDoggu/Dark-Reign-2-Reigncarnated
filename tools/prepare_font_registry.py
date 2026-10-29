#!/usr/bin/env python3
"""Stage original FontSys registration separately from atlas/draw implementation."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Registry staging must be external')
    pins=json.loads((repo/'cmake/font-registry-hashes.json').read_text());texts={}
    for name,digest in pins.items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed registry dependency: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    source=texts['interface/fontsys.cpp']
    structs=source[source.index('  struct FontFile'):source.index('  // System init flag')]
    def function(signature):
        start=source.index(signature);brace=source.index('{',start);depth=1;end=brace+1
        while depth:
            depth+=(source[end]=='{')-(source[end]=='}');end+=1
        return source[start:end]
    parts=[function(s) for s in ['  void Init()', '  void Done()', '  Bool Create(const char *name,', '  void Create(FScope *fScope)', '  void DeleteAll()', '  Font *GetFont(const char *name)', '  Font *GetFont(U32 crc)', '  void OnModeChange()']]
    init=parts[0]
    for line in ['    memset(textures, 0, sizeof(textures));\n','    currTex = 0;\n']:
        if init.count(line)!=1:raise ValueError('Unexpected Init texture ownership')
        init=init.replace(line,'')
    parts[0]=init
    for i in [4,7]:
        if parts[i].count('    ReleaseAllTextures();')!=1:raise ValueError('Unexpected release boundary')
        parts[i]=parts[i].replace('    ReleaseAllTextures();','    // NativeFont instances own atlas lifetimes at this boundary.')
    create=parts[2]
    create=create.replace('      fontFile = new FontFile;','      auto ownedFile = std::make_unique<FontFile>();\n      fontFile = ownedFile.get();')
    create=create.replace('      fontFiles.Add(fontFile->name.crc, fontFile);','      fontFiles.Add(fontFile->name.crc, fontFile);\n      ownedFile.release();')
    create=create.replace('    FontItem *fontItem = new FontItem;','    auto ownedItem = std::make_unique<FontItem>();\n    FontItem *fontItem = ownedItem.get();')
    create=create.replace('    fontItems.Add(fontItem->name.crc, fontItem);','    fontItems.Add(fontItem->name.crc, fontItem);\n    ownedItem.release();')
    parts[2]=create
    body='\n\n'.join(parts)
    for name in ['sysInit','fontFiles','fontItems']:body=body.replace(name,'state().'+name)
    output.mkdir(parents=True,exist_ok=True)
    (output/'ntree.h').write_text(texts['system/ntree.h'])
    notice='// Selected original FontSys implementation: Copyright 1997-1999 Pandemic Studios, Dark Reign II.\n'
    declarations='void Init();void Done();Bool Create(const char *,const char *,U32=0);void Create(FScope *);void DeleteAll();Font *GetFont(const char *);Font *GetFont(U32);void OnModeChange();\n'
    (output/'registry_slice.hpp').write_text(notice+'#pragma once\n#include "reign/font_registry_prelude.hpp"\nnamespace reign::registry_detail {\n'+structs+'\nstruct State {Bool sysInit=FALSE;NBinTree<FontFile> fontFiles{&FontFile::node};NBinTree<FontItem> fontItems{&FontItem::node};reign::ConfigLoader loader;};\nState &state();\n'+declarations+'}\n')
    (output/'registry_slice.cpp').write_text(notice+'#include "registry_slice.hpp"\nnamespace reign::registry_detail {\n'+body+'\n}\n')
    print('Staged original FontSys alias/config/mode-change registration externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

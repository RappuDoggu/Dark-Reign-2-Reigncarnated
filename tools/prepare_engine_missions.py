#!/usr/bin/env python3
"""Stage whole original mission, campaign, difficulty and utility sources.

OS-independent declarations replace broad GUI includes. No gameplay functions
are synthesized. The test links only the reachable mission-selection path;
unused save/UI entry points retain unresolved dependencies for later integration.
"""
import argparse, hashlib, json, re
from pathlib import Path

def prepare(upstream, output):
    repo=Path(__file__).resolve().parents[1]
    upstream,output=upstream.resolve(),output.resolve()
    if output==repo or repo in output.parents or output==upstream or upstream in output.parents:
        raise ValueError('External staging required')
    texts={}
    for name,digest in json.loads((repo/'cmake/engine-mission-source-hashes.json').read_text()).items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest: raise ValueError('Unreviewed source: '+name)
        texts[Path(name).name]=raw.decode().replace('\r\n','\n')
    for name in ['missions.h','difficulty.h']:
        texts[name]=texts[name].replace('#include "multilanguage.h"','#include "multilanguage_types.h"')
    for name in ['missions.cpp','runcodes.cpp']:
        texts[name]=texts[name].replace('#include "main.h"','#include "main_runtime.h"')
    texts['array.h']=texts['array.h'].replace('#include "debug_memory.h"','#include "aligned_memory.h"')
    # Dependent-base names need explicit qualification on GCC/Clang.
    texts['array.h']=texts['array.h'].replace('class Array4 : public Array<DATA, ALIGNMENT>\n{\npublic:',
        'class Array4 : public Array<DATA, ALIGNMENT>\n{\npublic:\n'
        '  using Array<DATA, ALIGNMENT>::data;\n  using Array<DATA, ALIGNMENT>::count;\n'
        '  using Array<DATA, ALIGNMENT>::size;\n  using Array<DATA, ALIGNMENT>::ClearData;\n'
        '  using Array<DATA, ALIGNMENT>::Release;')
    # RECT casts are a Windows GUI convenience, not part of engine geometry.
    a=texts['area.h'].index('  // windows conversion')
    b=texts['area.h'].index('\n};',a)
    texts['area.h']=texts['area.h'][:a]+'#if defined(_WIN32)\n'+texts['area.h'][a:b]+'\n#endif\n'+texts['area.h'][b:]
    # Preserve the 64-byte matrix rows. The unused SIMD Mat alias is not a
    # standard C++ anonymous aggregate (Vector has constructors).
    a=texts['mathtypes.h'].index('\tunion {',texts['mathtypes.h'].index('struct Matrix\n'))
    b=texts['mathtypes.h'].index('  Matrix() {}',a)
    texts['mathtypes.h']=texts['mathtypes.h'][:a]+(
        '  Vector right; F32 rightw;\n  Vector up; F32 upw;\n'
        '  Vector front; F32 frontw;\n  Vector posit; F32 positw;\n\n')+texts['mathtypes.h'][b:]
    texts['mathtypes.h']+='\nstatic_assert(sizeof(Vector)==12 && sizeof(Matrix)==64);\n'
    texts['reaperlist.h']=texts['reaperlist.h'].replace(
        'class ReaperList : public NList<NODE>\n{\npublic:',
        'class ReaperList : public NList<NODE>\n{\npublic:\n'
        '  using Iterator = typename NList<NODE>::Iterator;\n'+
        ''.join('  using NList<NODE>::'+n+';\n' for n in
          ['SetNodeMember','GetHeadNode','GetTailNode','GetHead','GetCount','Unlink','Dispose']))
    texts['reaperlist.h']=texts['reaperlist.h'].replace('NList<NODE>::Node *node', 'typename NList<NODE>::Node *node')
    texts['stdload.h']=texts['stdload.h'].replace('for (ReaperList<DATA, NODE>::Iterator', 'for (typename ReaperList<DATA, NODE>::Iterator')
    texts['stdload.h']=texts['stdload.h'].replace('TypeStrBufList(fScope->GetFunction(name), idents);','TypeStrBufList(parent->GetFunction(name), strings);')
    texts['stdload.h']=texts['stdload.h'].replace('TypeStrBufBinTree(fScope->Getfunction(name), idents);','TypeStrBufBinTree(parent->GetFunction(name), strings);')
    texts['stdload.h']=texts['stdload.h'].replace('TypeStrCrcList(fScope->GetFunction(name), idents);','TypeStrCrcList(fScope->GetFunction(name), strings);')
    # This original API encodes a heap pointer in a 32-bit object ID. Do not
    # silently truncate it. It remains an explicit link-time integration gap
    # until the matching coregame Resolver::Type name-resolution path is ported.
    old='inline U32 GetIdentAsU32(const char *ident)\n  {\n    return (reinterpret_cast<U32>(new GameIdent(ident)));\n  }'
    if old not in texts['stdload.h']: raise ValueError('Unexpected reaper name encoding')
    texts['stdload.h']=texts['stdload.h'].replace(old,'U32 GetIdentAsU32(const char *ident);')
    texts['utiltypes.cpp']=texts['utiltypes.cpp'].replace('#include "perfstats.h"','// No performance-counter dependency in this translation unit.')

    # Declare static-member specializations before stdload's default arguments
    # cause implicit instantiation (Clang rejects late specializations).
    declarations=re.findall(r'^(Range<[^>]+> const Range<[^>]+>::\w+)\(',texts['utiltypes.cpp'],re.M)
    texts['utiltypes.h']+='\n'+'\n'.join('template<> '+d+';' for d in declarations)+'\n'
    texts['utiltypes.cpp']=texts['utiltypes.cpp'].replace('\nRange<','\ntemplate<> Range<')
    texts['utiltypes.cpp']=re.sub(r'\b([0-9]+)l\b',r'\1',texts['utiltypes.cpp'])
    output.mkdir(parents=True,exist_ok=True)
    for name,text in texts.items():
        dest=output/name
        if not dest.exists() or dest.read_text()!=text: dest.write_text(text)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();prepare(a.upstream,a.output)

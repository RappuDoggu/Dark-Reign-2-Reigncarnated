#!/usr/bin/env python3
"""Stage original variable database and nibble-decoding implementations externally."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/varfile-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed variable database source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 def function(source,signature):
  a=source.index(signature);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 load=function(texts['util/varfile.cpp'],'  void Load(FScope *fScope, const char *path)')
 load=load.replace('          // Compose the new path','          if (Utils::Strlen(path)+1+Utils::Strlen(ident)>=VARSYS_MAXVARPATH) fScope->ScopeError("Variable database path too long");\n          // Compose the new path',1)
 load=load.replace('          U32 size = StdLoad::TypeU32(sScope, "Size");','          U32 size = StdLoad::TypeU32(sScope, "Size");\n          if (size>64u*1024u*1024u) sScope->ScopeError("Variable binary exceeds native resource limit");',1)
 binary=function(texts['util/stdload.cpp'],'  void TypeBinary(FScope *scope, U32 size, U8 *buffer)')
 source=texts['system/utils.cpp'];a=source.index('  const U8   translateDecode[]');b=source.index('};',a)+2;table=source[a:b]
 decode=function(source,'  void CharDecode(U8 *dst, const char *src, U32 srcSize, U32 dstSize)')
 decode=decode.replace('    ASSERT(dstSize >= srcSize / 2)', '''    if ((srcSize & 1u) || srcSize/2>dstSize || (srcSize && (!src || !dst)))
      throw std::runtime_error("Invalid binary text bounds");
    for(U32 i=0;i<srcSize;++i){unsigned char c=static_cast<unsigned char>(src[i]);
      if(!((c>='0'&&c<='9')||(c>='A'&&c<='F')))
        throw std::runtime_error("Invalid binary text digit");}
    ASSERT(dstSize >= srcSize / 2)''')
 # Validation establishes ASCII indices. Original decoder and low-nibble-first
 # order remain unchanged. Reject lowercase rather than decode it to zero.
 output.mkdir(parents=True,exist_ok=True)
 (output/'varfile_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "fscope.h"
#include "control_stdload.hpp"
namespace Utils {void CharDecode(U8 *,const char *,U32,U32);}
namespace StdLoad {void TypeBinary(FScope *,U32,U8 *);}
namespace VarFile {void Load(FScope *,const char *);}
''')
 (output/'varfile_slice.cpp').write_text('#include "varfile_slice.hpp"\nnamespace Utils {\n'+table+'\n'+decode+'\n}\nnamespace StdLoad {\n'+binary+'\n}\nnamespace VarFile {\n'+load+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

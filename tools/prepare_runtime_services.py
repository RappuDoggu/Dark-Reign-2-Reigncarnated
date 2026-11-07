#!/usr/bin/env python3
"""Stage original runtime services externally; no upstream implementation is vendored."""
import argparse,hashlib,json,re
from pathlib import Path

def prepare(upstream,out):
 repo=Path(__file__).resolve().parents[1];out=out.resolve()
 if out==repo or repo in out.parents:raise ValueError('Runtime overlay must be external')
 texts={}
 for name,pin in json.loads((repo/'cmake/runtime-source-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=pin:raise ValueError('Unreviewed runtime source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 out.mkdir(parents=True,exist_ok=True)
 notice='// Original implementation: Copyright 1997-1999 Pandemic Studios, Dark Reign II.\n'
 for stem,root in [('dtrack','system'),('tracksys','util')]:
  h=texts[f'{root}/{stem}.h'];a=h.index('\n#define __'+stem.upper()+'_H')+len('\n#define __'+stem.upper()+'_H');h=h[a:h.rindex('#endif')].replace('#include "dtrack.h"','')
  pre='#include "reign/runtime_prelude.hpp"\n'+('#include "dtrack.h"\n' if stem=='tracksys' else '')
  (out/f'{stem}.h').write_text(notice+'#pragma once\n'+pre+'namespace reign::tracking_detail {\n'+h+'\n}\n')
  cpp=texts[f'{root}/{stem}.cpp'];cpp=cpp[cpp.index(f'#include "{stem}.h"')+len(f'#include "{stem}.h"'):]
  (out/f'{stem}.cpp').write_text(notice+f'#include "{stem}.h"\nnamespace reign::tracking_detail {{\n'+cpp+'\n}\n')
 h=texts['util/event.h'];a=h.index('\n#define __EVENT_H')+len('\n#define __EVENT_H');h=h[a:h.rindex('#endif')]
 h=h.replace('#pragma pack(push, 1)','// Runtime-only Event has native pointer alignment.').replace('#pragma pack(pop)','')
 q=texts['system/queue.h'];a=q.index('template <class DATA, int SIZE> class Queue');b=q.index('// TEMPLATE SafeQueue',a);q=q[a:b]
 q=q.replace('head = NULL;','head = 0;').replace('tail = NULL;','tail = 0;')
 (out/'queue_slice.hpp').write_text(notice+'#pragma once\n#include "reign/event_prelude.hpp"\nnamespace reign::event_detail {\n'+q+'\n}\n')
 source=texts['util/event.cpp'];a=source.index('  typedef StrCrc<16> EventIdent;');b=source.index('  // System initialisation flag',a);types=source[a:b]
 body=source[source.index('  void Init()'):source.rindex('}')]
 body=body.replace('Type *e = new Type;','auto owned = std::make_unique<Type>();\n    Type *e = owned.get();').replace('    handlers.Add(id, e);','    handlers.Add(id, e);\n    owned.release();')
 for key in ['sysInit','nextEventId','queue','workQueue','handlers']:body=re.sub(r'\b'+key+r'\b','state().'+key,body)
 body=body.replace('Event state().queue overrun','Event queue overrun')
 (out/'event_slice.hpp').write_text(notice+'#pragma once\n#include "queue_slice.hpp"\nnamespace reign::event_detail {\n'+h+'\nnamespace EventSys {\n'+types+'\nstruct State {Bool sysInit=FALSE;U16 nextEventId=0;NativeSafeQueue<Event,128> queue;Queue<Event,128> workQueue;BinTree<Type> handlers;};\nState &state();\n}\n}\n')
 (out/'event_slice.cpp').write_text(notice+'#include "event_slice.hpp"\nnamespace reign::event_detail::EventSys {\n'+body+'\n}\n')
 (out/'ntree.h').write_text(texts['system/ntree.h'])
 for stem,root in [('str','system'),('varsys','util')]:
  h=texts[f'{root}/{stem}.h'];guard='__'+stem.upper()+'_H';a=h.index('\n#define '+guard)+len('\n#define '+guard);h=h[a:h.rindex('#endif')]
  for include in ['utiltypes.h','tracksys.h','str.h']:h=h.replace('#include "'+include+'"','')
  if stem=='str':h=h.replace('char buff[32];','char buff[34];')
  if stem=='varsys':h=h.replace('static char* itemTypeStrings','static const char* itemTypeStrings').replace('static char* GetTypeString','static const char* GetTypeString')
  extra='#include "str.h"\n' if stem=='varsys' else ''
  (out/f'{stem}.h').write_text(notice+'#pragma once\n#include "reign/vars_prelude.hpp"\n'+extra+'namespace reign::vars_detail {\n'+h+'\n}\n')
 for stem,root,header in [('str','system','str'),('varsys','util','varsys'),('varscope','util','varsys'),('varitem','util','varsys'),('varptr','util','varsys')]:
  cpp=texts[f'{root}/{stem}.cpp'];a=cpp.index('#include "'+header+'.h"')+len('#include "'+header+'.h"');cpp=cpp[a:]
  if stem=='str':cpp=cpp.replace('vsprintf(str.ref->data, format, args)','std::vsnprintf(str.ref->data, 512, format, args)')
  if stem=='varsys':
   cpp=cpp.replace('char* VarSys::itemTypeStrings','const char* VarSys::itemTypeStrings').replace('char* VarSys::GetTypeString','const char* VarSys::GetTypeString')
   marker='const char * VarSys::BuildVarName(const char *path, void *context)\n{'
   if cpp.count(marker)!=1:raise ValueError('Unexpected variable substitution boundary')
   cpp=cpp.replace(marker,marker+'\n  if (!path || std::strlen(path) >= VARSYS_MAXVARPATH) throw std::runtime_error("Variable path exceeds native buffer limit");')
   old='Utils::Strcpy(buf, substitution->Expand(buf, context));'
   if cpp.count(old)!=1:raise ValueError('Unexpected substitution copy')
   cpp=cpp.replace(old,'const char *expanded = substitution->Expand(buf, context);\n      if (!expanded || std::strlen(expanded)+std::strlen(dot) >= sizeof(buf)) throw std::runtime_error("Expanded variable path exceeds buffer");\n      std::memmove(buf, expanded, std::strlen(expanded)+1);')
  if stem=='varitem':
   if cpp.count('delete binary.data;')!=1:raise ValueError('Unexpected binary cleanup site')
   cpp=cpp.replace('delete binary.data;','delete [] binary.data;')
  (out/f'{stem}.cpp').write_text(notice+'#include "'+header+'.h"\nnamespace reign::vars_detail {\n'+cpp+'\n}\n')
 print('Staged original tracking, events and variables externally:',out)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

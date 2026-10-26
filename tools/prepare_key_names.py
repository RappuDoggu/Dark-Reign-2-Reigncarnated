#!/usr/bin/env python3
"""Stage original key name/modifier configuration; does not synthesize key events."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'interface/keybind.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/key-names-hashes.json').read_text())['interface/keybind.cpp']:raise ValueError('Unreviewed key configuration')
 source=raw.decode().replace('\r\n','\n')
 def extract(signature):
  a=source.index(signature);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 scan=extract('  void ProcessScanCodes(FScope *fScope)')
 scan=scan.replace('scanCode > 256','scanCode >= 256').replace('Scancode [%s] already defined','Scancode [%u] already defined')
 old='          keysByScan[scanCode] = new KeyItem(scanCode, sScope->NextArgString());'
 assert scan.count(old)==1
 scan=scan.replace(old,'''          const char *name = sScope->NextArgString();
          if (!*name || std::strlen(name)>=32 || keysByNameCrc.Find(Crc::CalcStr(name)))
            throw std::runtime_error("Invalid or duplicate scan-code name");
          auto owned = std::make_unique<KeyItem>(scanCode,name);
          keysByScan[scanCode] = owned.get();''')
 old='          keysByNameCrc.Add(keysByScan[scanCode]->name.crc, keysByScan[scanCode]);'
 scan=scan.replace(old,old+'\n          owned.release();')
 scan=scan.replace('LOG_ERR(("ScanCodes: Not Expecting [%s]", sScope->NameStr()))','throw std::runtime_error("Unknown scan-code instruction");')
 mod=extract('  void ProcessModifiers(FScope *fScope)')
 old='        modifiers.Add(Crc::CalcStr(name), new ModifierItem(name, key));'
 assert mod.count(old)==1
 mod=mod.replace(old,'''        if (!*name || std::strlen(name)>=64 || modifiers.Find(Crc::CalcStr(name)))
          throw std::runtime_error("Invalid or duplicate modifier name");
        auto owned=std::make_unique<ModifierItem>(name,key);
        modifiers.Add(Crc::CalcStr(name), owned.get());owned.release();''')
 mod=mod.replace('      if (key)','      if (!key) throw std::runtime_error("Unknown modifier type");\n      if (key)')
 bodies=[]
 for body in [scan,mod]:
  body=body.replace('    ASSERT(sysInit)','    (void)reign::key_name_detail::state();\n    if (!fScope) throw std::invalid_argument("Missing key configuration");')
  for name in ['keysByScan','keysByNameCrc','modifiers']:body=body.replace(name,'reign::key_name_detail::state().'+name)
  bodies.append(body)
 output.mkdir(parents=True,exist_ok=True)
 (output/'key_names_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "fscope.h"
#ifndef LOGEXTERN
#define LOGEXTERN
#endif
#include "control_input.hpp"
namespace reign::key_name_detail {
'''+extract('  struct KeyItem')+';\n'+extract('  struct ModifierItem')+''';
struct State {
 KeyItem *keysByScan[256]{};
 BinTree<KeyItem> keysByNameCrc;
 NBinTree<ModifierItem> modifiers{&ModifierItem::node};
 ~State(){keysByNameCrc.DisposeAll();modifiers.DisposeAll();}
};
State &state();
void ProcessScanCodes(FScope *);
void ProcessModifiers(FScope *);
}
''')
 (output/'key_names_slice.cpp').write_text('#include "key_names_slice.hpp"\nnamespace reign::key_name_detail {\n'+'\n'.join(bodies)+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

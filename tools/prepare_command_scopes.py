#!/usr/bin/env python3
"""Stage original Main configuration dispatch and scope registry externally."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'main/maincmd.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/command-scopes-hashes.json').read_text())['main/maincmd.cpp']:raise ValueError('Unreviewed command source')
 source=raw.decode().replace('\r\n','\n')
 def function(signature):
  a=source.index(signature);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 body=function('  void ScopeHandler(FScope *fScope)')
 def patch(a,b):
  nonlocal body
  if body.count(a)!=1:raise ValueError('Command dispatch anchor changed: '+a)
  body=body.replace(a,b)
 patch('void ScopeHandler(FScope *fScope)','void Dispatch(FScope *fScope)')
 patch('    ASSERT(fScope);','    if (!fScope) throw std::invalid_argument("Missing command scope");\n    (void)reign::command_detail::state();')
 patch('LOG_DIAG((fScope->NextArgString()));','LOG_DIAG(("%s", fScope->NextArgString()));')
 # Native boundaries must supply these services; none is silently dropped.
 for code,nextcode in [('0x4B85C789','0xD7FD32ED'),('0xD7FD32ED','0xC4FD8F50'),('0x4CE2B3B3','0xA5F83324'),('0xB09539F9','0xE8AC79E0'),('0xE8AC79E0','0x65C54B6A')]:
  a=body.index('      case '+code);b=body.index('      case '+nextcode,a)
  body=body[:a]+'      case '+code+':\n        reign::command_detail::forward(fScope);\n        break;\n\n'+body[b:]
 a=body.index('      case 0x65C54B6A');b=body.index('      default:',a)
 body=body[:a]+'      case 0x65C54B6A:\n        reign::command_detail::forward(fScope);\n        break;\n\n'+body[b:]
 patch('CON_ERR(("Unknown command \'%s\'", fScope->NameStr()))','throw std::runtime_error(std::string("Unregistered root scope: ")+fScope->NameStr());')
 patch('CON_ERR(("Expecting string argument"))','throw std::runtime_error("Cmd requires a string argument");')
 # Keep original default parameter semantics explicit in the staged declarations.
 body=body.replace('ProcessCmdScope(fScope);','ProcessCmdScope(fScope, ScopeHandler);').replace('ProcessCmdScope(nScope);','ProcessCmdScope(nScope, ScopeHandler);').replace('ProcessCmdScope(sScope);','ProcessCmdScope(sScope, ScopeHandler);')
 patch('Exec(fScope->NextArgString());','Exec(fScope->NextArgString(), ScopeHandler, TRUE);')
 patch('Switch::Value &value = Switch::GetValue(fScope, NULL);','Switch::Value &value = Switch::GetValue(fScope, NULL);\n        struct Cleanup {Switch::Value &v; ~Cleanup(){Switch::DisposeValue(v);}} cleanup{value};')
 patch('        Switch::DisposeValue(value);','        // Scoped cleanup also covers handler exceptions.')
 bodies=[function('  void RegisterRootScope('),function('  void UnregisterRootScope('),body]
 bodies=[s.replace('rootHandlers','reign::command_detail::state().handlers') for s in bodies]
 output.mkdir(parents=True,exist_ok=True)
 item=function('  struct RootScopeItem')
 (output/'command_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "fscope.h"
#include "reign/runcode_host.hpp"
namespace Main {
using RootScopeHandler=void(FScope *);
'''+item+''';
void RegisterRootScope(const char *,RootScopeHandler *);
void UnregisterRootScope(const char *,RootScopeHandler *);
void Dispatch(FScope *);
}
namespace reign::command_detail {
struct State {
 NBinTree<Main::RootScopeItem> handlers{&Main::RootScopeItem::node};
 std::function<void(FScope *)> service;
 unsigned depth=0;
 ~State(){handlers.DisposeAll();}
};
State &state();
void forward(FScope *);
}
''')
 (output/'command_slice.cpp').write_text('#include "command_slice.hpp"\n#include "comparison.h"\n#include "switch.h"\n#include "console_slice.hpp"\nnamespace Main {\n'+'\n'.join(bodies)+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

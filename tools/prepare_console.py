#!/usr/bin/env python3
"""Stage original console command parser and argument access, without GUI history."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'util/console.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/console-hashes.json').read_text())['util/console.cpp']:raise ValueError('Unreviewed console source')
 source=raw.decode().replace('\r\n','\n')
 def function(signature):
  a=source.index(signature)
  while ';' in source[a:source.index('{',a)]:a=source.index(signature,a+len(signature))
  b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 a=source.index('  class CmdParse');b=source.index('  // Is system initialized',a)
 parser=source[a:b]
 def patch(a,b,n=1):
  nonlocal parser
  if parser.count(a)!=n:raise ValueError('Console anchor changed: '+a)
  parser=parser.replace(a,b)
 patch('static U32 stackLevel;', 'static thread_local U32 stackLevel;')
 patch('U32 CmdParse::stackLevel = 0;', 'thread_local U32 CmdParse::stackLevel = 0;')
 patch('      stackLevel++;','      if (stackLevel >= 32) throw std::runtime_error("Console nesting limit");\n      stackLevel++;')
 patch('      stackLevel--;','      DeleteArguments();\n      stackLevel--;')
 patch('    throw (0);','    throw std::runtime_error(errStr);')
 patch('    currentCmd = cmd;','''    (void)reign::console_detail::state();
    if (!cmd || std::strlen(cmd)>65535) throw std::invalid_argument("Invalid console command length");
    struct Restore {const char *cmd; Bool handling; ~Restore(){currentCmd=cmd;inHandler=handling;}} restore{currentCmd,inHandler};
    currentCmd = cmd;''')
 patch('    catch (int) {}','    catch (...) { tBuf.Done(); throw; }')
 # Own parsed values across notification/command exceptions. Original deletion
 # sites become reset, preserving immediate destruction on successful paths.
 patch('    VNode *node;', '    VNode *node;\n    std::unique_ptr<VNode> owner;',2)
 if parser.count('    // Assign the new value')!=2:raise ValueError('Assignment anchor changed')
 parser=parser.replace('    // Assign the new value','    owner.reset(node);\n    // Assign the new value',1)
 patch('          // Create the offset var\n          VarSys::CreateInteger', '          owner.reset(node);\n          // Create the offset var\n          VarSys::CreateInteger')
 parser=parser.replace('delete node;', 'owner.reset();')
 # Original RAWDATA postincrement argument evaluation has unspecified order.
 patch('      MakeArgOffset(CmdParse::StackLevel(), argOffsetPath, argCount++);\n      VarSys::CreateInteger(argOffsetPath.str, argCount);',
       '      MakeArgOffset(CmdParse::StackLevel(), argOffsetPath, argCount++);\n      VarSys::CreateInteger(argOffsetPath.str, U32(tBuf.CurrentStr()-cmd_start()));')
 # Float-to-integer conversion must reject undefined out-of-range casts.
 parser=parser.replace('(S32)node->GetFPoint()', 'reign::console_detail::integer(node->GetFPoint())').replace('(S32)rVal->Float()', 'reign::console_detail::integer(rVal->Float())')
 funcs=[]
 for sig in ['  void MakeArgName(', '  void MakeArgCount(', '  void MakeArgOffset(', '  U32 ArgCount()',
 '  VarSys::VarItem* GetArgItem(', '  VarSys::VarItem* GetArgOffset(', '  Bool GetArg(', '  Bool GetArgInteger(',
 '  Bool GetArgString(', '  Bool GetArgFloat(', '  Bool GetArgScope(', '  const char * GetCmdString()',
 '  const char * GetCmdString(U32 index)', '  void ProcessCmd(const char *cmd, void *context)']:
  funcs.append(function(sig).replace('(U32)arg->Float()', 'reign::console_detail::integer(arg->Float())').replace('    CmdParse c;', '    (void)reign::console_detail::state();\n    CmdParse c;'))
 output.mkdir(parents=True,exist_ok=True)
 (output/'console_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "ptree.h"
#include "stdparse.h"
#include <functional>
namespace reign::console_detail {
struct State {std::function<void(VarSys::VarItem *,U32)> display;};
State &state();
S32 integer(F32);
}
namespace Console {
void ProcessCmd(const char *,void * = nullptr);
void MakeArgName(U32,VarPathIdent &,U32);
void MakeArgCount(U32,VarPathIdent &);
void MakeArgOffset(U32,VarPathIdent &,U32);
void DisplayVarItem(VarSys::VarItem *,U32);
U32 ArgCount();
Bool GetArg(U32,VarSys::VarItem *&);
Bool GetArgInteger(U32,S32 &);
Bool GetArgString(U32,const char *&);
Bool GetArgFloat(U32,F32 &);
Bool GetArgScope(U32,const VarSys::VarItem *&);
const char *GetCmdString();
const char *GetCmdString(U32);
}
namespace reign {
void run_console(const std::function<void()> &,const std::function<void(VarSys::VarItem *,U32)> &);
}
''')
 (output/'console_slice.cpp').write_text('''#include "console_slice.hpp"
#define CON_ERR(args) reign::engine_log args;
namespace Console {
static thread_local U32 consoleItemCount=0;
static thread_local Bool inHandler=FALSE;
static thread_local const char *currentCmd=nullptr;
static const char *cmd_start(){return currentCmd;}
'''+parser+'\n'+'\n'.join(funcs)+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

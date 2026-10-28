#!/usr/bin/env python3
"""Native whole-source integration workspace; never edits the pinned source tree."""
import argparse, shutil, re, hashlib, json, fcntl, subprocess, sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
root=a.upstream.resolve();out=a.output.resolve();repo=Path(__file__).resolve().parents[1]
if out==root or root in out.parents or out in root.parents or out==repo or repo in out.parents or out in repo.parents: raise ValueError('External staging required')
expected=json.loads((repo/'cmake/engine-full-source-hashes.json').read_text())
hashes={source.relative_to(root).as_posix(): hashlib.sha256(source.read_bytes()).hexdigest()
 for source in root.rglob('*') if source.is_file() and source.suffix.lower() in ['.h','.hpp','.cpp','.inl']}
digest=hashlib.sha256(json.dumps(hashes,sort_keys=True,separators=(',',':')).encode()).hexdigest()
if len(hashes)!=expected['source_files'] or digest!=expected['tree_sha256']:
 raise ValueError('Pinned original source tree differs from reviewed full-engine inputs')
lock_file=out.with_name(out.name+'.lock').open('a')
fcntl.flock(lock_file, fcntl.LOCK_EX)
for stage in ['abi','resources','missions','dx7']:
 subprocess.run([sys.executable,str(repo/'tools'/f'prepare_engine_{stage}.py'),str(root),str(root)+'-engine-'+stage],check=True)
# Preserve relative include paths and source ownership, rather than flattening
# duplicate headers or taking arbitrary files from an unreviewed search path.
for source in root.rglob('*'):
 if source.is_file() and source.suffix.lower() in ['.h','.hpp','.cpp','.inl']:
  dest=out/source.relative_to(root);dest.parent.mkdir(parents=True,exist_ok=True)
  shutil.copyfile(source,dest)
# Apply already reviewed, SHA-guarded source adaptations to the same tree.
locations={}
for source in root.rglob('*'):
 if source.is_file(): locations.setdefault(source.name,[]).append(source.relative_to(root))
for suffix in ['engine-abi','engine-resources','engine-missions']:
 for source in Path(str(root)+'-'+suffix).glob('*'):
  if source.is_file() and source.name in locations:
   matches=locations[source.name]
   if len(matches)!=1: continue
   dest=out/matches[0]
   if dest.read_bytes()!=source.read_bytes(): shutil.copyfile(source,dest)
for source in Path(str(root)+'-engine-dx7').glob('*'):
 dest=out/'3rdparty/dx7'/source.name
 if not dest.exists() or dest.read_bytes()!=source.read_bytes(): shutil.copyfile(source,dest)
(out/'system/std.h').write_text('#pragma once\n#include "engine_std.h"\n')
(out/'system/win32.h').write_text('#pragma once\n#include "dx7/windows.h"\n')
# Legacy include spelling was resolved case-insensitively on Windows.
for source in list(out.rglob('*')):
 if source.is_file() and source.name!=source.name.lower():
  alias=source.with_name(source.name.lower())
  if not alias.exists(): alias.symlink_to(source.name)

for source in out.rglob('*'):
 if source.is_file() and not source.is_symlink() and source.suffix.lower() in ['.h','.cpp','.inl']:
  raw=source.read_bytes()
  try: text=raw.decode()
  except UnicodeDecodeError: continue
  normalized=re.sub(r'(#\s*include\s*[<"])([^>"\n]+)([>"])',lambda m:m[1]+'/'.join(m[2].replace('\\','/').split('/')[:-1]+[m[2].replace('\\','/').split('/')[-1].lower()])+m[3],text)
  if normalized!=text: source.write_text(normalized)

# Graphics only owns an opaque movie handle. Actual movie calls must be adapted
# to the existing FFmpeg backend rather than linked to the x86 Bink DLL.
(out/'3rdparty/bink.h').write_text('#pragma once\nstruct BINK; using HBINK=BINK *;\n')

# Standard C++ dependent-name lookup and explicit declarations.
def patch(path, pairs):
 target=out/path
 text=target.read_text()
 for old,new in pairs:
  if old not in text: raise ValueError(f"Missing expected source in {path}: {old}")
  text=text.replace(old,new)
 target.write_text(text)
patch('util/balancetable.h', [('NBinTree<KeyInfo>::Iterator', 'typename NBinTree<KeyInfo>::Iterator')])
patch('graphics/effects_utils.h', [('    DisposeAll();','    this->DisposeAll();'),('List<DATA>::Iterator','typename List<DATA>::Iterator'),('+ count * sizeof(DATA)', '+ this->count * sizeof(DATA)')])
patch('graphics/family.h', [('    frame = _frame;', '    return frame = _frame;'),('    AnimKey::SetFrame( _frame);', '    return AnimKey::SetFrame( _frame);')])
patch('graphics/BucketDesc.h', [('class Bucket :', 'class BucketMan;\nclass Bucket :')])
patch('graphics/effects_utils.h', [('typename typename', 'typename')])
patch('graphics/camera.h', [('  Camera::Camera()', '  Camera()')])
for name,typ in [('coregame/promote.h','ReaperList<BASEOBJECT, BASENODE>'),('coregame_objects/gameobjctrl.h','ReaperList<OBJECT, NODE>'),('coregame/resolver.h','ReaperList<OBJECT, NODE>')]:
 patch(name, [(typ+'::Iterator', 'typename '+typ+'::Iterator')])
patch('coregame/resolver.h', [('ReaperList<TYPE, NODE>::Iterator','typename ReaperList<TYPE, NODE>::Iterator')])
patch('coregame/terraingroup.h', [('Type::BlendName','BlendName')])
patch('coregame/team.h', [('Team::GetConsoleId', 'GetConsoleId')])
patch('graphics/tranbucket.h', [('TranBucketMan::SetMaxZ','SetMaxZ'),('TranBucketMan::SetMinZ','SetMinZ'),('TranBucketMan::SetMaxBucketCount','SetMaxBucketCount')])
patch('interface/iftypes.h', [('sizeof TextureSkin','sizeof(TextureSkin)')])
patch('interface/icontrol.h', [('class IControl;', 'class IControl;\nclass IFaceVar;')])
# These alternatives are runtime-only fields, serialized individually, not a
# disk layout. Separate storage avoids the nonstandard anonymous Vector union.
patch('coregame_objects/bookmarkobj.h', [('  union\n  {\n    struct\n    {\n      // Position', '  /* Native storage: each field retains its original name. */\n      // Position'),('      F32     pitch;\n    };\n    struct\n    {', '      F32     pitch;'),('      Curve *focus;\n    };\n  };', '      Curve *focus;')])
patch('coregame_objects/unitobjlist.h', [('ReaperList<DATA, NODE>::Iterator','typename ReaperList<DATA, NODE>::Iterator')])
patch('coregame_ai/strategic_weighting.h', [('::Node typename DATA::*','::Node DATA::*')])
patch('util/stdload.h', [('fScope ? TypePoint<TYPE>(fScope, val) : val = dVal;', 'if (fScope) TypePoint<TYPE>(fScope, val); else val = dVal;')])
patch('coregame_tasks/tasks_spyidle.h', [('friend class SpyObj;', 'friend class ::SpyObj;')])
patch('coregame/condition_game.cpp', [('/*template <>*/ VarVar', '/*template <> */VarVar')])
for cls in ['VarConst','VarVar']:
 for typ in ['F32','U32']:
  patch('coregame/condition_game.cpp', [(f'/*template <> */{cls}<{typ}>::{cls}<{typ}>', f'template <> {cls}<{typ}>::{cls}')])
patch('coregame/movement_pathfollow.cpp', [('sizeof PathPoint', 'sizeof(PathPoint)')])
# On LP64, long literals otherwise ambiguously select float or 32-bit overloads.
patch('util/color.h', [('  inline Color(U32 rIn,', '  template<class R, class G, class B, class A=int, std::enable_if_t<std::is_integral_v<R> && std::is_integral_v<G> && std::is_integral_v<B> && std::is_integral_v<A>,int> = 0>\n  inline Color(R rIn, G gIn, B bIn, A aIn = 255)\n  { SetInline(S32(rIn), S32(gIn), S32(bIn), S32(aIn)); }\n  inline Color(U32 rIn,')])
patch('coregame_particles/particle.h', [('0L, 255L','S32(0), S32(255)')])
patch('coregame_objects/mapobj.cpp', [('sizeof Matrix','sizeof(Matrix)')])
patch('graphics/mesh.h', [('#include "vid.h"', 'class MeshRoot;\n#include "vid.h"')])
patch('coregame_objects/gameobj.h', [('#define __GAMEOBJ_H', '#define __GAMEOBJ_H\nclass MeshEnt;')])
patch('coregame_tasks/tasks_unitconstructor.cpp', [('U32(context)', 'reinterpret_cast<std::uintptr_t>(context)')])
patch('game/trackplayer.cpp', [('"TP::%08X", U32(this)', '"TP::%p", static_cast<void *>(this)')])
for source in (out/'coregame_tasks').glob('tasks_*.cpp'):
 text=source.read_text()
 cls=re.search(r'StateMachine<(\w+)>',text)
 if cls:
  text=re.sub(r'(stateMachine.AddState\([^\n]*,\s*)&(\w+)\)',lambda m:m[1]+'&'+cls[1]+'::'+m[2]+')',text)
 text=re.sub(r'(?m)^(GameTask<[^>]+>::~GameTask\(|void GameTask<[^>]+>::GameProcess\()',r'template <> \1',text)
 source.write_text(text)
patch('system/queue.h', [('#include "system.h"','#include "system_sync.h"')])
patch('coregame_ai/strategic_weighting.h', [('NBinTree<DATA, KEY>::Iterator','typename NBinTree<DATA, KEY>::Iterator')])
# All dynamic parameters at these two sites are UTF-16 string pointers. Pass
# those pointers using the native variadic ABI instead of x86 stack injection.
for name in ['coregame/displayobjective.cpp','coregame/radio.cpp']:
 source=out/name;text=source.read_text()
 text=text.replace('U32 params[10];','const CH *params[10] = {};').replace('U32 *ptr = params;', 'const CH **ptr = params;')
 text=text.replace('reinterpret_cast<U32>', 'reinterpret_cast<const CH *>')
 text=re.sub(r'    // Push the parameters onto the stack.*?(?=    // Translate)', '',text,flags=re.S) if name.endswith('displayobjective.cpp') else text
 text=re.sub(r'          // Push the parameters onto the stack.*?(?=          switch \(message->type.crc\))', '',text,flags=re.S)
 text=re.sub(r'    // Pop params off the stack.*?(?=    return \(text\);)', '',text,flags=re.S) if name.endswith('displayobjective.cpp') else text
 text=re.sub(r'          // Pop params off the stack.*?\n          }\n', '',text,flags=re.S)
 args=', '.join(f'params[{i}]' for i in range(10))
 if name.endswith('displayobjective.cpp'):
  text=text.replace('TRANSLATE((key, numParam))',f'TRANSLATE((key, numParam, {args}))')
  text=text.replace('          const char *p = StdLoad::TypeString(sScope);', '          if (numParam >= 10) { ERR_CONFIG(("Too many objective text parameters")); }\n          const char *p = StdLoad::TypeString(sScope);')
 else:
  text=text.replace('numParam);', f'numParam, {args});').replace('                  numParam\n',f'                  numParam, {args}\n')
  text=text.replace('(const CH *)"Someone"','(const CH *)u"Someone"')
 source.write_text(text)
patch('coregame/claim.cpp', [('UnitObj *Claim::GetOwner','UnitObj *GetOwner')])
patch('game/savegame.cpp', [('  static enum\n','  enum\n')])
patch('coregame_objects/mapobj.cpp', [('sizeof Vector','sizeof(Vector)'),('sizeof Quaternion','sizeof(Quaternion)')])
source=out/'game/sync.cpp';text=source.read_text()
start=text.index('    U32 buffer[2];',text.index('  void Caller()'))
end=text.index('\n  }',start)
text=text[:start]+'''    std::uintptr_t frames[4];
    const auto count = NativeEngine::CaptureStack(frames, 4);
    if (count > 2) stream << "caller@" << std::hex << frames[2] << std::dec << endl;
    SyncEnd();'''+text[end:]
start=text.index('    U32   callStack[64];',text.index('  void DumpCallStack('))
end=text.index('\n  }',start)
text=text[:start]+'''    std::uintptr_t frames[64];
    const auto count = NativeEngine::CaptureStack(frames, 64);
    Set(_file, _line);
    for (std::size_t i = 0; i < count; ++i) {
      stream << "frame@" << std::hex << frames[i] << std::dec;
      SyncEnd();
    }'''+text[end:]
source.write_text('#include "native_stack.h"\n'+text)
patch('coregame_ai/strategic_weighting.h', [('typename typename','typename')])
source=out/'system/debug_memory.h';text=source.read_text()
start=text.index('    class Aligning\n');end=text.index('    };',start)+len('    };')
text=text[:start]+text[end:]
source.write_text('#include "aligned_memory.h"\n'+text)
source=out/'util/stdload.h';source.write_text('#include "type_ident_registry.h"\n'+source.read_text())
patch('coregame/resolver.h', [('GameIdent * ident = reinterpret_cast<GameIdent *>(ptr.GetResolveId());','const GameIdent ident = NativeEngine::TakeTypeIdent(ptr.GetResolveId());'),('      VALIDATE(ident);','      // TakeTypeIdent validates the token before use.'),('Resolver::Type<TYPE>(ptr, *ident, required);','Resolver::Type<TYPE>(ptr, ident, required);'),('      delete ident;','      // The local value releases its storage automatically.')])
source=out/'coregame/resolver.h';source.write_text('#include "type_ident_registry.h"\n'+source.read_text())
patch('coregame_ai/strategic_weighting.h', [('          Cuts<DATA>::Item *ii', '          typename Cuts<DATA>::Item *ii')])
# Defer member lookup until concrete game object definitions are available.
patch('coregame/promote.h', [('template <class TYPE> TYPE * Type(GameObjType *type)', 'template <class TYPE> TYPE * Type(typename CompleteBase<TYPE, GameObjType>::Type *type)'),('template <class TYPE, class OBJECT> OBJECT * Object(GameObj *obj)', 'template <class TYPE, class OBJECT> OBJECT * Object(typename CompleteBase<TYPE, GameObj>::Type *obj)')])
patch('coregame_tasks/task.h', [('  // Initializing Constructor\n  GameTask', '''  template<class BASE> static void CannotPromote(BASE *obj) {
    ERR_FATAL(("Unable to promote [%d] %s", obj->Id(), obj->GameType()->GetName()))
  }
  // Initializing Constructor
  GameTask'''),('      ERR_FATAL(("Unable to promote [%d] %s", obj->Id(), obj->GameType()->GetName()))', '      CannotPromote(obj);')])
patch('coregame/game.cpp', [('  void PreOpenMission(const char *missionExecFile)\n  {', '  void PreOpenMission(const char *missionExecFile)\n  {\n    NativeEngine::ClearTypeIdents();'),('  void Done()\n  {', '  void Done()\n  {\n    NativeEngine::ClearTypeIdents();')])

patch('coregame/promote.h', [('namespace Promote\n{', 'namespace Promote\n{\n  template<class DEPENDENCY, class BASE> struct CompleteBase { using Type = BASE; };')])

patch('coregame/movement_pathfollow.cpp', [('item->unit[0]->UnitType()->GetHitPoints(),','U32(item->unit[0]->UnitType()->GetHitPoints()),'),('item->unit[1]->UnitType()->GetHitPoints()\n','U32(item->unit[1]->UnitType()->GetHitPoints())\n')])

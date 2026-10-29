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
for stage in ['abi','resources','missions','dx7','clock','events']:
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
for suffix in ['engine-abi','engine-resources','engine-missions','engine-clock','engine-events']:
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
(out/'3rdparty/bink.h').write_text('#pragma once\n#include "bink_native.h"\n')

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

# UI controls retain original scan-code values, without a DirectInput device.
keys='\n'.join(line for line in (root/'3rdparty/dx7/dinput.h').read_text().splitlines() if re.match(r'#define DIK_',line))
(out/'system/native_keycodes.h').write_text('#pragma once\n'+keys+'\n')
patch('util/color.h', [('  void Set( S32 rIn,', '''  template<class R, class G, class B, class A=int,
    std::enable_if_t<std::is_integral_v<R> && std::is_integral_v<G> && std::is_integral_v<B> && std::is_integral_v<A>,int> = 0>
  void Set(R rIn, G gIn, B bIn, A aIn = 255) { Set(U32(rIn), U32(gIn), U32(bIn), U32(aIn)); }
  void Set( S32 rIn,''')])
patch('interface/icdroplist.cpp', [('controlStyle |= TRANSPARENT;', 'controlStyle |= STYLE_TRANSPARENT;')])
patch('graphics/godfile.h', [('sizeof T','sizeof(T)')])
patch('graphics/bitmap_manager.cpp', [('char * memtype', 'const char * memtype')])
patch('coregame_interface/client_hud.cpp', [('sizeof ReticleProfile','sizeof(ReticleProfile)')])
patch('coregame_objects/mapobjlist.h', [('ReaperList<DATA, NODE>::Iterator','typename ReaperList<DATA, NODE>::Iterator')])
patch('interface/icontrol.cpp', [('alignTo->parent ? alignTo->parent : alignTo;', 'alignTo->parent ? alignTo->parent : static_cast<IControl *>(alignTo);')])
patch('interface/iclistslider.cpp', [('0L)', 'S32(0))')])
patch('interface/iface.cpp', [('sizeof ColorGroup', 'sizeof(ColorGroup)')])
patch('interface/coloreditor.cpp', [('color2.B(), 0.0f)', 'color2.B(), U32(0))')])
for name in ['graphics/mesh_render_vertextl.cpp','graphics/meshent_render_vertextl.cpp']:
 patch(name, [('Vid::renderState.texShadowSize, TRUE, 0, 0)', 'Vid::renderState.texShadowSize, TRUE, 0, U32(0))')])
for name in ['graphics/mesh.cpp','graphics/meshman.cpp','graphics/meshread.cpp']:
 patch(name, [('strnicmp(', 'Utils::Strnicmp(')])
patch('system/debug.h', [('#ifdef DEVELOPMENT\n  #define STATIC_GUARD_BLOCK_ENABLED\n#endif', '// Native builds use compiler sanitizers instead of Win32 static guards.')])
source=out/'interface/iface_util.cpp';text=source.read_text()
text=text.replace('len = Utils::Strlen(itoa(value, buf, 10));', 'Utils::Sprintf(buf, sizeof(buf), "%d", value);\n    len = Utils::Strlen(buf);')
start=text.index('    // Convert S32 to a string',text.index('  void RenderF32('));end=text.index('    // Draw the text',start)
text=text[:start]+'''    // F32 needs at most 39 integral digits. Keep precision bounded and
    // preserve negative values without the Windows fcvt static buffer.
    char buf[64];
    Utils::Sprintf(buf, sizeof(buf), "%.*f", int(Min<U32>(decPt, 9)), double(value));
    len = Utils::Strlen(buf);

'''+text[end:]
text=text.replace('    char *str;\n    U32 len;', '    U32 len;')
source.write_text(text)
patch('coregame_interface/common_mapwindow.cpp', [('sizeof VertexTL','sizeof(VertexTL)')])
patch('graphics/lex.h', [('item->name; list++','item->name; item++'),('stricmp(', 'Utils::Stricmp('),('return _END_OF_FILE;', 'TokenItem *end = tokList; while (end->name) ++end; return end->token;')])
patch('graphics/meshread.cpp', [('min(', 'Min('), ('Min(srcVI.count, MAXWEIGHTS)', 'Min<U32>(srcVI.count, MAXWEIGHTS)')])
patch('graphics/vid_cmd_dialog.cpp', [('min(', 'Min('),('max(', 'Max(')])
patch('graphics/vid_cmd.cpp', [('caps.clipGuard ? Vid::Var::clipGuard : FALSE', 'caps.clipGuard ? S32(Vid::Var::clipGuard) : FALSE'),('Color color = Vid::Var::suncolor;', 'Color color{U32(Vid::Var::suncolor)};'),('min(', 'Min('), ('max(', 'Max(')])
patch('graphics/vid_cmd_mesh.cpp', [('Color color = Var::baseColor;', 'Color color{U32(Var::baseColor)};')])
# Console callback keys are addresses, unlike the 32-bit message IDs.
patch('util/console.cpp', [('BinTree<HookItem>', 'BinTree<HookItem, std::uintptr_t>'),('U32(context)', 'reinterpret_cast<std::uintptr_t>(context)'),('id = (U32)context;', 'id = U32(reinterpret_cast<std::uintptr_t>(context));'),('vswprintf((wchar_t *)fmtBuf, (const wchar_t *)format, args);', 'NativeEngine::VFormat16(fmtBuf, MAX_TEXTBUF, format, args);'),('strnicmp(', 'Utils::Strnicmp(')])
source=out/'util/console.cpp';source.write_text('#include "text_native.h"\n'+source.read_text())
patch('util/perfstats.cpp', [('return S32(p1->parent) - S32(p2->parent);', 'const auto a = reinterpret_cast<std::uintptr_t>(p1->parent);\n    const auto b = reinterpret_cast<std::uintptr_t>(p2->parent);\n    return (a > b) - (a < b);'),('stricmp(', 'Utils::Stricmp('),('WRITE(itoa(rec, ibuf, 10));', 'Utils::Sprintf(ibuf, sizeof(ibuf), "%u", rec);\n        WRITE(ibuf);'),('WRITE(itoa(n, ibuf, 10));','Utils::Sprintf(ibuf, sizeof(ibuf), "%u", n);\n          WRITE(ibuf);'),('hwSupport = Hardware::CPU::HasFeature(Hardware::CPU::TSC);', 'hwSupport = TRUE; // Native Clock provides monotonic profiling; no x86 TSC needed.')])
# Native wchar_t is 32-bit. Preserve Windows UTF-16 literal code units with
# separately owned CH arrays, not reinterpret_casts into char16_t storage.
for source in out.rglob('*'):
 if source.is_file() and not source.is_symlink() and source.suffix.lower() in ['.cpp','.h','.inl']:
  try:text=source.read_text()
  except UnicodeDecodeError:continue
  text=re.sub(r'\(\s*const\s+(?:CH|U16)\s*\*\s*\)\s*L("(?:\\.|[^"\\])*?")',lambda m:'REIGN_TEXT16('+m[1]+')',text)
  source.write_text(text)
# AI orders already carry U32 IDs. Use existing lifecycle IDs rather than
# truncating addresses; token IDs are unique within the owning base manager.
patch('coregame_ai/strategic_script_state_actions.cpp', [('U32(&script)', 'script.dTrack.id')])
patch('coregame_ai/strategic_script_manager.cpp', [('U32(*s)', '(*s)->dTrack.id'),('// The ID is the pointer to a script', '// The ID is the script lifecycle ID, not its address')])
patch('coregame_ai/strategic_placement.cpp', [('U32(&token)', 'token.GetId()')])
patch('system/weightedlist.h', [('    Append(', '    this->Append('),('!GetCount()', '!this->GetCount()'),('for (Iterator i(this);', 'for (typename List<WeightedListNode<DATA>>::Iterator i(this);')])
source=out/'coregame_ai/strategic_asset_manager.cpp';text=source.read_text();text=re.sub(r'(stateMachine.AddState\([^\n]*,\s*)&(\w+)\)',r'\1&Asset::Manager::\2)',text);source.write_text(text)
# Preserve the original profiler declaration graph. Its desktop implementation
# remains unavailable; Main configuration dispatch needs declarations only.
patch('system/profile.h', [('#include "system.h"', '#include "system_sync.h"'),('#include <commctrl.h>', '// Native HWND/HTREEITEM declarations come from the platform prelude.')])
patch('main/maincmd.cpp', [('DebugBreak();', 'std::raise(SIGTRAP);')])
source=out/'main/maincmd.cpp';source.write_text('#include <csignal>\n#include "debug_memory.h"\n'+source.read_text())

# The original release path disables the external COM mesh simplifier. Its
# conversion helper must follow the same feature guard as its only caller.
patch('graphics/meshmrm.cpp', [('#pragma comment(lib, "ole32")', '// COM simplification is disabled in native release builds.'), ('MRMGenParams params;', '#ifdef DOMRMGEN\nMRMGenParams params;'), ('Bool MeshRoot::MRMGen( List<U16> * verts)', '#endif\nBool MeshRoot::MRMGen( List<U16> * verts)')])

# Retain the original Debug API with a shared, fixed-width native declaration.
source=out/'system/debug.h';text=source.read_text();start=text.index('    enum Type',text.index('namespace Error'));end=text.index('    // Used by the macro',start)
text=text[:start]+text[end:];source.write_text('#include "debug_native.h"\n'+text)
patch('graphics/bitmapprimitive.cpp', [('FIXME(919378333, "aiarossi");', '// Original note: LineX draws one extra pixel (1999-02-18).'), ('wsprintf( name.str,', 'Utils::Sprintf( name.str, sizeof(name.str),')])

# Keep original human-readable DirectX diagnostics without importing device APIs.
# Numeric DS/DI expressions come from the already hash-verified SDK source tree.
error_text='#pragma once\n#include "dx7/errors.h"\n'
for sdk,names in [('dsound.h',None),('dinput.h',set(re.findall(r'case\s+(DI\w+)\s*:',(root/'graphics/viderror.cpp').read_text())))]:
 text=(root/'3rdparty/dx7'/sdk).read_text().replace('\\\n','')
 for line in text.splitlines():
  match=re.match(r'#define\s+(\w+)',line)
  if match and ((names is None and (match[1].startswith('DSERR_') or match[1] in ['_FACDS','MAKE_DSHRESULT'])) or (names is not None and match[1] in names)):
   error_text+=line+'\n'
(out/'3rdparty/native_dx_errors.h').write_text(error_text)
source=out/'graphics/viderror.cpp';source.write_text('#include "native_dx_errors.h"\n'+source.read_text())

patch('graphics/bitmap.cpp', [('BinkOpen( (char *) binkFile->GetMemoryPtr(), BINKFROMMEMORY)', 'NativeEngine::OpenMovie(binkFile->GetMemoryPtr(), binkFile->Size())'), ('min(', 'Min('), ('register ', ''), ('strnicmp(', 'Utils::Strnicmp('), ('Sleep(1);', 'std::this_thread::sleep_for(std::chrono::milliseconds(1));')])
source=out/'graphics/bitmap.cpp';source.write_text('#include <thread>\n#include <chrono>\n'+source.read_text())

patch('graphics/bitmap.cpp', [('MAKEWORD(buf[0], buf[1])','U16(U16(buf[0]) | (U16(buf[1]) << 8))'), ('Min(bmpWidth, fileHdr.imageWidth)', 'Min<S32>(bmpWidth, fileHdr.imageWidth)'), ('Min(bmpWidth, fileHeader.width)', 'Min<S32>(bmpWidth, fileHeader.width)')])
source=out/'graphics/bitmap.cpp';text=source.read_text();start=text.index('  const FOURCC ckidData',text.index('Bool Palette::ReadMSPal'));end=text.index('\n  // Generate CLUT if needed',start)
text=text[:start]+'''  auto file = std::unique_ptr<FileSys::DataFile, void (*)(FileSys::DataFile *)>(FileSys::Open(filename), FileSys::Close);
  if (!file) return FALSE;
  std::array<PALETTEENTRY, 256> colors;
  if (!NativeEngine::DecodeRiffPalette(file->GetMemoryPtr(), file->Size(), colors)) return FALSE;
  Utils::Memcpy(pal, colors.data(), sizeof(pal));
'''+text[end:]
source.write_text('#include "palette_native.h"\n'+text)

# Original Bitmap operations retain the DX7 surface interface, now backed by
# native pixel storage. Window/device creation remains a separate renderer task.
patch('graphics/bitmap.cpp', [('Vid::ddx->CreateSurface( &desc, &surface, NULL)', 'NativeEngine::CreateMemorySurface(desc, &surface)')])
source=out/'graphics/bitmap.cpp';source.write_text('#include "surface_native.h"\n'+source.read_text())

# Split portable setup/renderer ownership from Windows process/device creation.
# Unported entry points stay undefined; they are not replaced with success stubs.
source=out/'main/setup.cpp';text=source.read_text();text=text[:text.index('  Bool FindOriginalCD()')]+'}\n';source.write_text(text)
source=out/'graphics/vid.cpp';text=source.read_text()
def original_function(text, signature):
 start=text.index(signature);opening=text.index('{',start);depth=1;end=opening+1
 while depth:
  if text[end]=='{':depth+=1
  elif text[end]=='}':depth-=1
  end+=1
 return text[start:end]+'\n'
state=text[text.index('  U8   startVar;'):text.index('  // wipe data members')]
state=re.sub(r'^.*\b(?:HFONT|HMENU)\b.*\n','',state,flags=re.M)
state=state.replace('Vid::isStatus','isStatus').replace('Vid::renderState','renderState')
resources='  extern U32 clipScreenCounter;\n'+''.join(original_function(text,sig) for sig in ['  void InitResources(', '  void DisposeAll()', '  void CDECL LogPerf('])
resources=resources.replace('    first = FALSE;', '    if (!fp) { LOG_WARN(("Unable to open performance log")); return; }\n    first = FALSE;')
callback=original_function(text,'  MODECHANGEPROC * SetModeChangeCallback(')
formats=original_function((out/'graphics/vid_enumdx.cpp').read_text(),'  Pix & PixForm(')
formats=formats.replace('    return i <', '    if (!pixFormatList.GetCount()) throw std::logic_error("Native video formats are not initialized");\n    return i <')
device_source=(out/'graphics/vid_enumdx.cpp').read_text()
caps_code=original_function(device_source,'  void DriverD3D::SetDesc(')+original_function(device_source,'  void DriverD3D::SetCaps()').replace('    Config::Setup();','    // Native backend reports its own caps; no Windows driver-quirk override.')
source.write_text('#include "light_priv.h"\n#include "mesh.h"\n#include "vid_private.h"\n#include "vid_native.h"\n#include "device_gles.h"\n#include "surface_native.h"\n#include <memory>\n#include <vector>\n#include <stdexcept>\nnamespace Vid {\n'+state+callback+formats+caps_code+resources+'\n#include "vid_native.inc"\n}\n')
# Original bug: clearing sizeof(this) only clears a pointer-sized prefix.
patch('graphics/vid_decl.h', [('sizeof( this)', 'sizeof( *this)')])

# Keep session checksums and the original local key/index store independently
# of Windows sockets. Network/host/session restoration entry points stay absent.
source=out/'multiplayer/multiplayer.cpp';text=source.read_text()
source.write_text('#include "multiplayer.h"\nnamespace MultiPlayer {\n  static U32 dataCrc;\n'+original_function(text,'  U32 GetDataCrc()')+original_function(text,'  void SetDataCrc(')+'}\n')
source=out/'styxnet/styxnet_data.h';text=source.read_text();source.write_text('namespace Win32 { class Socket; }\n'+text)
source=out/'styxnet/styxnet_data.cpp';text=source.read_text();text=text[:text.index('  void Data::SendAll(')]+'}\n'
for header in ['styxnet_private.h','styxnet_packet.h','styxnet_servermessage.h']:
 text=text.replace('#include "'+header+'"','')
text=text.replace('delete data;', 'delete[] data;')
text=text.replace('LDIAG("Attempt to set host data by non-host")','LOG_DIAG(("Attempt to set host data by non-host"));').replace('LDIAG("Attempt to clear host data by non-host")','LOG_DIAG(("Attempt to clear host data by non-host"));')
text=re.sub(r'LDIAG\("([^"\n]*)"\)',r'LOG_DIAG(("\1"));',text)
text=text.replace('  Data::~Data()\n  {\n  }','  Data::~Data()\n  {\n    Flush();\n  }')
old_set=original_function(text,'    void Set(')
text=text.replace(old_set,'    void Set(U32 _length, const U8 *_data)\n    {\n      if (_length && !_data) throw std::invalid_argument("Null session data");\n      auto fresh = std::make_unique<U8[]>(_length);\n      if (_length) Utils::Memcpy(fresh.get(), _data, _length);\n      delete[] data;\n      data = fresh.release();\n      length = _length;\n    }\n')
text=text.replace('U8 *data = new U8[length];','auto data = std::make_unique<U8[]>(length);').replace('TypeBinary(sScope, length, data);','TypeBinary(sScope, length, data.get());').replace('Store(key, index, length, data, host);','Store(key, index, length, data.get(), host);').replace('          delete[] data;','          // Temporary buffer is owned by unique_ptr.')
source.write_text('#include <memory>\n#include <stdexcept>\n'+text)
source=out/'multiplayer/multiplayer_data.cpp';text=source.read_text()
functions=['    void Save(','    void Load(','    Bool HaveData()','    Bool Get(','    void LocalClear(','    void LocalFlush()']
text=text.replace('return (data.IsEmpty() ? FALSE : TRUE);','return (NativeEngine::CurrentOfflineSession() || !data.IsEmpty()) ? TRUE : FALSE;')
source.write_text('#include "session_native.h"\n#include "multiplayer_data.h"\n#include "styxnet_data.h"\nnamespace MultiPlayer { namespace Data {\nstatic StyxNet::Data data;\n'+''.join(original_function(text,s) for s in functions)+'}}\n')


# Preserve original Effect/Record/Reserved policy with bounded native PCM handles.
(out/'3rdparty/mss.h').write_text('#pragma once\n#include "miles_native.h"\n')
patch('sound/sound.cpp', [(r'      AIL_set_redist_directory("library\\mss");','      // Native audio has no external redistributable directory.'),('      AIL_startup();','      // Android output is opened explicitly by the application.'),('      AIL_shutdown();','      // Output lifetime belongs to the native application.'),('return (AIL_last_error());','return (NativeEngine::DigitalLastError());')])
patch('sound/sound_digital_reserved.cpp', [('AIL_set_sample_file(handle, d->GetMemoryPtr(), 0)','NativeSetSampleFile(handle, d->GetMemoryPtr(), d->Size())')])
patch('sound/sound_digital_effect.cpp', [('    Effect::~Effect()\n    {','    Effect::~Effect()\n    {\n      if (Sound::Initialized()) Output::StopByEffect(this);')])
(out/'sound/sound_digital.cpp').write_text('#include "sound_native.inc"\n')
# Explicit local-only session target. Multiplayer saves fail before changing
# state; transport, lobby and remote players are not represented as working.
source=out/'multiplayer/multiplayer.cpp';source.write_text(source.read_text()+'\n#include "session_native.inc"\n')

# The monochrome developer display is not part of native Android builds.
patch('coregame_interface/client_debug.cpp', [('      Weapon::Object::SetupMono(monoClientInfo2);', '#ifndef MONO_DISABLED\n      Weapon::Object::SetupMono(monoClientInfo2);\n#endif')])

# Native process metadata and original frame variables, with explicit VarSys setup.
(out/'main/maininit.cpp').write_text('#include "runtime_setup_native.inc"\n')

# Preserve original edit control; use an application-local clipboard until the
# Android clipboard bridge is connected. No desktop clipboard API is emulated.
source=out/'interface/icedit.cpp';text=source.read_text()
text=text.replace('Max(pos, 0L)','Max(pos, S32(0))').replace('Max(caretPos-1, (long)0)','Max(caretPos-1, S32(0))').replace('editMax - editLen - 1L','editMax - editLen - S32(1)')
copy=original_function(text,'void ICEdit::ClipboardCopy(')
paste=original_function(text,'void ICEdit::ClipboardPaste()')
text=text.replace(copy,'''void ICEdit::ClipboardCopy(Bool cut) {
 if (!selected) return;
 const S32 first=Min(selectX0,selectX1),last=Max(selectX0,selectX1);
 if(first<0||last>editLen||first>=last)throw std::logic_error("Invalid edit selection");
 nativeEditClipboard.assign(displayBuf+first,std::size_t(last-first));
 if(cut)ReplaceSelectedText(NULL);
}
''').replace(paste,'''void ICEdit::ClipboardPaste() {
 if(nativeEditClipboard.empty())return;
 ReplaceSelectedText(NULL);
 const S32 inserted=InsertText(caretPos,nativeEditClipboard.c_str(),U32(nativeEditClipboard.size()));
 SetCaretPos(caretPos+inserted);
}
''')
source.write_text('#include <string>\nnamespace {std::string nativeEditClipboard;}\n'+text)
patch('graphics/mesheffect_system.cpp',[('U32 MeshEffectSystem::Report','U32 Report')])
# Stream API is implemented by sound_native.inc; exclude the old Miles owner.
(out/'sound/sound_digital_stream.cpp').write_text('// Native stream ownership lives in sound_digital.cpp.\n')
# Binary variables own byte arrays, including the language database payloads.
patch('util/varitem.cpp',[('delete binary.data;', 'delete[] binary.data;')])
# Default colors are copied during the next interface startup; clear the owner
# pointer when disposing its tree to prevent reinitialization reading freed data.
patch('interface/iface.cpp',[('    stdColorGroups.DisposeAll();','    stdColorGroups.DisposeAll();\n    data.cgDefault = NULL;')])

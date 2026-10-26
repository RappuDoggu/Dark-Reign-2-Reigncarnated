#!/usr/bin/env python3
"""Hash-stage original UI color and skin configuration, outside the repository."""
import argparse,hashlib,json,re
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/frontend-theme-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed theme source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 def function(source,name):
  m=re.search(r'^  (?:static )?[^\n;]*\b'+name+r'\([^\n]*\)\s*\n  \{',source,re.M)
  if not m:raise ValueError('Missing definition '+name)
  a=m.start();b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b].replace('  static ','  ',1)
 declarations=[];bodies=[]
 for src,names in [('interface/iface.cpp',['GetColor','FindColorGroup','FindTextureSkin','CreateColorGroup','CreateSystemColorGroup','CreateTextureSkin','InitDefaultColorGroups','InitColors','ConfigureColors','ProcessCreateColorGroup','ProcessCreateSkin','ConfigureDefaultConsoleColor','ConfigureConsoleColors','GetConsoleColor']),('interface/iface_util.cpp',['FScopeToColor','LoadPiece','LoadSkinState','FScopeToTextureSkin'])]:
  for name in names:
   body=function(texts[src],name)
   declarations.append(body[:body.index('{')].strip()+';')
   fields={'consoleColors':'consoleColors','defaultConsoleColor':'defaultConsoleColor','data':'data','colorTable':'colors','colorTableCrc':'keys','stdColorGroups':'standard','customColorGroups':'custom','textureSkins':'skins'}
   token=r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\b(?:'+'|'.join(fields)+r')\b'
   body=re.sub(token,lambda m:'reign::theme_detail::state().'+fields[m[0]] if m[0] in fields else m[0],body)
   body=body.replace('sizeof ColorGroup', 'sizeof(ColorGroup)')
   bodies.append(re.sub(r'\b(\d+)[Ll]\b',r'S32(\1)',body))
 source=texts['util/utiltypes.cpp'];a=source.index('Color Color::Std[16]');b=source.index('};',a)+2
 standard=re.sub(r'\b(\d+)[Ll]\b',r'S32(\1)',source[a:b])
 a=source.index('void Color::Set(S32 rIn, S32 gIn, S32 bIn, S32 aIn)')
 b=source.index('{',a)+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 standard+='\n'+source[a:b]

 # Original TypeArea uses named TypePoint overloads; the staged scalar loader
 # exposes the same operation via GetFunction and its direct-scope overload.
 output.mkdir(parents=True,exist_ok=True)
 (output/'theme_slice.hpp').write_text('''#pragma once
#include "reign/frontend_prelude.hpp"
#include "fscope.h"
#include "control_stdload.hpp"
namespace reign::theme_detail {
struct State {
 IFace::SystemData data{};Color colors[IFace::MAX_COLORS]{};U32 keys[IFace::MAX_COLORS]{};
 BinTree<ColorGroup> standard,custom;BinTree<TextureSkin> skins;Color defaultConsoleColor{};BinTree<Color> consoleColors;
 State(){data.alphaScale=1;}
 ~State(){consoleColors.DisposeAll();skins.DisposeAll();custom.DisposeAll();standard.DisposeAll();}
};
State &state();
}
namespace StdLoad {template<class T> void TypeArea(FScope *s,Area<T> &value){TypePoint<T>(s->GetFunction("Point1"),value.p0);TypePoint<T>(s->GetFunction("Point2"),value.p1);}}
namespace IFace {
void FScopeToTextureInfo(FScope *,TextureInfo &);
'''+ '\n'.join(declarations)+'\n}\n')
 (output/'theme_slice.cpp').write_text('#include "theme_slice.hpp"\n'+standard+'\nnamespace IFace {\n'+'\n'.join(bodies)+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

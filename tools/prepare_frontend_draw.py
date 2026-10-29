#!/usr/bin/env python3
"""Stage reviewed original UI triangle generation outside the repository."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External source staging required')
 for name,digest in json.loads((repo/'cmake/frontend-draw-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed draw source: '+name)
 source=raw.decode().replace('\r\n','\n')
 import re
 def function(signature):
  a=source.index(signature);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 bodies=[]
 signatures=re.findall(r'^\s*(?:void RenderTriangle|void RenderRectangle|void RenderGradient|void RenderShadow|U32 AdjustUp|U32 AdjustDown)\([^\n]*',source,re.M)
 # Three gradient overloads: two-color, adjusted-color, and four-corner colors.
 if len(signatures)!=8:raise ValueError('Unexpected primitive signatures')
 for signature in signatures:
  body=function(signature)
  if 'void RenderRectangle' in signature:
   a=body.index('    #ifdef DEVELOPMENT');b=body.index('    // Don',a)
   body=body[:a]+body[b:]
   a=body.index('    if (clip)\n    {',body.index('point[3]'))
   body=body[:a]+'    if (clip) throw std::runtime_error("Legacy polygon clipping not integrated");\n\t}'
   body=body.replace('    // Don', '    if (clip) throw std::runtime_error("Legacy polygon clipping not integrated");\n    // Don',1)
  body=body.replace('    UPDATE_TEX(tex, rect)', '    TextureInfo localTex;\n    UPDATE_TEX(tex, rect)')
  bodies.append(body)
 macro=source[source.index('  #define UPDATE_TEX'):source.index('  //\n  // Helper function')]
 bodies.append(function('  void FScopeToTextureInfo('))
 body='\n'.join(bodies)
 ctor=function('TextureInfo::TextureInfo(Bitmap *texture, U32 texMode)').replace('{', '{\n  if (!texture) throw std::invalid_argument("Missing texture");',1)
 uv=function('void TextureInfo::UpdateUV(')
 skin=function('void TextureSkin::Render(')
 skin=skin.replace('{', '{\n  if (colorIndex >= ColorGroup::MAX_INDEX) throw std::out_of_range("Skin color state");',1)
 output.mkdir(parents=True,exist_ok=True)
 (output/'frontend_triangle.cpp').write_text('#include "reign/frontend_draw.hpp"\n#include "reign/frontend_bitmap.hpp"\n#include "fscope.h"\n#include "control_stdload.hpp"\nnamespace IFace {\n'+macro+'\n'+body+'\n}\n'+ctor+'\n'+uv+'\n'+skin+'\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
 a=p.parse_args();prepare(a.upstream,a.output)

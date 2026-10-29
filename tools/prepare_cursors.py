#!/usr/bin/env python3
"""Stage pinned original cursor classes outside the repository."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1]
if a.output.resolve()==r or r in a.output.resolve().parents:raise ValueError('External staging required')
b=(a.upstream/'interface/cursor.cpp').read_bytes()
if hashlib.sha256(b).hexdigest()!=json.loads((r/'cmake/cursor-hashes.json').read_text())['interface/cursor.cpp']:raise ValueError('Unreviewed cursor source')
s=b.decode().replace('\r\n','\n')
def fn(sig):
 start=s.index(sig);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
classes=s[s.index('  class Base'):s.index('  // Private data')]
classes=classes.replace('IFace::RenderRectangle(frame->tex.texRect, 0xFFFFFFFF, &frame->tex);','IFace::RenderRectangle(frame->tex.texRect, 0xFFFFFFFF, &frame->tex, 1.0F);')
classes=classes.replace('FrameInfo *frame = new FrameInfo;', 'auto owned = std::make_unique<FrameInfo>();\n      FrameInfo *frame = owned.get();').replace('frames.Append(frame);','frames.Append(frame);owned.release();')
# Avoid signed overflow for externally supplied frame intervals.
classes=classes.replace('(elapsed += S32(ms)) > speed','(static_cast<unsigned long long>(elapsed) + ms) > static_cast<unsigned>(speed)')
classes=classes.replace('          // Move to next frame','          // Move to next frame')
# Preserve accumulation for intervals that do not advance a frame.
classes=classes.replace('            current.GoToHead();\n          }\n        }','            current.GoToHead();\n          }\n        }\n        else { elapsed += static_cast<S32>(ms); }')
body=fn('  U32 FindByName(')+'\n'+fn('  void ProcessCreateCursor(')+'\n'+fn('  void ProcessStandardCursors(')+'\n'+fn('  Bool Set(U32 id)')
body=body.replace('ASSERT(sysInit);','')
body=body.replace('    // Configure the cursor','    std::unique_ptr<Base> owned(newCrs);\n    if(cursors.Find(Crc::CalcStr(name))) throw std::runtime_error("Duplicate cursor");\n    // Configure the cursor')
body=body.replace('cursors.Add(Crc::CalcStr(name), newCrs);','cursors.Add(Crc::CalcStr(name), newCrs);owned.release();')
body=body.replace('LOG_ERR(("Unknown Cursor Class [%s]", cls));','throw std::runtime_error(std::string("Unknown cursor class: ")+cls);')
body=body.replace('LOG_ERR(("Cursor [%s] not found", name));','throw std::runtime_error(std::string("Unknown cursor: ")+name);')
body=body.replace('LOG_ERR(("Unknown standard cursor type [%s]", sScope->NameStr()));','throw std::runtime_error("Unknown standard cursor type");')
import re
for name in ['cursors','current','standardCrs']:body=re.sub(r'\b'+name+r'\b','state().'+name,body)
a.output.mkdir(parents=True,exist_ok=True)
(a.output/'cursor_slice.hpp').write_text('#pragma once\n#include "iface_events.hpp"\n#include "reign/frontend_draw.hpp"\nnamespace IFace {void FScopeToTextureInfo(FScope *,TextureInfo &); }\nnamespace reign::cursor_detail {\n'+classes+'''
enum {DEFAULT,IBEAM,WAIT,NO,MAX_CURSORS};
struct State {BinTree<Base> cursors;Base *current=nullptr;U32 standardCrs[MAX_CURSORS]{};~State(){cursors.DisposeAll();}};
State &state();void ProcessCreateCursor(FScope *);void ProcessStandardCursors(FScope *);Bool Set(U32);U32 FindByName(const char *);
}
''')
(a.output/'cursor_slice.cpp').write_text('#include "cursor_slice.hpp"\nnamespace reign::cursor_detail {\n'+body+'\n}\n')

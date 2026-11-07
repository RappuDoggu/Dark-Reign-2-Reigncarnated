#!/usr/bin/env python3
"""Stage original display-options type and lifetime; native display services replace DirectDraw."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/display-options-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed display source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 header=texts['graphics/vid_cmd.h'];start=header.index('  class Options :');end=header.index('};\n  //----------------------------------------------------------------------------',header.index('  class Graphics :',start))+2
 header=header[start:end]
 source=texts['graphics/vidoptions.cpp']
 def extract(sig):
  a=source.index(sig);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 graphics=texts['graphics/vidgraphics.cpp']
 for name in ['vid_private.h','vid_cmd.h','iface.h','iface_priv.h']:graphics=graphics.replace('#include \"'+name+'\"','')
 graphics=graphics.replace('IFace::sysInit','reign::iface_event_detail::state().treeReady').replace('Vid::caps.clipGuard','reign::display_options_detail::clip_guard()').replace('Command::OnModeChange();','reign::display_options_detail::mode_changed();')
 lifetime='\n'.join(extract(s) for s in ['  Options::Options(', '  Options::~Options()'])
 output.mkdir(parents=True,exist_ok=True)
 (output/'display_options_slice.hpp').write_text('#pragma once\n#include "frontend_configuration_slice.hpp"\n#include "icwindow.h"\nnamespace Vid {\n'+header+'\n}\n')
 (output/'display_options_slice.cpp').write_text('#include "display_options_slice.hpp"\n#include "reign/display_options.hpp"\nnamespace Vid {\nNList<Options> Options::allWindows(&Options::node);\n'+lifetime+'\n}\n'+graphics)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

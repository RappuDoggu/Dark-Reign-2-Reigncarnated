#!/usr/bin/env python3
"""Original IControl syntax probe; a failed compile is never reported as a link or menu."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('upstream',type=Path);p.add_argument('runtime',type=Path);p.add_argument('config',type=Path);p.add_argument('output',type=Path);p.add_argument('--compiler',default='c++');p.add_argument('--opaque-video-declarations',action='store_true');p.add_argument('--native-control-types',type=Path,help='External staged original control declarations');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve();source=a.upstream.resolve()
if out==repo or repo in out.parents:raise ValueError('Vendor/source staging must remain external')
out.mkdir(parents=True,exist_ok=True);(out/'bink').mkdir(exist_ok=True)
wrapper=(source/'3rdparty/bink.h').read_text().replace('..\\3rdparty\\bink\\bink.h','bink/bink.h')
(out/'bink.h').write_text('struct BINK; using HBINK=BINK*;\n' if a.opaque_video_declarations else wrapper)
(out/'bink/bink.h').write_text((source/'3rdparty/bink/bink.h').read_text().replace('"rad.h"','"Rad.h"'))
(out/'bink/Rad.h').write_bytes((source/'3rdparty/bink/Rad.h').read_bytes())
(out/'event.h').write_text('#pragma once\n#include "event_slice.hpp"\nusing Event=reign::event_detail::Event;namespace EventSys=reign::event_detail::EventSys;\n')
(out/'native_prefix.hpp').write_text('#include "varsys.h"\n#include "event.h"\nusing namespace reign::vars_detail;\n')
includes=[out,a.runtime.resolve(),a.config.resolve(),repo/'include']+[source/n for n in ['interface','graphics','util','system','main','coregame','game','3rdparty']]
if a.native_control_types:
    control=a.native_control_types.resolve()
    if not (control/'control_types.hpp').is_file():raise ValueError('Stage native control declarations first')
    includes.insert(1,control)
cmd=[a.compiler,'-std=c++17','-fsyntax-only','-fmax-errors=12','-include',str(out/'native_prefix.hpp')]+['-I'+str(n) for n in includes]+[str(source/'interface/icontrol.cpp')]
if a.native_control_types:cmd[1:1]=['-include',str(control/'control_types.hpp')]
r=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(r.stdout+r.stderr)
meta={'translation_unit':'interface/icontrol.cpp','compiler_exit_code':r.returncode,'opaque_video_declarations':a.opaque_video_declarations,'native_control_types':str(control) if a.native_control_types else None,'command':cmd,'linked':False,'engine_initialized':False,'menu_rendered':False}
(out/'result.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps(meta,indent=2));print(r.stderr[:6000]);raise SystemExit(r.returncode)

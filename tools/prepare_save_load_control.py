#!/usr/bin/env python3
"""Stage the complete original save/load control with explicit engine service boundaries."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/save-load-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed save/load source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 header=texts['game/game_saveload.h'].replace('#include "game_preview.h"','')
 source=texts['game/game_saveload.cpp']
 for h in ['savegame.h','user.h','console.h']:
  source=source.replace('#include "'+h+'"','')
 source=source.replace('#include "babel.h"','#include "babel_slice.hpp"')
 source=source.replace('TRANSLATE(("#shell.win.options.saveload.empty"))','Babel::Translate("#shell.win.options.saveload.empty",0)')
 source=source.replace('SaveGame::','reign::save_load_detail::').replace('Missions::','reign::save_load_detail::').replace('Clock::GetDateAndTime()','reign::save_load_detail::DateTime()').replace('info.description)', 'info.description.c_str())')
 output.mkdir(parents=True,exist_ok=True)
 (output/'game_saveload.h').write_text('#include "frontend_configuration_slice.hpp"\n'+header)
 (output/'game_saveload.cpp').write_text('#include "reign/save_load_services.hpp"\n'+source)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

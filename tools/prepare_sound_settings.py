#!/usr/bin/env python3
"""Stage the original sound-settings window against an explicit disabled-audio backend."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/sound-settings-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed sound-settings source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 source=texts['interface/icsoundconfig.cpp'].replace('#include "sound.h"','#include "reign/sound_settings.hpp"').replace('#include "random.h"','').replace('Sound::','reign::disabled_audio_detail::').replace('Random::nonSync.Integer','reign::disabled_audio_detail::RandomTrack')
 output.mkdir(parents=True,exist_ok=True)
 (output/'icsoundconfig.h').write_text('#include "frontend_configuration_slice.hpp"\n#include "iclistbox.h"\n'+texts['interface/icsoundconfig.h'])
 (output/'icsoundconfig.cpp').write_text(source)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

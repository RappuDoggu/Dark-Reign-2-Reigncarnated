#!/usr/bin/env python3
"""Stage original IFace sound Play routing onto explicit native consumers."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'interface/iface_sound.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/frontend-effects-hashes.json').read_text())['interface/iface_sound.cpp']:raise ValueError('Unreviewed sound routing source')
 source=raw.decode().replace('\r\n','\n');a=source.index('    void Play(U8 id, IControl *control)');b=source.index('{',a)+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 body=source[a:b].replace('soundTable','reign::effects_detail::state().table')
 body=body.replace('      control;', '      auto lifetime = reign::effects_detail::state().table[id].effect;\n      control;')
 def extract(signature):
  start=source.index(signature);end=source.index('{',start)+1;depth=1
  while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
  return source[start:end]
 configure=extract('    void Configure(FScope *fScope)')
 configure=configure.replace('      ASSERT(fScope)', '      if (!fScope) throw std::invalid_argument("Missing sound scope");')
 configure=configure.replace('ConfigureSound(MENU_COMMAND, sScope)', 'ConfigureSound(2, sScope)').replace('ConfigureSound(MENU_POPUP, sScope)', 'ConfigureSound(1, sScope)')
 end=configure.rfind('        }');configure=configure[:end]+'          default: throw std::runtime_error("Unknown interface sound type");\n'+configure[end:]
 sound=extract('    static void ConfigureSound(SoundType t, FScope *fScope)').replace('SoundType t','U8 t')
 begin=sound.index('      // Delete previous sound');end=sound.index('      // Volume is optional')
 sound=sound[:begin]+'''      auto &state = reign::effects_detail::state();
      if (!state.loader) throw std::runtime_error("Sound effect loader not bound");
      auto effect = std::make_shared<reign::effects_detail::Effect>(reign::effects_detail::Effect{state.loader(fScope->NextArgString())});
      if (!effect->play) throw std::runtime_error("Sound loader returned no consumer");
      float volume;
'''+sound[end:]
 sound=sound.replace('soundTable[t].volume','volume')
 sound=sound[:sound.rfind('    }')]+'      state.table[t] = {std::move(effect), volume};\n    }'
 body=sound+'\n'+configure+'\n'+body
 output.mkdir(parents=True,exist_ok=True)
 (output/'frontend_effects_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "fscope.h"
#include "control_stdload.hpp"
#include <array>
#include <functional>
#include <memory>
class IControl;
namespace reign::effects_detail {
struct Effect {std::function<void(float)> play; void Play2D(float volume){play(volume);}};
struct Item {std::shared_ptr<Effect> effect;float volume=1;};
struct State {std::array<Item,256> table;std::function<std::function<void(float)>(const std::string &)> loader;};
State &state();
}
namespace IFace::Sound {void Play(U8,IControl *);void Configure(FScope *);}
''')
 (output/'frontend_effects_slice.cpp').write_text('#include "frontend_effects_slice.hpp"\nnamespace IFace::Sound {\n'+body+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

#!/usr/bin/env python3
"""Stage original IFace configuration dispatch against scoped native services."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'interface/iface.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/frontend-configuration-hashes.json').read_text())['interface/iface.cpp']:raise ValueError('Unreviewed interface configuration')
 source=raw.decode().replace('\r\n','\n')
 def function(signature):
  a=source.index(signature+'\n  {');b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 body=function('  static void Configure(FScope *fScope)').replace('static void Configure','void ConfigureNative')
 body=body.replace('    ASSERT(fScope);','    (void)reign::frontend_configuration_detail::state();\n    if (!fScope) throw std::invalid_argument("Missing interface scope");')
 calls={'CreateRegData(sScope)':'reign::iface_event_detail::load_registry(sScope)',
 'ProcessCreateColorGroup(sScope)':'reign::frontend_configuration_detail::state().theme->configure_group(sScope)',
 'ProcessCreateSkin(sScope)':'reign::frontend_configuration_detail::state().theme->configure_skin(sScope)',
 'ConfigureColors(sScope)':'reign::frontend_configuration_detail::state().theme->configure_colors(sScope)',
 'FontSys::Create(sScope)':'reign::frontend_configuration_detail::state().fonts->configure_scope(sScope)',
 'ConfigureMetrics(sScope)':'reign::frontend_configuration_detail::state().metrics->configure_scope(sScope)'}
 for old in ['KeyBind::ProcessScanCodes','KeyBind::ProcessModifiers','CursorSys::ProcessCreateCursor','IFace::Sound::Configure','CursorSys::ProcessStandardCursors']:
  calls[old+'(sScope)']='reign::frontend_configuration_detail::forward(sScope)'
 for before,after in calls.items():
  if body.count(before)!=1:raise ValueError('Interface config anchor changed: '+before)
  body=body.replace(before,after)
 # Unlike the original switch, do not silently discard unsupported instructions.
 anchor='      switch (sScope->NameCrc())'
 body=body.replace(anchor,'      ++reign::frontend_configuration_detail::state().processed;\n'+anchor)
 at=body.rfind('      }');body=body[:at]+'        default: throw std::runtime_error(std::string("Unknown interface configuration: ")+sScope->NameStr());\n'+body[at:]
 root=function('  static void CreateRootLevelControl(FScope *fScope)').replace('static void CreateRootLevelControl','void CreateNativeRootControl')
 root=root.replace('    CreateControl(fScope, NULL);','    (void)reign::frontend_configuration_detail::state();\n    if (!fScope) throw std::invalid_argument("Missing root control scope");\n    CreateControl(fScope, NULL);')
 start=source.index('    const char *language = MultiLanguage::GetLanguage();')
 end=source.index('\n',source.index('VarSys::CreateString("iface.language", language, VarSys::NOEDIT);',start))
 language='void InitializeNativeLanguage(){\n'+source[start:end]+'\n}\n'
 output.mkdir(parents=True,exist_ok=True)
 (output/'frontend_configuration_slice.hpp').write_text('''#pragma once
#include "iface_events.hpp"
namespace IFace {void FScopeToColor(FScope *,Color &);void ConfigureDefaultConsoleColor(FScope *);void ConfigureConsoleColors(FScope *); }
#include "reign/frontend_theme.hpp"
#include "reign/font_registry.hpp"
#include "reign/interface_metrics.hpp"
namespace reign::frontend_configuration_detail {
struct State {FrontendTheme *theme;FontRegistry *fonts;InterfaceMetrics *metrics;std::function<void(FScope *)> service;unsigned processed=0;};
State &state();void forward(FScope *);
}
namespace IFace {void InitializeNativeLanguage();void ConfigureNative(FScope *);void CreateNativeRootControl(FScope *);}
''')
 (output/'frontend_configuration_slice.cpp').write_text('#include "frontend_configuration_slice.hpp"\n#include "localization_slice.hpp"\nnamespace IFace {\n'+language+body+'\n'+root+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

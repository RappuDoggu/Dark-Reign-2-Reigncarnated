#!/usr/bin/env python3
"""Stage original campaign selector and preview metadata, with native scoped resources."""
import argparse,hashlib,json
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/campaign-selection-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed campaign selection source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 def extract(source,sig):
  a=source.index(sig);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 output.mkdir(parents=True,exist_ok=True)
 header=texts['coregame/game_preview.h'].replace('#include "bitmap.h"','#include "reign/frontend_bitmap.hpp"').replace('#include "gameconstants.h"','')
 header=header.replace('    Bitmap *terrain;','    Bitmap *terrain;\n    std::unique_ptr<Bitmap> terrainOwner;')
 (output/'game_preview.h').write_text('#include "catalog_types.hpp"\n'+header)
 source=texts['coregame/game_preview.cpp']
 signatures=['  Preview::TeamInfo::TeamInfo(', '  Preview::Preview(', '  Preview::~Preview()', '  void Preview::LoadTextures()', '  void Preview::ReleaseTextures()', '  void Preview::ReloadTextures()', '  const Missions::Mission * Preview::GetMission()', '  Preview::TeamInfo *Preview::FindTeamId(']
 body='\n'.join(extract(source,sig) for sig in signatures)
 body=body.replace('Sides::GetSide().GetName().str','reign::preview_detail::default_side()').replace('FILENAME_MISSION_CONFIG','"game.cfg"')
 body=body.replace('    terrain(NULL),','    terrain(NULL),\n    size(0),')
 body=body.replace('const char *oldStream = OpenMissionStream();','auto missionResources = reign::preview_detail::open(path.str, name.str);')
 block='''    if (oldStream)
    {
      CloseMissionStream(oldStream);
    }'''
 if body.count(block)!=2:raise ValueError('Preview stream restoration changed')
 body=body.replace(block,'')
 old=extract(source,'  void Preview::LoadTextures()')
 body=body.replace(old,'''  void Preview::LoadTextures() {
    terrainOwner = reign::preview_detail::load_texture("terrain.bmp");
    terrain = terrainOwner.get();
  }''')
 body=body.replace('    terrain = NULL;','    terrainOwner.reset();\n    terrain = NULL;')
 # Original constructor used global fatal exits. Native exceptions must release teams.
 a=body.index('  Preview::Preview(');b=body.index('{',a);d=1;e=b+1
 while d:d+=(body[e]=='{')-(body[e]=='}');e+=1
 body=body[:b+1]+'\n    try {\n'+body[b+1:e-1]+'\n    } catch (...) { teams.DisposeAll(); throw; }\n'+body[e-1:]
 point=extract(texts['util/stdload.h'],'  template <class TYPE> void TypePoint(FScope *parent, const char *name, Point<TYPE> &val,')
 point=point.replace('fScope ? TypePoint<TYPE>(fScope, val) : val = dVal;','if (fScope) TypePoint<TYPE>(fScope, val); else val = dVal;')
 (output/'game_preview.cpp').write_text('#include "game_preview.h"\n#include "reign/campaign_selection.hpp"\n#include "ptree.h"\nnamespace StdLoad {\n'+point+'\n}\nnamespace Game {\n'+body+'\n}\n')
 (output/'game_campaignselection.h').write_text('#include "catalog_types.hpp"\n'+texts['game/game_campaignselection.h'])
 s=texts['game/game_campaignselection.cpp']
 for dep in ['worldctrl.h','unitobj.h','game.h','console.h','common_mapwindow.h','coregame.h','main.h']:s=s.replace('#include "'+dep+'"','')
 s=s.replace('#include "babel.h"','#include "babel_slice.hpp"')
 s=s.replace('TRANSLATE(((*i)->GetDescription().str))','Babel::Translate((*i)->GetDescription().str,0)')
 s=s.replace('Main::runCodes.Set(', 'reign::preview_detail::request_run_code(')
 s=s.replace('            preview->ReloadTextures();', '            if (previewDisplay.Alive()) previewDisplay->SetImage(nullptr);\n            preview->ReloadTextures();\n            ValidatePreview();')
 (output/'game_campaignselection.cpp').write_text('#include "reign/campaign_selection.hpp"\n'+s)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

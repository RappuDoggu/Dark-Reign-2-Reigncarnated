#!/usr/bin/env python3
"""Stage original campaign/progress/difficulty and read-only mission catalog code."""
import argparse,hashlib,json,re
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/campaign-catalog-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed catalog source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 output.mkdir(parents=True,exist_ok=True)
 def extract(source,sig):
  a=source.index(sig);b=source.index('{',a)+1;depth=1
  while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
  return source[a:b]
 common='#include "frontend_configuration_slice.hpp"\n#include "control_stdload.hpp"\nusing FileIdent=StrCrc<128>;using PathIdent=StrCrc<270>;using GameIdent=StrCrc<64>;using MultiIdent=StrBuf<128>;\n'
 (output/'catalog_types.hpp').write_text('#pragma once\n'+common)
 for name in ['missions','campaigns','difficulty']:
  h=texts['game/'+name+'.h']
  for dep in ['filesys.h','multilanguage.h','utiltypes.h','stats.h']:h=h.replace('#include "'+dep+'"','')
  (output/(name+'.h')).write_text('#include "catalog_types.hpp"\n'+h)
 for name in ['campaigns','difficulty']:
  s=texts['game/'+name+'.cpp'].replace('#include "stdload.h"','#include "catalog_save.hpp"').replace('#include "filesys.h"','')
  if name=='difficulty':s=s.replace('    LoadConfig();','    try { LoadConfig(); } catch (...) { ClearSettings(); throw; }')
  (output/(name+'.cpp')).write_text(s)
 save=texts['util/stdload.cpp']
 signatures=['  FScope* TypeU32(FScope *scope,','  FScope* TypeU32(FScope *parent,','  FScope* TypeString(FScope *scope,','  FScope* TypeString(FScope *parent,']
 bodies=[extract(save,s) for s in signatures]
 (output/'catalog_save.hpp').write_text('#pragma once\n#include "catalog_types.hpp"\nnamespace StdSave {\n'+'\n'.join('inline '+b.lstrip() for b in bodies)+'\n}\n')
 mission=texts['game/missions.cpp']
 signatures=['  Mission::Mission(', '  Bool Mission::IsSystem()', '  Group::Group(', '  Group::~Group()', '  void Group::Reset()', '  const Mission * Group::RegisterMission(', '  const Mission * Group::RegisterMissionPack(', '  void Group::LoadConfig()', '  U32 Group::Generate()', '  const Mission * Group::FindMission(U32', '  const Mission * Group::FindMission(const FileIdent', '  static Group * RegisterGroup(', '  static void RegisterGroups(', '  static Group * FindGroupInternal(', '  static void SetupWritePath(', '  static U32 Generate()', '  void Init()', '  void Done()', '  Bool Initialized()', '  const NList<Group> & GetGroups()', '  const Group * FindGroup(U32', '  const Group * FindGroup(const PathIdent', '  const Mission * FindMission(const FileIdent', '  const Mission * FindMission(U32', '  void SetSelected(', '  const Mission * GetSelected()']
 body='\n'.join(extract(mission,s) for s in signatures)
 body=body.replace('Utils::Strstr(', 'std::strstr(')
 body=body.replace('File::','reign::mission_fs::File::').replace('Dir::','reign::mission_fs::Dir::').replace('FilePath','reign::mission_fs::FilePath').replace('FileSys::GetPackExtension()','".zwp"')
 body=body.replace("if (char *ptr = Utils::Strchr(missionName.str, '.'))","if (char *ptr = std::strchr(missionName.str, '.'))")
 (output/'mission_catalog_slice.cpp').write_text('#include "missions.h"\n#include "ptree.h"\n#include "campaigns.h"\n#include "reign/mission_fs.hpp"\nnamespace Missions {\nstatic const char *MISSIONS_FILE_GROUP="group.cfg";static bool initialized=false;static NList<Group> groups(&Group::node);static const Mission *selected=nullptr,*active=nullptr;static reign::mission_fs::FilePath writePath;static bool safeLoad=false;\n'+body+'\n}\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

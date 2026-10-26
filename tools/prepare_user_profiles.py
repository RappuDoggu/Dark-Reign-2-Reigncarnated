#!/usr/bin/env python3
"""Stage original User/statistics/Login source with native private-profile IO."""
import argparse,hashlib,json,re
from pathlib import Path
def prepare(upstream,output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 texts={}
 for name,digest in json.loads((repo/'cmake/user-profile-hashes.json').read_text()).items():
  raw=(upstream/name).read_bytes()
  if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed user source: '+name)
  texts[name]=raw.decode().replace('\r\n','\n')
 def bounds(source,sig):
  a=source.index(sig);b=source.index('{',a)+1;depth=1;c=b
  while depth:depth+=(source[c]=='{')-(source[c]=='}');c+=1
  return a,b,c
 output.mkdir(parents=True,exist_ok=True)
 for name in ['user','stats','game_login']:
  prefix='util/' if name=='stats' else 'game/'
  header=texts[prefix+name+'.h'].replace('#include "utiltypes.h"','')
  (output/(name+'.h')).write_text('#include "catalog_types.hpp"\nusing GameIdentList=List<GameIdent>;\n'+header)
 stats=texts['util/stats.cpp'].replace('#include "stdload.h"','#include "catalog_save.hpp"')
 (output/'stats.cpp').write_text(stats)
 user=texts['game/user.cpp'].replace('#include "mods.h"','').replace('#include "sound.h"','#include "reign/sound_settings.hpp"').replace('#include "stdload.h"','#include "user_stdload.hpp"')
 user=user.replace('Sound::','reign::disabled_audio_detail::').replace('File::Exists(', 'reign::profile_detail::exists(').replace('File::Remove(', 'reign::profile_detail::remove_profile(').replace('Dir::MakeFull(', 'reign::profile_detail::make_directory(')
 user=user.replace('Dir::','reign::mission_fs::Dir::').replace('File::Attrib::','reign::mission_fs::File::Attrib::').replace('FilePath','reign::mission_fs::FilePath')
 user=user.replace('Utils::MakePath(', 'reign::profile_detail::make_path(')
 for old,new in [('USER_PATH, buf, NULL','USER_PATH, buf'),('path, file, NULL','path, file'),('USER_PATH, dir, NULL','USER_PATH, dir')]:user=user.replace(old,new)
 user=user.replace('pTree.WriteTreeBinary(GetFile(path, USER_FILE_INFO))','reign::profile_detail::write_tree(pTree, GetFile(path, USER_FILE_INFO))').replace('pTree.WriteTreeBinary(GetFile(path, USER_FILE_DATA))','reign::profile_detail::write_tree(pTree, GetFile(path, USER_FILE_DATA))')
 user=user.replace('ERR_FATAL(("Stat \'%s\' type mismatch"))','ERR_FATAL(("Stat \'%s\' type mismatch", name))')
 for signature,phase,team in [('  void InitializeAddons()',0,'nullptr'),('  void LoadAddons()',1,'nullptr'),('  void SetupAddons(Team *team)',2,'team')]:
  a,b,c=bounds(user,signature)
  user=user[:b]+'\n    ASSERT(initialized)\n    ASSERT(active)\n    for (List<GameIdent>::Iterator i(&addons); *i; ++i) reign::profile_detail::addon('+str(phase)+', (*i)->crc, '+team+');\n  }'+user[c:]
 # Recover partially loaded records when native exceptions replace fatal exits.
 a,b,c=bounds(user,'  Bool Login(const ExistingUser *user)')
 user=user[:b]+'\n    try {\n'+user[b:c-1]+'\n    } catch (...) { info.DisposeAll(); addons.DisposeAll(); configs.DisposeAll(); Campaigns::GetProgress().Clear(); active=nullptr; throw; }\n'+user[c-1:]
 cleanup='''\nnamespace User {
void AbortNative() {info.DisposeAll();addons.DisposeAll();configs.DisposeAll();Campaigns::GetProgress().Clear();existingUsers.DisposeAll();active=nullptr;initialized=FALSE;}
}\n'''
 (output/'user.cpp').write_text('#include "reign/profile_storage.hpp"\n'+user+cleanup)
 a,b,c=bounds(texts['util/stdload.h'],'  template <U32 SIZE> void TypeStrCrcList(')
 (output/'user_stdload.hpp').write_text('#pragma once\n#include "catalog_save.hpp"\nnamespace StdLoad {\n'+texts['util/stdload.h'][a:c]+'\n}\n')
 (output/'game_login.cpp').write_text(texts['game/game_login.cpp'])
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)

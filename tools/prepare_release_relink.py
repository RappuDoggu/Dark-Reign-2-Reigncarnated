#!/usr/bin/env python3
"""Prepare portable, debug-stripped relink inputs for the menu AND mission.

Reads the real Ninja menu command and mission native-build.json. Requires clean,
committed port source and original source provenance; never exports keys/game data.
The emitted relink.py builds the exact release .so files with the pinned NDK.
"""
import argparse,hashlib,json,shlex,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['mission','menu','ffmpeg','ndk','output']:p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--native-source-revision',required=True,help='Source checkpoint actually compiled into the reused objects')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();ndk=a.ndk.resolve();ff=a.ffmpeg.resolve()
if out==repo or repo in out.parents:p.error('Use a new output directory outside the checkout')
if out.exists() and any(out.iterdir()):p.error('Use an empty output directory; do not overwrite a previous kit')
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
if subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip():p.error('Commit release source before exporting it')
# A reused object must correspond to the current implementation. Documentation,
# metadata and packaging changes do not require recompiling unchanged game code.
changed=subprocess.check_output(['git','diff','--name-only',a.native_source_revision,revision,'--','engine','src','include','android/mission','android/menu/native_menu.cpp','tools/prepare_engine_full.py','tools/prepare_iface_events.py'],cwd=repo,text=True).strip()
if changed:p.error('Native implementation changed since the compiled source; rebuild first: '+changed)
llvm=ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin';strip=llvm/'llvm-strip';files={};records={}
for directory in ['application','decoder-lib','link','decoder']:(out/directory).mkdir(parents=True,exist_ok=True)
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def copy_input(path):
 path=path.resolve()
 if path in files:return files[path]
 if not path.is_file():raise FileNotFoundError(path)
 folder='decoder-lib' if ff in path.parents else 'application'
 name=digest(path)[:16]+'-'+path.name if folder=='application' else path.name
 rel=folder+'/'+name;target=out/rel;shutil.copy2(path,target)
 if folder=='application' and path.suffix in ('.o','.a'):
  subprocess.run([strip,'--strip-debug',target],check=True)
 files[path]=rel;records[rel]={'sha256':digest(target),'bytes':target.stat().st_size}
 return rel
# Verify recorded mission inputs instead of trusting a stale external cache.
mission=json.loads((a.mission/'native-build.json').read_text())
for item in mission['original_objects'].values():
 if digest(Path(item['path']))!=item['sha256']:p.error('Recovered original object no longer matches its native receipt')
menu_text=subprocess.check_output(['ninja','-C',str(a.menu),'-t','commands','libreign_menu.so'],text=True).strip().splitlines()[-1]
tokens=shlex.split(menu_text);begin=next(i for i,t in enumerate(tokens) if Path(t).name=='clang++');end=next((i for i in range(begin,len(tokens)) if tokens[i]=='&&'),len(tokens));menu=tokens[begin:end]
def portable(command,cwd,library):
 result=[];i=0
 while i<len(command):
  token=command[i]
  if token=='-Xlinker' and i+1<len(command) and command[i+1].startswith('--dependency-file='):i+=2;continue
  if token=='-o':result+=['-o','@OUT@/'+library+'.unstripped.so'];i+=2;continue
  if token.endswith(('.o','.a')):
   path=Path(token);path=path if path.is_absolute() else cwd/path;token=copy_input(path)
  elif token.startswith('-Wl,--version-script='):
   path=Path(token.split('=',1)[1]);path=path if path.is_absolute() else cwd/path
   name=library+'.exports.map';shutil.copy2(path,out/'link'/name);token='-Wl,--version-script=link/'+name
  token=token.replace(str(ndk),'@NDK@')
  if '/workspace/' in token:raise ValueError('Unmapped workspace argument: '+token)
  result.append(token);i+=1
 return result
commands={
 'libreign_menu':portable(menu,a.menu.resolve(),'libreign_menu'),
 'libreign_mission':portable(mission['link_command'],a.mission.resolve(),'libreign_mission')}
(out/'link-commands.json').write_text(json.dumps(commands,indent=2)+'\n')
archive=ff.parent.parent/'ffmpeg-n6.1.1.tar.gz'
assert digest(archive)=='7c1ebea95d815e49c1e60c7ee816410dec73a81b8ac002b276780d2f9048e598'
shutil.copy2(archive,out/'decoder/ffmpeg-n6.1.1.tar.gz')
shutil.copy2(ff/'COPYING.LGPLv2.1',out/'COPYING.LGPLv2.1')
shutil.copy2(repo/'tools/build_bink_decoder.sh',out/'build_bink_decoder.sh')
for name in ['config.h','ffbuild/config.mak']:
 path=ff.parent/name
 if path.is_file():(out/('decoder/'+Path(name).name)).write_text(path.read_text().replace(str(ndk),'@NDK@').replace(str(ff.parent.parent),'@DECODER_ROOT@'))
(out/'relink.py').write_text('''#!/usr/bin/env python3
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--ndk',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--decoder-libs',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
results={}
for name,command in json.loads((root/'link-commands.json').read_text()).items():
 args=[]
 for token in command:
  if a.decoder_libs and token.startswith('decoder-lib/'):token=str(a.decoder_libs.resolve()/Path(token).name)
  args.append(token.replace('@NDK@',str(a.ndk.resolve())).replace('@OUT@',str(out)))
 subprocess.run(args,cwd=root,check=True)
 target=out/(name+'.so')
 subprocess.run([a.ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip','--strip-unneeded','-o',target,out/(name+'.unstripped.so')],check=True)
 results[target.name]=hashlib.sha256(target.read_bytes()).hexdigest()
(out/'relink-result.json').write_text(json.dumps(results,indent=2)+'\\n');print(json.dumps(results,indent=2))
''')
(out/'README.md').write_text('''# Reigncarnated 0.1 source and relink kit

This kit replaces the historical menu-only v0.5 recipe. Both native libraries
are rebuildable from these exact debug-stripped application objects and decoder
libraries. No development key, game packs or unstripped native library is included.
Port source and the pinned full FFmpeg n6.1.1 archive accompany these objects.

Linux x86_64, Python3, Android NDK27.2.12479018:

    python3 relink.py --ndk /path/to/ndk --output /private/relinked

The two stripped library SHA256 values must match the release build receipt.
To modify FFmpeg, unpack decoder/ffmpeg-n6.1.1.tar.gz, make your edits and run its
configure/make/install with the flags in build_bink_decoder.sh (invoke configure
directly for modified source; do not weaken the upstream archive checksum).
Then pass --decoder-libs /your/modified/install/lib to relink.py.
For an unmodified decoder rebuild:

    export ANDROID_NDK_HOME=/path/to/ndk
    bash build_bink_decoder.sh "$PWD/decoder" android
    python3 relink.py --ndk "$ANDROID_NDK_HOME" --output /private/relinked --decoder-libs "$PWD/decoder/android/install/lib"

Replace lib/arm64-v8a/libreign_menu.so and libreign_mission.so in unsigned.apk,
storing the libraries uncompressed. Use Android build-tools35.0.0 zipalign -P16
then apksigner with YOUR private key. The port's original signing key is never
part of the kit. A modified app signed by a different key cannot update the
existing installation; export owned data before uninstalling it or change the
package identity when rebuilding Java/manifest from source.

source/ contains matching port source, build instructions and source hashes.
The complete original engine source is fetched separately at the pin in those
instructions; application objects are supplied here for decoder replacement.
See source/docs/SOURCE_PROVENANCE.md and COPYING.LGPLv2.1 for provenance/notices.
''')
manifest={'release_source_revision':revision,'native_source_revision':a.native_source_revision,'ndk':'27.2.12479018','ffmpeg':'n6.1.1','application_inputs':records,'objects_debug_stripped':True,'game_data_or_keys':False}
(out/'kit-inputs.json').write_text(json.dumps(manifest,indent=2)+'\n')
# Omit historical binaries/media/logs, retain all current source, authored test
# fixtures and source checks. List exclusions explicitly in the kit receipt.
tracked=subprocess.check_output(['git','ls-files','-z'],cwd=repo).split(b'\0');excluded=[]
for raw in tracked:
 if not raw:continue
 name=raw.decode();path=Path(name)
 if path.parts[0] in ('builds','trailers') or name.startswith('docs/logs/') or path.suffix in ('.apk','.a','.so','.o'):
  excluded.append(name);continue
 if path.suffix.lower() in ('.keystore','.jks','.trace','.zwp','.bik'):raise ValueError('Private/game file tracked: '+name)
 target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(subprocess.check_output(['git','show',revision+':'+name],cwd=repo))
(out/'source-export.json').write_text(json.dumps({'revision':revision,'excluded_historical_artifacts':excluded},indent=2)+'\n')
print(out)

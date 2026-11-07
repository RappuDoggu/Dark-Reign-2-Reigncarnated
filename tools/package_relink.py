#!/usr/bin/env python3
"""Bundle exact application objects, pinned decoder source and a portable relink recipe.
No signing keys or game assets are included. Run after package_menu.sh.
"""
import argparse,json,pathlib,shlex,shutil,zipfile,hashlib
p=argparse.ArgumentParser();p.add_argument('build',type=pathlib.Path);p.add_argument('ffmpeg',type=pathlib.Path);p.add_argument('ndk',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args()
build=a.build.resolve();ff=a.ffmpeg.resolve();ndk=a.ndk.resolve();root=a.output.resolve().with_suffix('');root.mkdir(parents=True,exist_ok=True)
(root/'objects').mkdir(exist_ok=True);(root/'ffmpeg-lib').mkdir(exist_ok=True)
args=shlex.split((build/'CMakeFiles/reign_menu.dir/link.txt').read_text());portable=[];skip=False
for arg in args:
 if skip:skip=False;continue
 if arg=='-Xlinker':skip=True;continue
 if arg.endswith(('.a','.o')):
  source=pathlib.Path(arg);source=source if source.is_absolute() else build/source
  folder='ffmpeg-lib' if ff in source.parents else 'objects';target=root/folder/source.name
  if target.exists() and target.read_bytes()!=source.read_bytes():target.unlink()
  shutil.copy2(source,target);portable.append(folder+'/'+source.name)
 else:portable.append(arg.replace(str(ndk),'@NDK@'))
(root/'link-args.json').write_text(json.dumps(portable,indent=2)+'\n')
(root/'relink.py').write_text('''#!/usr/bin/env python3
import argparse,json,pathlib,subprocess
p=argparse.ArgumentParser();p.add_argument('--ndk',required=True);p.add_argument('--decoder-libs',help='Directory containing rebuilt libavformat.a, libavcodec.a, libswscale.a and libavutil.a');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent
args=json.loads((root/'link-args.json').read_text())
args=[x.replace('@NDK@',str(pathlib.Path(a.ndk).resolve())) for x in args]
if a.decoder_libs:args=[str(pathlib.Path(a.decoder_libs).resolve()/pathlib.Path(x).name) if x.startswith('ffmpeg-lib/') else x for x in args]
subprocess.run(args,cwd=root,check=True)
print(root/'libreign_menu.so')
''')
for name in ['ffmpeg-n6.1.1.tar.gz']:
 shutil.copy2(ff/name,root/name)
shutil.copy2(ff/'source/COPYING.LGPLv2.1',root/'COPYING.LGPLv2.1')
shutil.copy2(pathlib.Path(__file__).with_name('build_bink_decoder.sh'),root/'build_bink_decoder.sh')
for name in ['config.h','ffbuild/config.mak','ffbuild/config.log']:
 source=ff/'android'/name
 if source.exists():shutil.copy2(source,root/('ffmpeg-'+source.name))
shutil.copy2(build/'unsigned.apk',root/'unsigned.apk')
(root/'README.txt').write_text('''Reigncarnated v0.5 decoder source and relink materials

FFmpeg n6.1.1, LGPL 2.1 or later. The exact upstream source archive and license
are included, along with this build's configuration and application object files.
No game data or development signing key is included.

Linux x86_64; Android NDK r27c (27.2.12479018), Android build-tools 35.0.0.
To relink the supplied application with the supplied decoder:
  python3 relink.py --ndk /path/to/android-ndk-r27c
To use a modified decoder, unpack the FFmpeg source, make your changes, build
using the Android configuration in build_bink_decoder.sh, then:
  python3 relink.py --ndk /path/to/ndk --decoder-libs /your/install/lib
The checksum-pinned script is for reproducing the unmodified source; after
modification invoke source/configure directly with its documented arguments.

Replace lib/arm64-v8a/libreign_menu.so in a copy of unsigned.apk with the result
using an ordinary ZIP tool (store, do not compress, the .so). Then run:
  zipalign -f -P 16 4 unsigned.apk aligned.apk
  apksigner sign --ks YOUR_KEYSTORE --out modified.apk aligned.apk
A different signing key requires uninstalling the old package first; retain your
owned data ZIP externally before uninstalling. APK contains no game assets.

This kit enables decoder replacement/relinking; it does not grant rights to the
original game assets or resolve licensing of the original engine. Keep this
project and its engine-derived application objects private.
''')
manifest={str(f.relative_to(root)):hashlib.sha256(f.read_bytes()).hexdigest() for f in root.rglob('*') if f.is_file() and f.name!='SHA256.json'}
(root/'SHA256.json').write_text(json.dumps(manifest,indent=2)+'\n')
with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,str(f.relative_to(root)))
print(a.output.resolve())

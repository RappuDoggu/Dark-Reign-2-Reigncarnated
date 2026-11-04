#!/usr/bin/env python3
"""Package rebuilt menu and mission libraries as an update to the test candidate.
Requires the exact private v05 base APK for notices/fallback menu, newly linked
ARM64 libraries, SDK 35, and a JDK. Rebuilds all Java for the candidate app ID.
"""
import argparse,hashlib,subprocess,zipfile,json,shutil,re
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--signing-key',type=Path);p.add_argument('--new-recovery-key',action='store_true',help='Separate recovery application ID and private new signing key')
p.add_argument('--output-name',default='Reigncarnated-Recovery.apk')
p.add_argument('--application-id',help='Explicit installation identity, including a side-by-side test identity when the old private signer is unavailable')
p.add_argument('--app-label',help='Visible Android launcher label for an explicit application ID')
p.add_argument('--menu-library',type=Path);p.add_argument('--base-apk',type=Path,required=True);p.add_argument('--native-library',type=Path,required=True);p.add_argument('--sdk',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
if out==repo or repo in out.parents:p.error('Keep keys and build output outside the checkout')
if a.signing_key and not a.signing_key.is_file():p.error('The supplied signing key does not exist')
base_hash=hashlib.sha256(a.base_apk.read_bytes()).hexdigest()
if base_hash not in {'1d0fa253b52d5d7200caa3d356a1f6b58f9d2fa7084f1cd61940581ef04bc598','9527160c7d594b6b36a99e71cb0ead8303ed58340d49e56e6c85f916d0fccc7b','1cd5e963306dc17d0869eda528fb6f02970164052374ce78d1fda1459f92cbe5'}:p.error('Unverified base APK')
if not a.menu_library:p.error('Rebuild both menu and mission libraries together')
if not a.signing_key and not a.new_recovery_key:p.error('Provide a private signing key or explicitly select --new-recovery-key')
if a.signing_key and a.new_recovery_key:p.error('Choose an existing key or a new separate recovery identity')
application_id=a.application_id or ('org.reigncarnated.recovery' if a.new_recovery_key else 'org.reigncarnated.missioncandidate')
if not re.fullmatch(r'org\.reigncarnated\.[a-z][a-z0-9_]*(?:\.[a-z][a-z0-9_]*)*',application_id):p.error('Invalid Reigncarnated application ID')
if a.app_label and not a.application_id:p.error('--app-label requires an explicit --application-id')
app_label=a.app_label or ('Reigncarnated Recovery' if application_id=='org.reigncarnated.recovery' else 'Reigncarnated IA Test')
if not app_label or len(app_label)>80 or any(c in app_label for c in '<>&"\r\n'):p.error('Invalid Android launcher label')
if Path(a.output_name).name!=a.output_name or not a.output_name.endswith('.apk'):p.error('Output name must be an APK filename')
revision=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
if subprocess.check_output(['git','-C',str(repo),'status','--porcelain'],text=True).strip():p.error('Commit the source before packaging so the APK has an exact revision')
receipt={'source_revision':revision,'upstream_revision':'9ce9da5f21604310a997f0c41e9cd383f5e292c3','base_apk_sha256':base_hash,'sdk':35,'ndk':'27.2.12479018','ffmpeg':'n6.1.1','android_abi':'arm64-v8a','application_id':application_id,'application_label':app_label,'simulation':'original 10 Hz; unchanged','native_libraries':{str(f.name):hashlib.sha256(f.read_bytes()).hexdigest() for f in [a.native_library,a.menu_library]},'recovery_identity':a.new_recovery_key}

platform=a.sdk/'platforms/android-35/android.jar';tools=a.sdk/'build-tools/35.0.0'
classes=out/'classes';dex=out/'dex';classes.mkdir(exist_ok=True);dex.mkdir(exist_ok=True)
def run(cmd):subprocess.run([str(x) for x in cmd],check=True)
run(['java','com.sun.tools.javac.Main','--release','8','-classpath',platform,'-d',classes,*sorted((repo/'android/menu/org/reigncarnated/menu').glob('*.java'))])
run([tools/'d8','--lib',platform,'--min-api','26','--output',dex,*sorted(classes.rglob('*.class'))])
manifest=out/'AndroidManifest.xml';manifest_text=(repo/'android/menu/AndroidManifest.xml').read_text()
manifest_text=manifest_text.replace('org.reigncarnated.missioncandidate',application_id).replace('Reigncarnated IA Test',app_label)
manifest.write_text(manifest_text)
unsigned=out/'mission-unsigned.apk';run([tools/'aapt2','link','-o',unsigned,'-I',platform,'--manifest',manifest])
with zipfile.ZipFile(a.base_apk) as old,zipfile.ZipFile(unsigned,'a') as apk:
 for item in old.infolist():
  if (item.filename.startswith('assets/') and item.filename not in {'assets/notices/performance-candidate.txt','assets/build/source.json','assets/build/BUILD.md','assets/build/PROGRESS.md'}) or (item.filename=='lib/arm64-v8a/libreign_menu.so' and not a.menu_library):apk.writestr(item.filename,old.read(item),compress_type=zipfile.ZIP_STORED if item.filename.endswith('.so') else zipfile.ZIP_DEFLATED)
 if a.menu_library:apk.write(a.menu_library,'lib/arm64-v8a/libreign_menu.so',compress_type=zipfile.ZIP_STORED)
 apk.write(a.native_library,'lib/arm64-v8a/libreign_mission.so',compress_type=zipfile.ZIP_STORED)
 for file in dex.glob('classes*.dex'):apk.write(file,file.name,compress_type=zipfile.ZIP_DEFLATED)
 apk.writestr('assets/build/source.json',json.dumps(receipt,indent=2)+'\n')
 apk.writestr('assets/build/BUILD.md',(repo/'docs/RECOVERY_BUILD.md').read_text())
 apk.writestr('assets/build/PROGRESS.md',(repo/'docs/RECOVERY_PROGRESS.md').read_text())
 apk.writestr('assets/notices/performance-candidate.txt','Private original ARM64 performance candidate. Menu and mission libraries are rebuilt together with optimized compilation; original game data is imported separately. See the matching source checkpoint and accompanying source/relink archive. FFmpeg n6.1.1, LGPL 2.1 or later. No proprietary game data is included in this APK.')
aligned=out/'mission-aligned.apk';run([tools/'zipalign','-f','-P','16','4',unsigned,aligned])
key=a.signing_key or out/'recovery-debug.keystore'
if not key.exists():run(['keytool','-genkeypair','-noprompt','-keystore',key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-dname','CN=Reigncarnated Private Mission Candidate','-keyalg','RSA','-keysize','2048','-validity','3650'])
key.chmod(0o600)
final=out/a.output_name;run([tools/'apksigner','sign','--ks',key,'--ks-pass','pass:android','--out',final,aligned]);run([tools/'apksigner','verify','--verbose',final]);run([tools/'zipalign','-c','-P','16','4',final]);run([tools/'aapt2','dump','badging',final])
with zipfile.ZipFile(final) as apk:
 assert apk.testzip() is None
 assert all(name in apk.namelist() for name in ['classes.dex','lib/arm64-v8a/libreign_menu.so','lib/arm64-v8a/libreign_mission.so'])
receipt['apk_sha256']=hashlib.sha256(final.read_bytes()).hexdigest();(out/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(final);print('SHA256',hashlib.sha256(final.read_bytes()).hexdigest())

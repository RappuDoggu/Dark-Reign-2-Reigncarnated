#!/usr/bin/env python3
"""Package the verified original menu binary with the separate mission engine.
Requires the exact private v05 APK, a newly linked ARM64 engine library, SDK 35,
and a JDK. Rebuilds all Java and uses a distinct application ID for device tests.
"""
import argparse,hashlib,subprocess,zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--base-apk',type=Path,required=True);p.add_argument('--native-library',type=Path,required=True);p.add_argument('--sdk',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
assert hashlib.sha256(a.base_apk.read_bytes()).hexdigest()=='1d0fa253b52d5d7200caa3d356a1f6b58f9d2fa7084f1cd61940581ef04bc598','The v05 menu binary/Java JNI contract must match'
platform=a.sdk/'platforms/android-35/android.jar';tools=a.sdk/'build-tools/35.0.0'
classes=out/'classes';dex=out/'dex';classes.mkdir(exist_ok=True);dex.mkdir(exist_ok=True)
def run(cmd):subprocess.run([str(x) for x in cmd],check=True)
run(['java','com.sun.tools.javac.Main','--release','8','-classpath',platform,'-d',classes,*sorted((repo/'android/menu/org/reigncarnated/menu').glob('*.java'))])
run([tools/'d8','--lib',platform,'--min-api','26','--output',dex,*sorted(classes.rglob('*.class'))])
unsigned=out/'mission-unsigned.apk';run([tools/'aapt2','link','-o',unsigned,'-I',platform,'--manifest',repo/'android/menu/AndroidManifest.xml'])
with zipfile.ZipFile(a.base_apk) as old,zipfile.ZipFile(unsigned,'a') as apk:
 for item in old.infolist():
  if item.filename.startswith('assets/') or item.filename=='lib/arm64-v8a/libreign_menu.so':apk.writestr(item.filename,old.read(item),compress_type=zipfile.ZIP_STORED if item.filename.endswith('.so') else zipfile.ZIP_DEFLATED)
 apk.write(a.native_library,'lib/arm64-v8a/libreign_mission.so',compress_type=zipfile.ZIP_STORED)
 for file in dex.glob('classes*.dex'):apk.write(file,file.name,compress_type=zipfile.ZIP_DEFLATED)
 apk.writestr('assets/notices/mission-candidate.txt','Private original ARM64 mission-engine candidate. The retained v05 native menu is unchanged; the mission renderer/runtime are in a separate library. See the matching source checkpoint and accompanying source/relink archive. FFmpeg n6.1.1, LGPL 2.1 or later. No proprietary game data is included in this APK.')
aligned=out/'mission-aligned.apk';run([tools/'zipalign','-f','-P','16','4',unsigned,aligned])
key=out/'mission-debug.keystore'
if not key.exists():run(['keytool','-genkeypair','-noprompt','-keystore',key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-dname','CN=Reigncarnated Private Mission Candidate','-keyalg','RSA','-keysize','2048','-validity','3650'])
final=out/'Reigncarnated-Instant-Action-v06-Candidate.apk';run([tools/'apksigner','sign','--ks',key,'--ks-pass','pass:android','--out',final,aligned]);run([tools/'apksigner','verify','--verbose',final]);run([tools/'zipalign','-c','-P','16','4',final]);run([tools/'aapt2','dump','badging',final])
with zipfile.ZipFile(final) as apk:
 assert apk.testzip() is None
 assert all(name in apk.namelist() for name in ['classes.dex','lib/arm64-v8a/libreign_menu.so','lib/arm64-v8a/libreign_mission.so'])
print(final);print('SHA256',hashlib.sha256(final.read_bytes()).hexdigest())

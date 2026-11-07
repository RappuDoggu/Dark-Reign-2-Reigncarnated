#!/usr/bin/env python3
"""Independently verify a signed 0.1 APK, exact source receipt and native ABI."""
import argparse,hashlib,json,re,struct,subprocess,zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['apk','sdk','ndk','output']:p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--source-revision',required=True);p.add_argument('--certificate',required=True)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];bt=a.sdk/'build-tools/35.0.0';llvm=a.ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin'
cert=subprocess.check_output([bt/'apksigner','verify','--print-certs',a.apk],text=True)
assert a.certificate in cert,'Unexpected update signer'
subprocess.run([bt/'zipalign','-c','-P','16','4',a.apk],check=True,capture_output=True)
badging=subprocess.check_output([bt/'aapt2','dump','badging',a.apk],text=True)
assert "name='org.reigncarnated.instantcity'" in badging and "versionName='0.1'" in badging and "versionCode='32'" in badging
assert "minSdkVersion:'26'" in badging and "targetSdkVersion:'35'" in badging
with zipfile.ZipFile(a.apk) as z:
 assert z.testzip() is None
 source=json.loads(z.read('assets/build/source.json'));assert source['source_revision']==a.source_revision
 for name,path in [('BUILD.md','RECOVERY_BUILD.md'),('PROGRESS.md','RECOVERY_PROGRESS.md'),('RELEASE_0_1.md','RELEASE_0_1.md'),('PLAYER_GUIDE.md','PLAYER_GUIDE.md'),('SOURCE_PROVENANCE.md','SOURCE_PROVENANCE.md')]:
  expected=subprocess.check_output(['git','show',a.source_revision+':docs/'+path],cwd=repo)
  assert z.read('assets/build/'+name)==expected,name
 for name in z.namelist():assert not name.lower().endswith(('.zwp','.bik','.xsi','.god','.keystore','.jks','.trace','.mp3','.wav')) and 'unstripped' not in name
 libs={}
 for name in ['libreign_menu.so','libreign_mission.so']:
  info=z.getinfo('lib/arm64-v8a/'+name);data=z.read(info);assert data[:5]==b'\x7fELF\x02' and struct.unpack_from('<H',data,18)[0]==183
  digest=hashlib.sha256(data).hexdigest();assert digest==source['native_libraries'][name]
  phoff=struct.unpack_from('<Q',data,32)[0];phsize,count=struct.unpack_from('<HH',data,54)
  for n in range(count):
   at=phoff+n*phsize
   if struct.unpack_from('<I',data,at)[0]==1:assert struct.unpack_from('<Q',data,at+48)[0]>=16384
  with a.apk.open('rb') as f:f.seek(info.header_offset+26);n,e=struct.unpack('<HH',f.read(4))
  assert (info.header_offset+30+n+e)%16384==0 and info.compress_type==zipfile.ZIP_STORED
  import tempfile
  with tempfile.TemporaryDirectory() as temp:
   lib=Path(temp)/name;lib.write_bytes(data)
   dynamic=subprocess.check_output([llvm/'llvm-readelf','-d',lib],text=True);assert 'libc++_shared' not in dynamic
   notes=subprocess.check_output([llvm/'llvm-readelf','-n',lib],text=True)
   libs[name]={'sha256':digest,'build_id':re.search(r'Build ID: (\w+)',notes).group(1)}
receipt={**source,'version_name':'0.1','version_code':32,'apk_bytes':a.apk.stat().st_size,'apk_sha256':hashlib.sha256(a.apk.read_bytes()).hexdigest(),'signer_certificate_sha256':a.certificate,'elf':libs,'verification':{'signature':True,'zip_crc':True,'zip_and_elf_16k':True,'arm64_static_libcxx':True,'embedded_source_and_documents':True,'no_keys_or_game_assets':True,'phone_execution_of_0_1':False}}
a.output.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['source_revision','version_name','version_code','apk_bytes','apk_sha256']},indent=2))

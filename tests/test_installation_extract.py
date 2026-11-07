#!/usr/bin/env python3
import io,pathlib,subprocess,sys,tarfile,tempfile,unittest
TOOL=pathlib.Path(__file__).resolve().parents[1]/'tools/extract_installation_7z.py'
class Extraction(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=pathlib.Path(self.tmp.name);self.archive=self.root/'fixture.tar';self.dest=self.root/'out'
 def pack(self,name='nested/test.bin',link=False):
  with tarfile.open(self.archive,'w') as t:
   info=tarfile.TarInfo(name)
   if link:info.type=tarfile.SYMTYPE;info.linkname='../outside';t.addfile(info)
   else:info.size=3;t.addfile(info,io.BytesIO(b'abc'))
 def run_tool(self):return subprocess.run([sys.executable,str(TOOL),str(self.archive),str(self.dest)],capture_output=True,text=True)
 def test_bytes(self):
  self.pack();r=self.run_tool();self.assertEqual(r.returncode,0,r.stderr);self.assertEqual((self.dest/'nested/test.bin').read_bytes(),b'abc')
 def test_traversal(self):
  self.pack('../outside');self.assertNotEqual(self.run_tool().returncode,0);self.assertFalse((self.root/'outside').exists())
 def test_link(self):
  self.pack('link',True);self.assertNotEqual(self.run_tool().returncode,0);self.assertFalse((self.dest/'link').exists())
 def test_existing_output(self):
  self.pack('kept');self.dest.mkdir();(self.dest/'kept').write_bytes(b'keep');self.assertNotEqual(self.run_tool().returncode,0);self.assertEqual((self.dest/'kept').read_bytes(),b'keep')
 def test_existing_partial(self):
  self.pack('kept');self.dest.mkdir();(self.dest/'kept.partial').write_bytes(b'keep');self.assertNotEqual(self.run_tool().returncode,0);self.assertEqual((self.dest/'kept.partial').read_bytes(),b'keep')
if __name__=='__main__':unittest.main()

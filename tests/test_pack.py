import importlib.util
from pathlib import Path
import struct
import unittest
spec = importlib.util.spec_from_file_location('pack', Path(__file__).resolve().parents[1] / 'tools/inspect_pack.py')
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)

def fixture():
    entry = struct.pack('<IB', 1, 8) + b'base.cfg' + struct.pack('<II', 56, 3)
    return struct.pack('<4s5I32x', b'NORK', 1, 1, 59 + len(entry), 1, 59) + b'abc' + entry + b'\x04data'

class PackTests(unittest.TestCase):
    def test_real_format_synthetic_content(self):
        result = pack.inspect(fixture())
        self.assertEqual(result['directories'], ['data'])
        self.assertEqual(result['files'], [dict(name='base.cfg', directory_index=1, offset=56, size=3)])

    def test_truncation_at_every_position(self):
        data = fixture()
        for n in range(len(data)):
            with self.subTest(length=n), self.assertRaises(pack.PackError):
                pack.inspect(data[:n])

    def test_invalid_signature_version_offsets_counts_and_ranges(self):
        for offset, value in [(0, 0), (4, 2), (8, 10000), (12, 0xffffffff),
                              (16, 10000), (20, 0), (59, 2), (72, 0), (76, 0xffffffff)]:
            data = bytearray(fixture())
            struct.pack_into('<I', data, offset, value)
            with self.subTest(offset=offset), self.assertRaises(pack.PackError):
                pack.inspect(data)

if __name__ == '__main__':
    unittest.main()

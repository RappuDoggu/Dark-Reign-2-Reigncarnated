#!/usr/bin/env python3
"""Verify private Ghidra menu evidence; stdout is metadata safe to commit.
This checks the pinned executable, complete decompilations and observed call
edges. It does NOT establish behavioral equivalence or an executable menu.
"""
import argparse
import hashlib
import json
from pathlib import Path

SHA = '5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250'
EXPECTED = {
    '005456d0': ['00558d20', '00548360', '0065d490', '0065bb20'],
    '00545890': ['005e7610', '005e7160'],
    '005455a0': ['00550480', '005e56c0', '005572e0'],
    '00545620': ['00558d20'],
    '00545840': ['005e7610', '005e7160'],
    '00545510': ['00550480', '005e56c0', '005572e0'],
}
ADDRESSES = set(EXPECTED) | {'00558d20', '00548360', '005e7610', '005e7160',
                            '00550480', '005e56c0', '005572e0'}

def require(condition, message):
    if not condition:
        raise ValueError(message)

def verify(directory, executable):
    require(hashlib.sha256(executable.read_bytes()).hexdigest() == SHA, 'Executable hash mismatch')
    report = json.loads((directory / 'menu-metadata.json').read_text())
    require(report['executable_sha256'] == SHA, 'Metadata hash mismatch')
    require(report['language'] == 'x86:LE:32:default', 'Language mismatch')
    require(report['engine_executed'] is False and report['menu_rendered'] is False, 'Unsupported runtime claim')
    rows = report['functions']
    require(len(rows) == len(ADDRESSES), 'Incomplete/duplicate function list')
    require({r['address'] for r in rows} == ADDRESSES, 'Unexpected function addresses')
    for row in rows:
        address = row['address']
        code = (directory / (address + '.c.txt')).read_text()
        require(row['decompiled'] is True and len(code) > 30, 'Decompilation failed: ' + address)
        require(len(code) == row['pseudocode_characters'], 'Pseudocode length mismatch: ' + address)
        if address in EXPECTED:
            require([r['target'] for r in row['direct_call_references']] == EXPECTED[address],
                    'Call graph mismatch: ' + address)
        row['pseudocode_sha256'] = hashlib.sha256(code.encode()).hexdigest()
    report['verification'] = 'PASS: 13 decompilations; six lifecycle call sequences'
    report['semantic_equivalence_proven'] = False
    return report

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory', type=Path)
    p.add_argument('executable', type=Path)
    args = p.parse_args()
    print(json.dumps(verify(args.directory, args.executable), indent=2))

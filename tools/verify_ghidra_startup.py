#!/usr/bin/env python3
"""Validate actual private Ghidra startup output against the supplied binary/source.

Does not publish pseudocode and does not establish REA provider readiness.
The inferred source identity is not a proof of whole-binary/source equivalence.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('binary',type=Path)
p.add_argument('upstream',type=Path)
p.add_argument('evidence',type=Path)
a=p.parse_args()
report=json.loads((a.evidence/'startup-metadata.json').read_text())
digest=hashlib.sha256(a.binary.read_bytes()).hexdigest()
if report['executable_sha256'] != digest or report['language'] != 'x86:LE:32:default':
    raise ValueError('Target identity/language mismatch')
rows=report['startup_candidates']
if len(rows)!=1 or not rows[0]['decompile_completed'] or rows[0]['error']:
    raise ValueError('Expected one successful startup decompilation')
row=rows[0]
code=(a.evidence/('startup-'+row['address']+'.c.txt')).read_text()
source=(a.upstream/'appdr2/winmain.cpp').read_text()
source=re.sub(r'/\*.*?\*/|//[^\n]*','',source,flags=re.S)
expected=re.findall(r'runCodes\.Register\("([^"\n]+)"', source)
observed=re.findall(r'\("(KeyCheck|Intro|Login|Shell|Mission|Outro|Studio)"', code)
if observed!=expected or sorted(expected)!=row['run_code_strings']:
    raise ValueError('Registration sequence does not match inspected WinMain source')
if len(code)!=row['pseudocode_characters'] or 'return 0;' not in code:
    raise ValueError('Missing or inconsistent pseudocode output')
print(json.dumps({'standalone_ghidra_decompile_verified':True,
    'rea_integration_verified':False,'target_sha256':digest,
    'source_file_sha256':hashlib.sha256((a.upstream/'appdr2/winmain.cpp').read_bytes()).hexdigest(),
    'function_address':row['address'],'source_identity':'WinMain (strong inference from seven ordered registrations)',
    'registration_order':observed,'pseudocode_characters':len(code),
    'full_binary_source_equivalence_established':False},indent=2))

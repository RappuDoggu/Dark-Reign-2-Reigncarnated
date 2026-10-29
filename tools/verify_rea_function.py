#!/usr/bin/env python3
"""Capture actual REA provider output without overwriting prior evidence.
This procedure has not yet run in Work. A successful exit alone does not
establish that a stripped binary function has been correctly identified.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
from datetime import datetime, timezone

p = argparse.ArgumentParser()
p.add_argument("target", type=pathlib.Path)
p.add_argument("function", help="Provider-confirmed function address or symbol; never guess")
p.add_argument("output", type=pathlib.Path, help="New directory outside existing analysis")
a = p.parse_args()
target = a.target.resolve(strict=True)
if not target.is_file():
    p.error("target must be a regular file")
a.output.mkdir(parents=True, exist_ok=False)
digest = hashlib.sha256()
with target.open("rb") as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
        digest.update(chunk)
report = {"created_utc": datetime.now(timezone.utc).isoformat(),
          "target": str(target), "sha256": digest.hexdigest(),
          "function_selector": a.function,
          "identity_confirmed": False, "commands": []}
commands = [
    ("version", ["rea", "--version"]),
    ("doctor", ["rea", "doctor", "--provider", "ghidra", "--json"]),
    ("providers", ["rea", "providers", "--json"]),
    ("function", ["rea", "function", str(target), a.function, "--provider", "ghidra", "--json"]),
    ("decompile", ["rea", "decompile", str(target), a.function, "--provider", "ghidra", "--json"]),
]
failed = False
for name, command in commands:
    record = {"name": name, "argv": command}
    with (a.output / (name + ".stdout")).open("wb") as out, (a.output / (name + ".stderr")).open("wb") as err:
        try:
            record["exit_code"] = subprocess.run(command, stdout=out, stderr=err, check=False).returncode
        except OSError as exc:
            record["launch_error"] = str(exc)
            record["exit_code"] = None
    report["commands"].append(record)
    (a.output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    if record["exit_code"] != 0:
        failed = True
        break
print(json.dumps(report, indent=2))
print("Review actual provider results and establish function identity before claiming integration.", file=sys.stderr)
sys.exit(1 if failed else 0)

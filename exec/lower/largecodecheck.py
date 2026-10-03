#!/usr/bin/env python3
"""Cross the former million-slot ARG/TXT boundary with real buffered code."""
import pathlib, subprocess, sys, tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]))
from tests.enc.tins import parse

assert len(sys.argv) == 3, 'usage: largecodecheck.py RUN LOWER.net'
count = 125010
with tempfile.TemporaryDirectory(prefix='lower-large-code-') as tmp:
    source = pathlib.Path(tmp) / 'large.tape'
    source.write_text('.str message "sentinel"\n_start:\n  .lea r2, message\n'
                      + '  mov r0, r1\n' * count + '  jump tail\ntail:\n  .exit r0\n')
    result = subprocess.run([sys.argv[1], sys.argv[2], str(source)],
                            capture_output=True, timeout=45)
    assert result.returncode == 0, result.stderr
    tape = parse(result.stdout.decode())
    assert sum(i.op == 'mov' and tuple(i.args) == ('x0', 'x1') for i in tape.code) == count
    assert any(i.op == '.lea' and tuple(i.args) == ('x2', 'message') for i in tape.code)
    assert any(i.op == 'jump' and tuple(i.args) == ('tail',) for i in tape.code)
    assert '_start' in tape.labels and 'tail' in tape.labels
    print('ARM lower: 125010 buffered mov rows, >1000000 ARG slots; text and labels intact')

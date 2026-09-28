#!/usr/bin/env python3
"""Quoted nested includes use the including directory; angle lookup stays -I."""
import os
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parent.parent
ua = os.environ['UA']
bound = str(root / 'tests/bound')
with tempfile.TemporaryDirectory(prefix='unisacc-quoted-include-') as folder:
    work = Path(folder)
    (work / 'nested').mkdir()
    (work / 'search').mkdir()
    (work / 'nested' / 'first.h').write_text('#include "leaf.h"\n')
    (work / 'nested' / 'leaf.h').write_text('#define LEAF 17\n')
    (work / 'leaf.h').write_text('#define LEAF 99\n')
    (work / 'choice.h').write_text('#define CHOICE 88\n')
    (work / 'search' / 'choice.h').write_text('#define CHOICE 13\n')
    (work / 'large.h').write_text('/*' + 'x' * 150000 + '*/\n#define LARGE 1\n')
    source = work / 'main.c'
    source.write_text('#include "nested/first.h"\n#include <choice.h>\n#include "large.h"\n'
                      'int main(void){ return LEAF + CHOICE + LARGE; }\n')
    for compiler in [ua, os.environ.get('CC', 'cc')]:
        output = work / ('native' if compiler != ua else 'ua')
        result = subprocess.run([bound, '15', compiler, '-I', str(work / 'search'),
                                 str(source), '-o', str(output)], capture_output=True, timeout=17)
        if result.returncode != 0:
            raise SystemExit(result.stderr.decode(errors='replace'))
        result = subprocess.run([bound, '5', str(output)], capture_output=True, timeout=7)
        if result.returncode != 31:
            raise SystemExit('nested/angle/large header: wrong exit ' + str(result.returncode))
    (work / 'too-large.h').write_bytes(b' ' * 4194304)
    source.write_text('#include "too-large.h"\nint main(void){return 0;}\n')
    result = subprocess.run([bound, '15', ua, str(source), '-o', str(work / 'invalid')],
                            capture_output=True, timeout=17)
    if result.returncode != 1 or b'included source too large' not in result.stderr:
        raise SystemExit('oversized include did not reject explicitly')
print('quoted includes: nested origin, angle -I, >128KB read and over-capacity reject ok')

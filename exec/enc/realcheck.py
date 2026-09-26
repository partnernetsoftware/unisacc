#!/usr/bin/env python3
"""Whole real lowering output -> delta text bytes; no instruction filtering.
Python remains the lowering producer and reference assembler. No image claim.
"""
import platform
import pathlib
import subprocess
import sys
import tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.__main__ import _oracle
from unisa.driver import compile_file
from unisa.lower import lower
from unisa.assemble import assemble
from unisa import image
from unisa.tape import DATA_BASE
from tins import dump


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: realcheck.py RUN DELTA.tbl')
    oracle = _oracle('built')
    with tempfile.TemporaryDirectory() as d:
        path = pathlib.Path(d) / 'program.txt'
        for target in ('lnx/x86_64', 'osx/x86_64'):
            for f in ('examples/hello.c', 'examples/fib.c'):
                tp = lower(compile_file([f], oracle, target), target, oracle, drive='built')
                path.write_text(dump(tp, full=True))
                ref, stats = assemble(tp)
                if stats['encoded'] != stats['insns']:
                    raise RuntimeError('reference contains unencoded instructions')
                r = subprocess.run([sys.argv[1], sys.argv[2], str(path)], capture_output=True, timeout=60)
                if r.returncode or r.stdout != ref:
                    at = next((i for i,(a,b) in enumerate(zip(r.stdout,ref)) if a!=b), min(len(r.stdout),len(ref)))
                    raise RuntimeError('%s %s rc %d first diff %d (%d/%d bytes): %s' % (target,f,r.returncode,at,len(r.stdout),len(ref),r.stderr.decode(errors='replace')))
                print('real encoding',target,f,stats['insns'],'instructions',len(ref),'bytes equal',flush=True)
                host = 'osx' if sys.platform == 'darwin' else 'lnx' if sys.platform.startswith('linux') and platform.machine() == 'x86_64' else None
                if target == str(host) + '/x86_64':
                    # Transitional wrapper only: lowering/layout metadata and image writer
                    # remain Python. The executable's complete code comes from the delta.
                    data = image.relocate(tp, tp.data, stats['data_va'] - DATA_BASE)
                    exe = pathlib.Path(d) / 'program'
                    exe.write_bytes(image.build(tp, r.stdout, data, stats['entry']))
                    exe.chmod(0o755)
                    if host == 'osx':
                        subprocess.run(['codesign','-f','-s','-',str(exe)],check=True,capture_output=True,timeout=60)
                    result = subprocess.run([str(exe)],capture_output=True,timeout=60)
                    want = b'hello from C99\n' if f.endswith('hello.c') else b'55\n'
                    if result.returncode or result.stdout != want or result.stderr:
                        raise RuntimeError('generated program result: ' + repr(result))
                    print('real execution',target,f,'expected output and exit 0',flush=True)



if __name__ == '__main__':
    main()

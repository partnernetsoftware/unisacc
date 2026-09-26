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
    if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3]!='arm64'):
        raise SystemExit('usage: realcheck.py RUN DELTA.tbl [arm64]')
    arch = 'arm64' if len(sys.argv)==4 else 'x86_64'
    oracle = _oracle('built')
    with tempfile.TemporaryDirectory() as d:
        path = pathlib.Path(d) / 'program.txt'
        for target in ('lnx/'+arch, 'osx/'+arch):
            for f in ('examples/hello.c', 'examples/fib.c'):
                tp = lower(compile_file([f], oracle, target), target, oracle, drive='built')
                if arch=='arm64' and target.startswith('osx'):
                    assert all(i.meta.get('gate')=='svc80' and i.meta.get('carry') is True for i in tp.code if i.op=='gate')
                path.write_text(dump(tp, full=True))
                ref, stats = assemble(tp)
                if stats['encoded'] != stats['insns']:
                    raise RuntimeError('reference contains unencoded instructions')
                r = subprocess.run([sys.argv[1], sys.argv[2], str(path)], capture_output=True, timeout=60)
                if r.returncode or r.stdout != ref:
                    at = next((i for i,(a,b) in enumerate(zip(r.stdout,ref)) if a!=b), min(len(r.stdout),len(ref)))
                    raise RuntimeError('%s %s rc %d first diff %d (%d/%d bytes): %s' % (target,f,r.returncode,at,len(r.stdout),len(ref),r.stderr.decode(errors='replace')))
                if arch=='arm64':
                    from unisa.emit_arm import size
                    offset=0;checked=0
                    for ins in tp.code:
                        if ins.op=='.lea' and ins.args[1] in tp.syms:
                            w0=int.from_bytes(r.stdout[offset:offset+4],'little')
                            w1=int.from_bytes(r.stdout[offset+4:offset+8],'little')
                            pages=((w0>>29)&3)|(((w0>>5)&0x7ffff)<<2)
                            if pages&(1<<20): pages-=1<<21
                            actual=((stats['text_va']+offset)&~4095)+pages*4096+((w1>>10)&4095)
                            assert actual==tp.syms[ins.args[1]]+stats['data_va']-DATA_BASE
                            checked+=1
                        offset+=size(ins,tp.labels)
                    assert checked>0, 'no ARM data address decoded'
                print('real encoding',target,f,stats['insns'],'instructions',len(ref),'bytes equal',flush=True)
                host = 'osx' if sys.platform == 'darwin' else 'lnx' if sys.platform.startswith('linux') and platform.machine() == arch else None
                if target == str(host) + '/'+arch:
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

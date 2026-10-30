#!/usr/bin/env python3
"""One native driver + selected unchanged networks, no APE outer container.
Run once per target, enabling bounded external parallel scheduling.
The frozen unified package is the authoritative input, not cached models.
"""
import argparse
import hashlib
import json
import os
import pathlib
import struct
import subprocess
import tempfile
from targetpackage import TARGETS, package_bytes, select_target, sha

ROOT=pathlib.Path(__file__).resolve().parents[2]

def native_kind(raw,target):
    osname,arch=target.split('/')
    if osname=='lnx':
        if raw[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',raw,18)[0] != (183 if arch=='arm64' else 62):
            raise ValueError('wrong native ELF ISA')
    elif osname=='osx':
        if raw[:4]!=b'\xcf\xfa\xed\xfe' or struct.unpack_from('<I',raw,4)[0] != (0x100000c if arch=='arm64' else 0x1000007):
            raise ValueError('wrong native Mach-O ISA')
    else:
        if raw[:2]!=b'MZ': raise ValueError('native PE required')
        off=struct.unpack_from('<I',raw,60)[0]
        if raw[off:off+4]!=b'PE\0\0' or struct.unpack_from('<H',raw,off+4)[0] != (0xaa64 if arch=='arm64' else 0x8664):
            raise ValueError('wrong native PE ISA')

def driver_identity():
    paths=sorted(set((ROOT/'exec/c').glob('*.c'))|set((ROOT/'exec/c').glob('*.h')))
    paths += sorted((ROOT/'include').glob('*.h'))+[ROOT/'src/version.h',ROOT/'src/host_dl.h']
    h=hashlib.sha256()
    for p in paths:
        h.update(p.relative_to(ROOT).as_posix().encode()+b'\0'+p.read_bytes()+b'\0')
    return h.hexdigest()

def build(source,target,via,output):
    original=pathlib.Path(source).read_bytes();package,ledger=select_target(package_bytes(original),target)
    via=pathlib.Path(via).resolve();output=pathlib.Path(output).resolve()
    identity=driver_identity();seed_sha=sha(via.read_bytes())
    if 'UNISA_SINGLE_TARGET' not in (ROOT/'exec/c/compiler.c').read_text():
        raise ValueError('single-target driver contract not installed')
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='unisacc-target-',dir=output.parent) as td:
        td=pathlib.Path(td);binary=td/'driver';pkg=td/'compiler.pkg'
        pkg.write_bytes(package)
        command=[str(via),str(ROOT/'exec/c/asmcompiler.c'),'-b',target,'-O2',
                 '-D',f'UNISA_SINGLE_TARGET="{target}"','-o',str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=50)
        raw=binary.read_bytes();native_kind(raw,target)
        binary.write_bytes(raw+package+b'UNIPKG1\n'+struct.pack('<Q',len(package)))
        binary.chmod(0o755)
        # The seed's Mach-O writer already emits its ad-hoc code signature.
        # The package is an overlay after that sealed driver; re-signing would
        # append a second signature after the footer, violating this format.
        # Enterprise outer sealing is a separate publishing operation.
        if driver_identity()!=identity or sha(via.read_bytes())!=seed_sha:
            raise ValueError('driver/seed changed during build')
        final=binary.read_bytes()
        if package_bytes(final)!=package: raise ValueError('native payload changed')
        ledger.update({'source_artifact_sha256':sha(original),'driver_source_sha256':identity,
                       'seed_sha256':seed_sha,'artifact_sha256':sha(final),'artifact_bytes':len(final),
                       'unsealed_driver_bytes':len(raw),'container':'native single format',
                       'ape_shell_bytes':0,'driver_target':target,'other_driver_images':0,
                       'verification':'format and selected content; native execution recorded separately'})
        os.replace(binary,output)
        output.with_suffix(output.suffix+'.json').write_text(json.dumps(ledger,sort_keys=True,indent=2)+'\n')
    return ledger

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--from-package',required=True,type=pathlib.Path)
    ap.add_argument('--target',required=True,choices=TARGETS)
    ap.add_argument('--via',required=True,type=pathlib.Path)
    ap.add_argument('-o',required=True,type=pathlib.Path)
    a=ap.parse_args()
    try: print(json.dumps(build(a.from_package,a.target,a.via,a.o),sort_keys=True))
    except (OSError,ValueError,subprocess.SubprocessError) as e: ap.exit(1,f'buildtarget: {e}\n')

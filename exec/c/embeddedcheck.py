#!/usr/bin/env python3
"""Bounded host execution of an APE with one generic network package.
No compiler semantics in the fixture; packaging uses Python, execution does not.
"""
import os, pathlib, subprocess, sys, tempfile
from net import convert
from tbl import CODE
from pack import build
ROOT = pathlib.Path(__file__).resolve().parents[2]
def call(args, **kw):
    r = subprocess.run(list(map(str,args)), capture_output=True, timeout=60, **kw)
    assert r.returncode == 0, (r.args,r.returncode,r.stderr)
    return r.stdout
with tempfile.TemporaryDirectory(prefix='embedded models ') as td:
    d=pathlib.Path(td)
    # Two containers share executable slices, but must select their own payload.
    images=[]
    for ch in (65,66):
        net=d/'m.net'
        net.write_text(convert(f'T 1 1 1 0 0\nQ 2 {CODE["OUT"]} {ch} {CODE["ACCEPT"]}\nR 0 0 0 0\n')[0])
        manifest=d/'route.tsv';manifest.write_text('demo\ta\tbytes\tbytes\tm.net\n')
        payload=build([manifest]);pkg=d/'models.pkg';pkg.write_bytes(payload)
        image=d/f'model {ch}.com'
        call([sys.executable,'-m','unisa','ape',ROOT/'exec/c/run.c','--via',os.environ.get('UA','/tmp/ua_ref'),'-O2','--payload',pkg,'-o',image],cwd=ROOT)
        body=image.read_bytes()
        assert body[-16:-8]==b'UNIPKG1\n'
        assert int.from_bytes(body[-8:],'little')==len(payload)
        assert body[-16-len(payload):-16]==payload and body.count(payload)==1
        images.append(image)
    for p in (net,manifest,pkg): p.unlink()
    inp=d/'input';inp.write_bytes(b'')
    # Empty PATH is unsuitable for the shell launcher (uname/gzip); no Python
    # or model files are consulted by the executable after shell extraction.
    for image,ch in zip(images,(65,66)):
        assert call(['sh',image,'--embedded','demo',inp],cwd=d)==bytes([ch])
    print('embedded package: two containers, one payload each, isolated host execution ok')

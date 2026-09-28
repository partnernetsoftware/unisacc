#!/usr/bin/env python3
"""Actual model compiler -> libSystem/libffi, both -run and native output.
System cc is not a runtime collector. MODEL_COM selects the frozen candidate.
"""
import os, pathlib, subprocess, sys, tempfile, time
ROOT=pathlib.Path(__file__).resolve().parents[2]
DEADLINE=time.monotonic()+53
helper=subprocess.check_output([str(ROOT/'tests/bound'),'--helper'],text=True,timeout=55).strip()
compiler=pathlib.Path(os.environ.get('MODEL_COM',str(ROOT/'unisacc.com'))).resolve()
def call(argv):
    seconds=min(20,int(DEADLINE-time.monotonic())-2)
    if seconds<1: raise RuntimeError('53-second product FFI budget exhausted')
    return subprocess.run([helper,str(seconds),*map(str,argv)],cwd=ROOT,capture_output=True,timeout=seconds+2)
def passing(argv):
    p=call(argv)
    if p.returncode or p.stdout.count(b'PASS ')!=17 or b'FAIL ' in p.stdout or p.stdout.count(b'END failures=0\n')!=1:
        raise RuntimeError((argv,p.returncode,p.stdout,p.stderr))
    return p.stdout
if sys.platform!='darwin': raise SystemExit('product FFI requires macOS; unavailable is not pass')
with tempfile.TemporaryDirectory(prefix='unisacc-ffi-product-') as td:
    d=pathlib.Path(td); native=d/'ffi'
    interpreted=passing([compiler,'-run','exec/ffi/probe.c'])
    p=call([compiler,'-O2','exec/ffi/probe.c','-o',native])
    if p.returncode or not native.is_file(): raise RuntimeError((p.returncode,p.stderr))
    if passing([native])!=interpreted: raise RuntimeError('native and -run FFI probe differ')
    # Reject wrong intrinsic arity before any execution; hostcall(0) must never jump.
    for name,expr in [('call','__hostcall(0)'),('addr','__hostaddr0(1)')]:
        c=d/(name+'.c'); out=d/(name+'.out')
        c.write_text('int main(void){return '+expr+';}\n')
        p=call([compiler,c,'-o',out])
        if p.returncode!=1 or out.exists() or not p.stderr.strip():
            raise RuntimeError((name,p.returncode,p.stderr,out.exists()))
print('product FFI: 17/17 -run and native; wrong intrinsic arity rejected')

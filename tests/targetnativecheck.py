#!/usr/bin/env python3
"""Bounded native single-target CLI equivalence, run only on its host OS/ISA.
Two probe units cover public tokens, warnings, optimisation and multi-file IO.
"""
import argparse
import hashlib
import json
import pathlib
import platform
import subprocess
import tempfile


def check(native,unified,target):
    host_os={'Darwin':'osx','Linux':'lnx','Windows':'win'}[platform.system()]
    host_arch='arm64' if platform.machine().lower() in ('arm64','aarch64') else 'x86_64'
    if target!=host_os+'/'+host_arch:
        raise ValueError('native test requires matching host OS/ISA; cross target is not a pass')
    native=pathlib.Path(native).resolve();unified=pathlib.Path(unified).resolve()
    def run(command,args,cwd):
        r=subprocess.run(command+args,cwd=cwd,timeout=10,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        return (r.returncode,r.stdout,r.stderr)
    uniform=['bash',str(unified)] if host_os!='win' else [str(unified)]
    ncmd=[str(native)];records=[]
    with tempfile.TemporaryDirectory(prefix='unisacc-native-equal-') as td:
        td=pathlib.Path(td);p=td/'probe.c';q=td/'second.c'
        p.write_text('#include <stdio.h>\nint main(void) { printf("value %d\\n",42); return 0; }\n')
        q.write_text('int other(void) { return 7; }\n')
        for flags in [['-run'],['-S'],['-E'],['-dump-tokens'],['-Wall','-run'],['-O1','-run'],['-O2','-run']]:
            n=run(ncmd,flags+[str(p)],td)
            uflags=flags if flags==['-dump-tokens'] or '-run' in flags else ['-b',target]+flags
            u=run(uniform,uflags+[str(p)],td)
            assert n==u and n[0]==0,(flags,n[0],n[2],u[0],u[2])
            records.append({'flags':flags,'bytes':len(n[1]),'sha256':hashlib.sha256(n[1]).hexdigest()})
        for flags in [['-S'],['-Wall','-S']]:
            args=flags+[str(p),str(q)]
            n=run(ncmd,args,td);u=run(uniform,['-b',target]+flags+[str(p),str(q)],td)
            assert n==u and n[0]==0,('multi',flags,n[0],n[2],u[0],u[2])
            records.append({'multi':flags,'bytes':len(n[1])})
        for level in ('-O0','-O1','-O2'):
            a,b=td/'native-image',td/'unified-image'
            n=run(ncmd,[str(p),level,'-o',str(a)],td)
            u=run(uniform,[str(p),'-b',target,level,'-o',str(b)],td)
            assert n[0]==u[0]==0 and a.read_bytes()==b.read_bytes(),('image',level,n,u)
            records.append({'image':level,'bytes':a.stat().st_size})
        foreign='win/arm64' if target!='win/arm64' else 'lnx/x86_64'
        r=run(ncmd,[str(p),'-S','-b',foreign],td)
        assert r[0]==1 and b'single-target' in r[2],('foreign target',r)
    return {'target':target,'native_sha256':hashlib.sha256(native.read_bytes()).hexdigest(),
            'unified_sha256':hashlib.sha256(unified.read_bytes()).hexdigest(),
            'execution':'native host','equal':records,'foreign_target_rejected':True}

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',required=True)
    ap.add_argument('--unified',required=True);ap.add_argument('--target',required=True)
    a=ap.parse_args();print(json.dumps(check(a.native,a.unified,a.target),sort_keys=True))

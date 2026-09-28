#!/usr/bin/env python3
"""Bounded private app adapter checks. Requires an already signed bundle.
No shared UA, no rebuilding the compiler, no claim of notarization.
"""
import argparse, hashlib, json, os, shutil, signal, subprocess, tempfile, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DEADLINE=time.monotonic()+55

def call(args, cwd, expected=0):
    budget=min(12,int(DEADLINE-time.monotonic())-2)
    if budget<1: raise RuntimeError('55-second qualification budget exhausted')
    p=subprocess.Popen(list(map(str,args)),cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try: out,err=p.communicate(timeout=budget)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);p.communicate();raise RuntimeError('bounded test timeout')
    if expected is not None and p.returncode!=expected: raise RuntimeError((args,p.returncode,out,err))
    return {'returncode':p.returncode,'stdout':out.decode(errors='replace'),'stderr':err.decode(errors='replace')}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--work',type=Path,required=True);ns=ap.parse_args()
    work=ns.work.resolve();app=work/'Unisacc.app';entry=app/'Contents/MacOS/unisacc'
    rec=json.loads((work/'qualification.json').read_text());checks={}
    with tempfile.TemporaryDirectory(prefix='unisacc-bundlecheck-') as td:
        d=Path(td);shutil.copytree(ROOT/'include',d/'include')
        checks['signature']=call(['/usr/bin/codesign','--verify','--strict','--verbose=2',app],d)
        source=d/'argument source.c'
        source.write_text('#include <stdio.h>\nint main(int n,char **v){printf("%d|%s|%s\\n",n,v[1],v[2]);return 37;}\n')
        tail=['-run',source,'space argument','dollar$quote"']
        direct=call(['/bin/sh',app/'Contents/Resources/unisacc.com',*tail],d,37)
        if direct['stdout']!='3|space argument|dollar$quote"\n':raise RuntimeError(direct)
        for arch in ['arm64','x86_64']:
            result=call(['/usr/bin/arch','-'+arch,entry,*tail],d,37)
            if result['stdout']!=direct['stdout']:raise RuntimeError('adapter changed arguments or output')
            checks['run_'+arch]=result
        out=d/'hello-native'
        checks['compile']=call([entry,source,'-o',out],d)
        checks['native']=call([out,'space argument','dollar$quote"'],d,37)
        if checks['native']['stdout']!=direct['stdout']:raise RuntimeError('native compilation output mismatch')
        ffi=d/'ffi.c';ffi.write_text('#include <unisacc_ffi.h>\n#include <stdio.h>\nint main(void){void *h;void *fn;char *s;int k[1];void *v[1];long n;h=uffi_dlopen("/usr/lib/libSystem.B.dylib",2);fn=uffi_dlsym(h,"strlen");s="sealed ffi";k[0]=UFFI_POINTER;v[0]=&s;n=0;if(uffi_call(fn,UFFI_ULONG,k,v,1,-1,&n)!=0)return 2;printf("%ld\\n",n);return n!=10;}\n')
        checks['ffi']=call([entry,'-run',ffi],d)
        if checks['ffi']['stdout']!='10\n':raise RuntimeError(checks['ffi'])
        clone=d/'Unisacc.app';shutil.copytree(app,clone)
        centry=clone/'Contents/MacOS/unisacc';payload=clone/'Contents/Resources/unisacc.com'
        original=payload.read_bytes();payload.unlink()
        checks['missing_resource']=call([centry,*tail],d,126)
        payload.write_bytes(original+b'\n')
        checks['tampered_seal']=call([centry,*tail],d,126)
        checks['tampered_signature']=call(['/usr/bin/codesign','--verify','--strict',clone],d,None)
        if checks['tampered_signature']['returncode']==0:raise RuntimeError('codesign accepted tampered resource')
        payload.write_bytes(original)
        manifest=clone/'Contents/Resources/payload.sha256';manifest.write_text('0'*64+'\n')
        checks['tampered_manifest']=call([centry,*tail],d,126)
        shutil.rmtree(clone);shutil.copytree(app,clone)
        checks['quarantine_mark']=call(['/usr/bin/xattr','-w','com.apple.quarantine','0083;00000000;UnisaccQualification;',clone],d)
        checks['quarantined_gatekeeper']=call(['/usr/sbin/spctl','-a','-t','exec','-vv',clone],d,None)
    digest=hashlib.sha256((app/'Contents/Resources/unisacc.com').read_bytes()).hexdigest()
    if digest!=rec['payload_sha256']:raise RuntimeError('tests changed payload')
    report={'schema':1,'actually_run':True,'checks':checks,'payload_sha256':digest,
            'signature_scope':rec.get('rehearsal_signature','Developer ID receipt must be checked separately'),
            'architecture_scope':'both launcher slices; the shell/APE chooses its existing host route',
            'trust':'pending: no notarization, staple, or user Gatekeeper launch acceptance established',
            'ffi_scope':'host libSystem strlen through unchanged APE -run on host arm64',
            'ape_cache_scope':'existing APE extraction behavior exercised; cache coldness not established'}
    (work/'runtime-checks.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({'checks':list(checks),'receipt':str(work/'runtime-checks.json'),'trust':'pending'}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Fixed-package model-driver bootstrap on macOS arm64; not a full APE rebuild.
MODEL_COM must explicitly name the finished model container. Run prepare,
bootstrap and probe separately against the same private output directory.
Python only snapshots, checks package framing, invokes tools and compares bytes.
"""
import argparse, hashlib, json, os, pathlib, platform, shutil, stat, subprocess, sys, time
R = pathlib.Path(__file__).resolve().parents[2]
START = time.monotonic()
PROBE = '#include <stdio.h>\nint fib(int n){return n<2?n:fib(n-1)+fib(n-2);}\nint main(void){printf("%d %u %d\\n",fib(10),4294967295u,(int)(unsigned char)257);return 0;}\n'

def digest(data): return hashlib.sha256(data).hexdigest()
def require(ok, why):
    if not ok: raise ValueError(why)
def call(args, env=None, okay=True):
    seconds=min(25,int(51-(time.monotonic()-START)))
    require(seconds>0,'step budget exhausted; invoke the next step separately')
    command=list(map(str,args))
    if env is not None:
        # Keep the watchdog's own process-discovery tools available; isolate its child.
        command=['/usr/bin/env','-u','UNISA_KERNEL','PATH=',*command]
    p=subprocess.run([sys.executable,str(R/'tests/bound.py'),str(seconds),*command],
                     capture_output=True,timeout=seconds+2,cwd=OUT)
    if okay: require(p.returncode==0,f'{args}: rc={p.returncode}, stderr={p.stderr[-1500:]!r}')
    return p

def package_check(data):
    from packageformat import read_package
    parsed=read_package(data)
    nm,ns,nr=len(parsed['models']),len(parsed['rows']),len(parsed['resources'])
    require(nm>0 and ns>0 and nr>0,'empty package')
    routes={row[1] for row in parsed['rows']};resources=parsed['resources']
    for suffix in ['image/O0','image/O2','run/O0','run/O2','memory']:
        require(('osx/arm64/'+suffix).encode() in routes,'missing bootstrap/probe route')
    kernel=resources.get(b'\0kernel/arm64',b'')
    require(len(kernel)>=40 and kernel[:8]==b'UNIKERN1','missing assembly kernel')
    require(any(k.startswith(b'\0hdr/') for k in resources),'missing carried headers')
    return {'networks':nm,'stage_rows':ns,'resources':nr,'kernel_sha256':digest(kernel)}

def source_files():
    files=sorted(p for p in (R/'exec/c').rglob('*') if p.suffix in ('.c','.h') and p.is_file())
    return [*files,R/'src/version.h']

def inputs():
    value=os.environ.get('MODEL_COM');require(value,'MODEL_COM is required; no reference fallback')
    candidate=pathlib.Path(value).resolve()
    require(stat.S_ISREG(candidate.stat().st_mode) and os.access(candidate,os.X_OK),'MODEL_COM must be executable')
    raw=candidate.read_bytes();require(raw[-16:-8]==b'UNIPKG1\n','missing embedded package footer')
    size=int.from_bytes(raw[-8:],'little');require(0<size<len(raw)-16,'package footer extent')
    pkg=raw[-16-size:-16]
    manifest={'candidate':str(candidate),'candidate_sha256':digest(raw),'candidate_bytes':len(raw),
              'package_sha256':digest(pkg),'package_bytes':len(pkg),'package':package_check(pkg),
              'source_files':{str(p.relative_to(R)):digest(p.read_bytes()) for p in source_files()},
              'test_sha256':digest(pathlib.Path(__file__).read_bytes())}
    return candidate,pkg,manifest

def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('step',choices=['prepare','bootstrap','probe'])
    ap.add_argument('output',type=pathlib.Path)
    args=ap.parse_args();OUT=args.output.resolve()
    require(platform.system()=='Darwin' and platform.machine()=='arm64','requires macOS arm64')
    candidate,pkg,manifest=inputs()
    if args.step=='prepare':
        OUT.mkdir(parents=True,exist_ok=True)
        require(not list(OUT.iterdir()),'prepare requires an empty private directory')
        for source in source_files():
            dest=OUT/'src'/source.relative_to(R);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest)
        (OUT/'compiler.pkg').write_bytes(pkg);(OUT/'probe.c').write_text(PROBE)
        (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        print('prepared',manifest['candidate_sha256'],manifest['package_sha256'],manifest['package']);return
    require(json.loads((OUT/'manifest.json').read_text())==manifest,'candidate/source/test changed; prepare a new directory')
    require((OUT/'compiler.pkg').read_bytes()==pkg,'snapshot package changed')
    for name,sha in manifest['source_files'].items():require(digest((OUT/'src'/name).read_bytes())==sha,'snapshot source changed: '+name)
    require((OUT/'probe.c').read_text()==PROBE,'probe source changed')
    source=OUT/'src/exec/c/asmcompiler.c';base=['--models',OUT/'compiler.pkg','-O2','-b','osx/arm64',source]
    clean=dict(os.environ,PATH='');clean.pop('UNISA_KERNEL',None)
    if args.step=='bootstrap':
        call(['sh',candidate,'-O2','-b','osx/arm64',source,'-o',OUT/'N1'])
        # Native child uses fixed networks and the package's assembly kernel.
        for old,new in (('N1','N2'),('N2','N3')):
            call([OUT/old,*base,'-o',OUT/new],clean)
        first=(OUT/'N1').read_bytes()
        require(first and all(first==(OUT/name).read_bytes() for name in ('N2','N3')),'N1/N2/N3 whole image bytes differ')
        result={'bytes':len(first),'sha256':digest(first),'N1_equals_N2_equals_N3':True,'manifest_sha256':digest((OUT/'manifest.json').read_bytes())}
        (OUT/'bootstrap.json').write_text(json.dumps(result,indent=2)+'\n');print('bootstrap',json.dumps(result));return
    result=json.loads((OUT/'bootstrap.json').read_text())
    require(result['manifest_sha256']==digest((OUT/'manifest.json').read_bytes()),'bootstrap inputs differ')
    for name in ('N1','N2','N3'):require(digest((OUT/name).read_bytes())==result['sha256'],'driver changed: '+name)
    call([os.environ.get('CC','cc'),OUT/'probe.c','-o',OUT/'host'])
    want=call([OUT/'host']).stdout;require(want,'empty host expectation');checks=[]
    for level in (0,2):
        opts=[OUT/'N3','--models',OUT/'compiler.pkg','-b','osx/arm64','-O'+str(level)]
        memory=call([*opts,'-run',OUT/'probe.c'],clean).stdout
        output=OUT/('probe-O'+str(level));call([*opts,OUT/'probe.c','-o',output],clean)
        native=call([output],clean).stdout;require(memory==native==want,'offspring differs from host')
        checks.append({'level':level,'stdout':memory.decode(),'image_sha256':digest(output.read_bytes())})
    missing=OUT/'missing.pkg'
    try:missing.stat()
    except FileNotFoundError:pass
    else:raise ValueError('missing-package test path unexpectedly exists')
    bad=call([OUT/'N3','--models',missing,'-run',OUT/'probe.c'],clean,okay=False)
    require(bad.returncode==2 and not bad.stdout and b'cannot open' in bad.stderr,'missing package did not reject explicitly')
    require(inputs()[2]==manifest and digest((OUT/'compiler.pkg').read_bytes())==manifest['package_sha256'],'inputs changed during probes')
    (OUT/'probe.json').write_text(json.dumps({'checks':checks,'missing_package_rc':bad.returncode},indent=2)+'\n')
    print('probe: N3 O0/O2 memory/native = host; empty PATH; missing package rc2; fixed-package driver only')

if __name__=='__main__':
    try:main()
    except (OSError,ValueError,KeyError,subprocess.TimeoutExpired) as error:
        raise SystemExit('modelboot: '+str(error))

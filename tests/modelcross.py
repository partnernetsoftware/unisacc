#!/usr/bin/env python3
"""Run an explicit finished MODEL_COM in one already-running guest.
Usage: MODEL_COM=/absolute/candidate.com python3 tests/modelcross.py lnx/arm64
Targets: lnx/arm64, lnx/x86_64, win/arm64, win/x86_64. No VM is started.
Exit 77 means unavailable/unexecuted, never pass. --status checks availability only.
Windows requires --probe hello/fib/convert in separate <=60s calls.
Windows APE runs its x86_64 PE driver; -run uses that default target. The separate
compile/native-execution probe uses the requested Windows target. This is model
product smoke coverage, not bootstrap or a six-platform completeness claim.
"""
import argparse, hashlib, io, json, os, pathlib, shlex, shutil, subprocess, sys, tarfile, tempfile, time, uuid
R = pathlib.Path(__file__).resolve().parents[1]
SOURCES = {
    'hello': '#include <stdio.h>\nint main(void){printf("hello from C99\\n");return 0;}\n',
    'fib': '#include <stdio.h>\nint fib(int n){return n<2?n:fib(n-1)+fib(n-2);}\nint main(void){printf("%d\\n",fib(10));return 0;}\n',
    'convert': '#include <stdio.h>\nint main(void){unsigned x=4294967295u;printf("%u %d %u\\n",x,(int)(unsigned char)257,(unsigned)(unsigned short)65537);return 0;}\n',
}

def run(args, seconds=15, data=None, okay=True):
    p = subprocess.run(['perl', str(R/'tests/bound.pl'), str(seconds), *map(str,args)],
                       input=data, capture_output=True, timeout=seconds+3)
    if okay and p.returncode:
        raise RuntimeError(f'{args}: rc={p.returncode} {p.stderr.decode(errors="replace")[-2000:]}')
    return p

def unavailable(message):
    print('UNEXECUTED: '+message, flush=True)
    raise SystemExit(77)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('target',choices=['lnx/arm64','lnx/x86_64','win/arm64','win/x86_64'])
    ap.add_argument('--status',action='store_true')
    ap.add_argument('--probe',choices=list(SOURCES))
    args=ap.parse_args()
    sources={args.probe:SOURCES[args.probe]} if args.probe else SOURCES
    value=os.environ.get('MODEL_COM')
    if not value: raise RuntimeError('MODEL_COM must name a finished candidate; no reference fallback')
    candidate=pathlib.Path(value).resolve()
    if not candidate.is_file() or not os.access(candidate,os.X_OK):
        raise RuntimeError('MODEL_COM missing or not executable: '+str(candidate))
    blob=candidate.read_bytes();digest=hashlib.sha256(blob).hexdigest()
    if blob[-16:-8]!=b'UNIPKG1\n': raise RuntimeError('candidate lacks embedded model package footer')
    size=int.from_bytes(blob[-8:],'little')
    if not 0<size<len(blob)-16 or not blob[-16-size:-16].startswith((b'P 1 ',b'P 2 ')):
        raise RuntimeError('invalid model package extent/header')
    print(json.dumps({'candidate':str(candidate),'sha256':digest,'bytes':len(blob),'target':args.target}),flush=True)
    linux=args.target.startswith('lnx/')
    if linux:
        vm=os.environ.get('LIMA_VM','default' if args.target.endswith('arm64') else 'minicon-lnx-x86_64')
        if not shutil.which('limactl'):unavailable('limactl unavailable')
        entries=[json.loads(x) for x in run(['limactl','list','--json'],5).stdout.splitlines()]
        entry=next((x for x in entries if x['name']==vm),None)
        arch='aarch64' if args.target.endswith('arm64') else 'x86_64'
        if not entry or entry['status']!='Running':unavailable(vm+' not running')
        if entry['arch']!=arch:unavailable(vm+' architecture does not match '+args.target)
    else:
        utm=pathlib.Path('/Applications/UTM.app/Contents/MacOS/utmctl')
        vm=os.environ.get('WINVM','minicon-win-arm-64')
        if not utm.is_file():unavailable('utmctl unavailable')
        status=run([utm,'status',vm],5,okay=False)
        if status.returncode or status.stdout.strip()!=b'started':unavailable(vm+' not started')
    if args.status:unavailable('status-only; probes not run')
    if not linux and args.probe is None:
        raise RuntimeError('Windows requires --probe hello|fib|convert for a bounded single-probe run')
    with tempfile.TemporaryDirectory(prefix='unisacc-modelcross-') as td:
        t=pathlib.Path(td);expected={}
        for name,source in sources.items():
            (t/(name+'.c')).write_text(source)
            run([os.environ.get('CC','cc'),t/(name+'.c'),'-o',t/name],15)
            expected[name]=run([t/name],5).stdout
            if not expected[name]:raise RuntimeError('empty host expectation: '+name)
        token=uuid.uuid4().hex
        if linux:
            remote='/tmp/unisacc-modelcross-'+token
            def shell(code,seconds=18,data=None):
                return run(['limactl','shell',vm,'--','sh','-c',code],seconds,data)
            archive=io.BytesIO()
            with tarfile.open(fileobj=archive,mode='w') as tar:
                for name,data in [('model.com',blob),*[(n+'.c',s.encode()) for n,s in sources.items()]]:
                    info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o700 if name=='model.com' else 0o600
                    tar.addfile(info,io.BytesIO(data))
            shell('mkdir '+shlex.quote(remote)+' && tar xf - -C '+shlex.quote(remote),20,archive.getvalue())
            try:
                actual=shell('sha256sum '+shlex.quote(remote+'/model.com'),5).stdout.split()[0].decode()
                if actual!=digest:raise RuntimeError('guest candidate hash differs')
                for name in sources:
                    prefix='cd '+shlex.quote(remote)+' && '
                    mem=shell(prefix+'timeout -k 2 12 sh ./model.com -run '+name+'.c').stdout
                    shell(prefix+'timeout -k 2 12 sh ./model.com -b '+args.target+' '+name+'.c -o '+name+'.bin && test -s '+name+'.bin')
                    native=shell(prefix+'chmod +x '+name+'.bin && timeout -k 2 8 ./'+name+'.bin').stdout
                    if mem!=expected[name] or native!=expected[name]:raise RuntimeError(f'{name}: output mismatch {mem!r} {native!r} expected {expected[name]!r}')
                    print(args.target,name,'model -run + guest model compile/native = host',flush=True)
            finally:shell('rm -rf '+shlex.quote(remote),5)
        else:
            stem=r'C:\u\modelcross-'+token
            def push(name,data):run([utm,'file','push',vm,stem+name],8,data)
            def pull(name):return run([utm,'file','pull',vm,stem+name],8).stdout
            try:
                push('.exe',blob)
                received=pull('.exe');actual=hashlib.sha256(received).hexdigest()
                if actual!=digest:
                    raise RuntimeError(f'guest candidate hash differs: expected {len(blob)} bytes {digest}; received {len(received)} bytes {actual}')
                for name,source in sources.items():
                    push('-'+name+'.c',source.encode())
                    # Synchronous exec gets enough budget; each guest child owns a shorter timeout.
                    script=r"""$ErrorActionPreference='Stop'
function Step($exe,$argv,$out,$seconds) {
 $opts=@{FilePath=$exe;RedirectStandardOutput=$out;RedirectStandardError=($out+'.err');PassThru=$true}
 if($argv.Count){$opts.ArgumentList=$argv}
 $p=Start-Process @opts
 if(-not $p.WaitForExit($seconds*1000)){
  $k=Start-Process taskkill -ArgumentList @('/PID',$p.Id,'/T','/F') -PassThru -WindowStyle Hidden
  if(-not $k.WaitForExit(2000)){$k.Kill()};throw 'timeout'
 }
 $p.WaitForExit();if($p.ExitCode -ne 0){throw ('rc='+$p.ExitCode)}
}
try {
 Step 'CAND.exe' @('-run','STEM.c') 'STEM.mem' 12
 Step 'CAND.exe' @('-b','TARGET','STEM.c','-o','STEM.bin.exe') 'STEM.compile' 12
 if((Get-Item 'STEM.bin.exe').Length -eq 0){throw 'empty image'}
 Step 'STEM.bin.exe' @() 'STEM.native' 8
 [IO.File]::WriteAllText('STEM.rc','0')
} catch { [IO.File]::WriteAllText('STEM.rc',$_);exit 1 }
""".replace('STEM',stem+'-'+name).replace('CAND',stem).replace('TARGET',args.target)
                    push('.ps1',script.encode())
                    deadline=time.monotonic()+40
                    run([utm,'exec',vm,'--hide','--cmd','powershell.exe','--','-NoProfile','-ExecutionPolicy','Bypass','-File',stem+'.ps1'],37)
                    # UTM may return before the guest completes: poll a unique per-probe result.
                    status=None
                    while time.monotonic()<deadline:
                        result=run([utm,'file','pull',vm,stem+'-'+name+'.rc'],2,okay=False)
                        if result.returncode==0:
                            status=result.stdout.strip();break
                        time.sleep(0.2)
                    if status!=b'0':raise RuntimeError('guest probe failed or timed out: '+repr(status))
                    mem,native=pull('-'+name+'.mem'),pull('-'+name+'.native')
                    # CRT text mode may turn stdout LF into CRLF on Windows.
                    if mem.replace(b'\r\n',b'\n')!=expected[name] or native.replace(b'\r\n',b'\n')!=expected[name]:raise RuntimeError(name+': output mismatch')
                    print(args.target,name,'APE x86_64 -run + requested-target guest compile/native = host',flush=True)
            finally:
                run([utm,'exec',vm,'--hide','--cmd','powershell.exe','--','-NoProfile','-Command',"Remove-Item -Force '"+stem+"*'"],8,okay=False)
    if hashlib.sha256(candidate.read_bytes()).hexdigest()!=digest:raise RuntimeError('candidate changed during validation')
    print('PASS',args.target,len(sources),'fixed programs, explicit candidate; no bootstrap claim',flush=True)

if __name__=='__main__':
    try:main()
    except (RuntimeError,subprocess.TimeoutExpired) as error:
        print('FAIL:',error,file=sys.stderr);raise SystemExit(1)

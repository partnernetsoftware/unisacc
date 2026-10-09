#!/usr/bin/env python3
"""Bounded host readiness, not product acceptance or a test-result cache."""
import argparse, hashlib, json, os, pathlib, platform, shutil, stat, subprocess, sys, tempfile, time
ROOT = pathlib.Path(__file__).resolve().parents[1]
CSMITH_OPTS = ['--no-packed-struct', '--no-bitfields', '--no-volatiles', '--max-funcs', '4', '--max-block-depth', '3']

def regular(p):
    try: return stat.S_ISREG(pathlib.Path(p).stat().st_mode)
    except FileNotFoundError: return False

def digest(p):
    h = hashlib.sha256()
    with pathlib.Path(p).open('rb') as f:
        for b in iter(lambda: f.read(65536), b''): h.update(b)
    return h.hexdigest()

def host():
    arch = platform.machine()
    return platform.system() + '/' + ('arm64' if arch in ('arm64', 'aarch64') else arch)

def native_target(observed=None):
    return {'Linux/x86_64':'lnx/x86_64', 'Linux/arm64':'lnx/arm64',
            'Darwin/x86_64':'osx/x86_64', 'Darwin/arm64':'osx/arm64'}.get(observed or host())

def matches(required, observed=None, rosetta=None):
    observed = observed or host()
    if required == 'any': return True
    if required == 'native-posix': return native_target(observed) is not None
    if required == 'darwin-arm64': return observed == 'Darwin/arm64'
    if required == 'darwin-x86_64':
        if observed == 'Darwin/x86_64': return True
        if observed != 'Darwin/arm64': return False
        if rosetta is not None: return rosetta
        r = subprocess.run([sys.executable, str(ROOT/'tests/bound.py'), '5',
                            '/usr/bin/arch', '-x86_64', '/usr/bin/true'], capture_output=True)
        return r.returncode == 0
    return False

def require_host(required, target='host'):
    if matches(required): return 0
    print('UNVERIFIED: required=%s observed=%s missing=execution-host target=%s' % (required,host(),target))
    return 77

def csmith_include(executable):
    explicit = os.environ.get('CSMITH_INCLUDE')
    if explicit:
        p = pathlib.Path(explicit)
        if not regular(p/'csmith.h'): raise FileNotFoundError('CSMITH_INCLUDE has no csmith.h')
        return p
    prefix = pathlib.Path(executable).resolve().parents[1]
    candidates = sorted((prefix/'include').glob('csmith-*')) + [prefix/'include/csmith', pathlib.Path('/usr/include/csmith')]
    for p in candidates:
        if regular(p/'csmith.h'): return p
    raise FileNotFoundError('csmith headers not found')

def header_identity(directory):
    files = sorted(pathlib.Path(directory).rglob('*.h'))
    if not files: raise FileNotFoundError('empty header directory')
    h = hashlib.sha256()
    for p in files:
        h.update(p.relative_to(directory).as_posix().encode()+b'\0'); h.update(bytes.fromhex(digest(p)))
    return h.hexdigest()

def tool_identity(executable, version):
    return {'sha256':digest(pathlib.Path(executable).resolve()), 'version_sha256':hashlib.sha256(version).hexdigest()}

class Checks:
    def __init__(self, seconds=50): self.deadline=time.monotonic()+seconds; self.cache={}; self.identities={}; self.memory={}
    def run(self, args, **kwargs):
        left=int(self.deadline-time.monotonic())
        if left < 1: raise TimeoutError('host precheck budget exhausted')
        return subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),str(min(10,left)),*map(str,args)],capture_output=True,**kwargs)
    def probe(self, kind):
        if kind in self.cache: return self.cache[kind]
        try: result=self._probe(kind)
        except FileNotFoundError: result='MISSING'
        except TimeoutError: result='UNKNOWN'
        self.cache[kind]=result
        return result
    def executable(self, name):
        command=name  # The declared suites invoke literal cc, not an unconsumed CC override.
        if name in ('clang','llvm-objcopy') and os.environ.get('LLVM_BIN'):
            command=str(pathlib.Path(os.environ['LLVM_BIN'])/name)
        exe=shutil.which(command)
        if not exe: raise FileNotFoundError(command)
        # csmith may write platform.info even during capability/version discovery.
        with tempfile.TemporaryDirectory(prefix='unisacc-host-version-') as td:
            r=self.run([exe,'--version'],cwd=td)
        if r.returncode: return None
        self.identities[name]=tool_identity(exe,r.stdout+r.stderr)
        return exe
    def _probe(self, kind):
        if kind == 'UNKNOWN': return 'UNKNOWN'
        if kind.startswith('seed-memory:'):
            import seedmemory
            family,names=seedmemory.SUITES[kind.split(':',1)[1]]
            rc,need,observed,basis=seedmemory.assess(family,names)
            self.memory[kind]={'required_bytes':need,'available_bytes':observed,'basis':basis}
            return {0:'READY',77:'UNVERIFIED',2:'UNKNOWN'}[rc]
        if kind == 'corpus':
            # Presence only; never a proof of corpus contents or test result freshness.
            return 'READY' if regular(ROOT/'corpus/c-testsuite/README.md') else 'MISSING'
        if kind not in ('cc','cc-arch-arm64','cc-arch-x86_64','clang-arch-x86_64','csmith','libffi','clang-coff','llvm-objcopy','python3'):
            return 'UNKNOWN'
        if kind == 'python3':
            exe=self.executable('python3')
            return 'READY' if exe and self.run([exe,'-c','pass']).returncode==0 else 'FAILED_PROBE'
        if kind == 'llvm-objcopy': return 'READY' if self.executable('llvm-objcopy') else 'FAILED_PROBE'
        if kind == 'clang-coff':
            exe=self.executable('clang')
            if not exe: return 'FAILED_PROBE'
            with tempfile.TemporaryDirectory(prefix='unisacc-hostcheck-') as td:
                src=pathlib.Path(td)/'p.c';src.write_text('int f(void){return 0;}\n')
                for target in ('x86_64-pc-windows-msvc','aarch64-pc-windows-msvc'):
                    if self.run([exe,'-target',target,'-c',src,'-o',pathlib.Path(td)/'p.o']).returncode: return 'FAILED_PROBE'
            return 'READY'
        cc=self.executable('clang' if kind.startswith('clang-arch-') else 'cc')
        if not cc: return 'FAILED_PROBE'
        with tempfile.TemporaryDirectory(prefix='unisacc-hostcheck-') as td:
            d=pathlib.Path(td);src=d/'p.c';flags=[]
            if kind == 'csmith':
                exe=self.executable('csmith')
                if not exe: return 'FAILED_PROBE'
                inc=csmith_include(exe);self.identities['csmith_headers']={'sha256':header_identity(inc)}
                generated=self.run([exe,'--seed','1',*CSMITH_OPTS],cwd=d)
                if generated.returncode: return 'FAILED_PROBE'
                src.write_bytes(generated.stdout);flags=['-I',str(inc)]
            elif kind == 'libffi':
                src.write_text('#include <ffi.h>\nint main(void){return ffi_type_void.size!=0;}\n');flags=['-lffi']
            else:
                src.write_text('int main(void){return 0;}\n')
                if kind.startswith(('cc-arch-','clang-arch-')): flags=['-arch',kind.split('-arch-',1)[1]]
            r=self.run([cc,'-std=c99',src,*flags,'-o',d/'p'])
            return 'READY' if r.returncode==0 else 'FAILED_PROBE'

def declarations(com=False):
    env=dict(os.environ,GATE_BOUND='1',TERM_SH_INSIDE='1')
    raw=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'15','sh',str(ROOT/'tests/gate.sh'),'--host-plan',*(['--com'] if com else [])],env=env,capture_output=True,check=True).stdout
    fields=raw.split(b'\0'); assert fields[-1]==b'';fields.pop()
    if len(fields)%2: raise ValueError('malformed host declarations')
    result={}
    for i in range(0,len(fields),2):
        name=fields[i].decode(); values=fields[i+1].decode().split('|')
        if name in result or len(values)!=5: raise ValueError('duplicate or malformed host declaration')
        result[name]=dict(zip(('required','target','dependencies','owner','probe'),values))
    names=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'15','sh',str(ROOT/'tests/gate.sh'),'--list',*(['--com'] if com else [])],env=env,capture_output=True,check=True).stdout.decode().splitlines()
    if set(names)!=set(result) or not names: raise ValueError('host declarations differ from gate list or empty')
    return result

def prediction(record, baseline, observed, identity):
    if baseline.get('host')!=observed or baseline.get('input_identity')!=identity: return 'UNKNOWN'
    seconds=record.get('wall_seconds');limit=record.get('limit_seconds')
    if not isinstance(seconds,(int,float)) or not isinstance(limit,(int,float)) or seconds<0 or limit<=0: return 'UNKNOWN'
    return 'PREDICT_TIMEOUT' if seconds>limit else 'WITHIN_BASELINE'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--com',action='store_true');p.add_argument('--suite',action='append');p.add_argument('--inventory',action='store_true');p.add_argument('--baseline',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path);p.add_argument('--require');p.add_argument('--target',default='host');a=p.parse_args()
    if a.require: return require_host(a.require,a.target)
    decl=declarations(a.com);names=a.suite or list(decl)
    if not names or any(n not in decl for n in names): p.error('empty or unknown suite selection')
    checks=Checks();rows=[]
    for name in names:
        d=decl[name]; status='UNKNOWN'
        if d['required']!='UNKNOWN':
            if not matches(d['required']): status='UNVERIFIED'
            elif not a.inventory:
                statuses=[checks.probe(k) for k in d['dependencies'].split(',')]
                status=next((k for k in ('FAILED_PROBE','MISSING','UNKNOWN','UNVERIFIED') if k in statuses),'READY')
        rows.append(dict(suite=name,**d,status=status))
        if d['dependencies'].startswith('seed-memory:'):
            rows[-1]['memory']=checks.memory.get(d['dependencies'],{'basis':'not probed'})
    inputs={'host':host(),'declarations':decl,'tools':checks.identities,
            'environment_sha256':{k:hashlib.sha256(os.environ.get(k,'').encode()).hexdigest() for k in ('CC','PATH','CSMITH_INCLUDE','LLVM_BIN','UA','MODEL_COM','SEED_DIR')}}
    # This binds readiness inputs, never stands in for gatequeue's full suite fingerprint.
    for key in ('UA','MODEL_COM'):
        value=os.environ.get(key)
        if value and regular(value): inputs[key]={'sha256':digest(value)}
    sd=os.environ.get('SEED_DIR')
    if sd:
        for name in ('unisacc-seed.com','unisacc-seed.com.build.json'):
            f=pathlib.Path(sd)/name
            if regular(f): inputs[name]={'sha256':digest(f)}
    identity=hashlib.sha256(json.dumps(inputs,sort_keys=True).encode()).hexdigest()
    baseline=json.loads(a.baseline.read_text()) if a.baseline else {}
    for row in rows: row['prediction']=prediction(baseline.get('suites',{}).get(row['suite'],{}),baseline,host(),identity)
    result={'schema':1,'host':host(),'input_identity':identity,'tools':checks.identities,'coverage':{'enumerated':len(decl),'selected':len(rows),'missing':0,'extra':0},'readiness':rows,'product_acceptance':'UNVERIFIED'}
    for row in rows: print('%s %s required=%s target=%s prediction=%s' % (row['suite'],row['status'],row['required'],row['target'],row['prediction']))
    print('hostcheck: checked=%d unknown=%d unverified=%d product_acceptance=UNVERIFIED' % (len(rows),sum(r['status']=='UNKNOWN' for r in rows),sum(r['status']=='UNVERIFIED' for r in rows)))
    if a.output:
        a.output.parent.mkdir(parents=True,exist_ok=True)
        with a.output.open('x') as f: json.dump(result,f,indent=2);f.write('\n')
    if any(r['status'] in ('FAILED_PROBE','MISSING') for r in rows): return 1
    if any(r['status'] in ('UNKNOWN','UNVERIFIED') for r in rows):
        print('UNVERIFIED: required=declared-ready-host observed='+host()+' missing=host-or-dependency-obligation target=declared')
        return 77
    return 0

if __name__=='__main__': sys.exit(main())

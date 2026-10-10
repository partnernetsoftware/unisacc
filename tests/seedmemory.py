#!/usr/bin/env python3
"""Fresh seed-suite memory admission; UNKNOWN is rc2, insufficient known memory rc77.
Gen RSS: research/c39-a5-gen-memory.md, current key 6ec1fe9d67dd0733, isolated O2.
Parse2 key and shared e3/errorparse RSS unchanged.
RSS KiB floored to MiB; add one MiB per route before summing.
Final -w/no-w builds byte-identical; four-worker groups sum serial route peaks.
Cgroup accounting peaks and ASan diagnostics are separate from these RSS values.
Python cold construction and unmeasured flags/COM remain UNKNOWN.
"""
import argparse, hashlib, pathlib, stat, subprocess, sys, platform, shutil, re, tempfile, os
ROOT=pathlib.Path(__file__).resolve().parents[1]
C_SHA='b802e4f2d2d7f10ad13dd9392792e085de2dfdcd30fd2c2f0c8bbd42d9190cdb'
REFERENCE_KEYS={'gen':'6ec1fe9d67dd0733','parse2':'3d19910f75b9932c'}
BINARY_SHA='f997b25d8f8391e12034871bd14e20523fa6cc88720cadb3bc6a71a41c591e64'
# Historical host-adapter receipt; cc independently confirmed both flag sets
# construct this exact binary. No other argv is assumed equivalent.
EVIDENCE_IDENTITY={'host':'Linux/x86_64',
 'cc_sha256':'a23ecab8ff08f09ad8c80602c2c5df7f49e09c25905cb8975902e101bf72635f',
 'cc_version':'cc (Debian 14.2.0-19) 14.2.0', 'parallelism':4,'cache_branch':'warm'}
LAUNCHER_PATH=pathlib.Path('/tmp/cdx37-host-tools/cc')
LAUNCHER_SHA='5c036d429e38725565e20f4f4989afad0e11284144616287e939eb42239ccdc3'
LAUNCHER_TARGET=pathlib.Path('/usr/bin/cc')
# Only this reviewed byte-for-byte script is recognised; never infer targets
# by executing or generally parsing arbitrary shell wrappers.
ENV_KEYS=('CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','OBJC_INCLUDE_PATH',
 'GCC_EXEC_PREFIX','COMPILER_PATH','LIBRARY_PATH','LD_PRELOAD','LD_LIBRARY_PATH',
 'GCC_SPECS','CFLAGS','CPPFLAGS','LDFLAGS')
EQUIVALENT_FLAGS=('-std=c99 -O2 -w -Iseed','-std=c99 -O2 -Iseed')
C_RSS={'e2':184,'e1':330,'e3':1255,'e4':40,'o1':14,'prune':32,
 'nativeabi':298,'lower-lnx-x':192,'lower-lnx-a':215,'lower-osx-x':185,'lower-osx-a':210,'lower-win-x':130,
 'lower-win-a':157,'errorparse':2120,'warnparse':2135,'warnunits':196,'warnlex':741,'tokenlex':119,
 'tokenpp':38,'warnpp':180,'obj-lower-x':195,'obj-lower-a':219,'obj-enc-x':156,'obj-enc-a':159,
 'enc-elf':142,'arm-elf':142,'enc-macho':208,'arm-macho':208,'enc-pe':263,'arm-pe':264}
SUITES={
 'seedparse2-1':('parse2',['x','locations','warnings','errors']),
 'seedparse2-2':('parse2',['locations,warnings','locations,errors','warnings,errors','locations,warnings,errors']),
 'seedgen':('gen','e2 e1 e3 e4 o1 prune nativeabi enc-elf enc-macho enc-pe arm-elf arm-macho arm-pe'.split()),
 'seedgen-2':('gen','lower-lnx-x lower-lnx-a lower-osx-x lower-osx-a lower-win-x lower-win-a obj-lower-x obj-lower-a obj-enc-x obj-enc-a'.split()),
 'seedgen-3':('gen','tokenpp tokenlex warnlex'.split()),
 'seedgen-4':('gen','warnparse warnunits errorparse warnpp'.split()),
 'com-seedgen':('com',[])}
DEFAULTS={'parse2':SUITES['seedparse2-1'][1]+SUITES['seedparse2-2'][1],
 'gen':sum((SUITES[n][1] for n in ('seedgen','seedgen-2','seedgen-3','seedgen-4')),[]),'com':[]}

def available(meminfo=pathlib.Path('/proc/meminfo'),cgroup=pathlib.Path('/proc/self/cgroup'),mountinfo=pathlib.Path('/proc/self/mountinfo')):
    mem=int(next(l.split()[1] for l in meminfo.read_text().splitlines() if l.startswith('MemAvailable:')))*1024
    member=next(l.split(':',2)[2] for l in cgroup.read_text().splitlines() if l.startswith('0::'))
    mount=next(l.split() for l in mountinfo.read_text().splitlines() if ' - cgroup2 ' in l)
    # mount root can itself be a delegated subgroup.
    relative=pathlib.PurePosixPath(member).relative_to(pathlib.PurePosixPath(mount[3]))
    base=pathlib.Path(mount[4]);current=base/relative;observed=mem
    while True:
        try: maximum=(current/'memory.max').read_text().strip()
        except FileNotFoundError:
            if current!=base:raise
            maximum='max'  # cgroup2 root has no memory.max by definition
        if maximum!='max':
            used=int((current/'memory.current').read_text().strip())
            observed=min(observed,max(0,int(maximum)-used))
        if current==base:break
        current=current.parent
    return observed

def reference_key(family):
    # Exact existing suite cache key, not a second cache protocol.
    files='exec/assemble.py exec/build/*.py exec/parse2/* exec/facts/*.tsv' if family=='parse2' else 'exec/assemble.py exec/finite_rules.py exec/build/*.py exec/*/*.tsv exec/facts/*.tsv weights/gold/*.tsv'
    r=subprocess.run(['sh','-c','cat '+files+' 2>/dev/null | shasum | cut -c1-16'],cwd=ROOT,capture_output=True,text=True,timeout=10,check=True)
    return r.stdout.strip()

def execution_identity(family):
    script=ROOT/'tests'/('seedparse2check.sh' if family=='parse2' else 'seedgencheck.sh')
    source=script.read_text()
    flags=re.search(r"(?m)^GEN_CC_FLAGS='([^']+)'$",source)
    workers=re.search(r'(?m)^GEN_PARALLELISM=([0-9]+)$',source)
    if not flags or not workers:return None
    cc=shutil.which('cc')
    if not cc:return None
    command=pathlib.Path(cc)
    entity=command.resolve(strict=True)
    command_sha=hashlib.sha256(entity.read_bytes()).hexdigest()
    launcher=None
    if command_sha!=EVIDENCE_IDENTITY['cc_sha256']:
        if command.absolute()!=LAUNCHER_PATH or command_sha!=LAUNCHER_SHA:return None
        launcher={'path':str(LAUNCHER_PATH),'sha256':command_sha,'target':str(LAUNCHER_TARGET)}
        entity=LAUNCHER_TARGET.resolve(strict=True)
        if hashlib.sha256(entity.read_bytes()).hexdigest()!=EVIDENCE_IDENTITY['cc_sha256']:return None
        cc=str(LAUNCHER_TARGET)
    environment={k:os.environ.get(k,'') for k in ENV_KEYS}
    if any(environment.values()):return None
    with tempfile.TemporaryDirectory(prefix='seed-memory-identity-') as td:
        version=subprocess.run([cc,'--version'],cwd=td,capture_output=True,text=True,timeout=5)
    if version.returncode or not version.stdout.splitlines():return None
    return {'host':platform.system()+'/'+platform.machine(),
      'cc_sha256':hashlib.sha256(entity.read_bytes()).hexdigest(),
      'cc_version':version.stdout.splitlines()[0], 'flags':flags.group(1),
      'parallelism':int(workers.group(1)), 'launcher':launcher,
      'environment':environment,'compile_resources':'UNKNOWN'}

def identity_reason(identity,branch):
    if identity is None:return 'execution identity UNKNOWN'
    launcher=identity.get('launcher')
    if launcher is not None and launcher!={'path':str(LAUNCHER_PATH),'sha256':LAUNCHER_SHA,'target':str(LAUNCHER_TARGET)}:
        return 'launcher route differs from reviewed evidence'
    if any(identity.get('environment',{}).values()):return 'unproved compiler/loader environment'
    for key in ('host','cc_sha256','cc_version','parallelism'):
        if identity.get(key)!=EVIDENCE_IDENTITY[key]:return 'memory evidence identity differs: '+key
    if identity.get('flags') not in EQUIVALENT_FLAGS:return 'unproved compiler flags'
    if branch!=EVIDENCE_IDENTITY['cache_branch']:return 'unmeasured cache branch: '+branch
    return None

def verify_binary(path):
    try: actual=hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
    except FileNotFoundError:return 2
    return 0 if actual==BINARY_SHA else 2

def requirement(family,names,tmpdir=None):
    if family=='com':return None,'unmeasured COM construction/generator route'
    if hashlib.sha256((ROOT/'seed/gen.c').read_bytes()).hexdigest()!=C_SHA:return None,'C source differs from measured route'
    try: identity=execution_identity(family)
    except (FileNotFoundError,subprocess.TimeoutExpired):return None,'execution identity UNKNOWN'
    reason=identity_reason(identity,'warm')
    if reason:return None,reason
    names=names or DEFAULTS[family]
    peaks=[]
    for name in names:
        route={'x':'e3','errors':'errorparse'}.get(name) if family=='parse2' else name
        if route not in C_RSS:return None,'unmeasured C flags: '+name
        peaks.append((C_RSS[route]+1)*1024**2)
    import os
    cache=pathlib.Path(tmpdir or os.environ.get('TMPDIR','/tmp'))/('unisacc-seedparse2' if family=='parse2' else 'unisacc-seedgen')
    key=reference_key(family)
    if key!=REFERENCE_KEYS[family]:return None,'generator inputs differ from measured route'
    branch='warm';cold_name=None
    for name in names:
        f=cache/('py-'+key+'-'+name+'.json')
        try: info=f.stat()
        except FileNotFoundError:branch='cold';cold_name=name;break
        if not stat.S_ISREG(info.st_mode) or info.st_size==0:branch='cold';cold_name=name;break
    reason=identity_reason(identity,branch)
    if reason:return None,reason+': '+str(cold_name)
    parallelism=identity['parallelism']
    peak=max(sum(peaks[i:i+parallelism]) for i in range(0,len(peaks),parallelism))
    return peak+max((peak+3)//4,512*1024**2),'generator execution RSS + max(25%,512MiB); warm Python references; compile_resources=UNKNOWN'

def assess(family,names):
    need,basis=requirement(family,names)
    if need is None:return 2,need,None,basis
    try: observed=available()
    except (FileNotFoundError,ValueError,StopIteration):return 2,need,None,'memory availability/cgroup UNKNOWN'
    return (77 if observed<need else 0),need,observed,basis

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('family',choices=DEFAULTS);p.add_argument('names',nargs='*');p.add_argument('--verify-binary',type=pathlib.Path);a=p.parse_args()
    if a.verify_binary:
        rc=verify_binary(a.verify_binary)
        reason='generator-binary-identity'
        if not rc:
            need,reason=requirement(a.family,' '.join(a.names).split())
            if need is None:rc=2
        if rc:print('UNKNOWN: required=memory observed=unknown missing=memory-evidence reason='+reason)
        return rc
    rc,need,observed,basis=assess(a.family,' '.join(a.names).split())
    if rc==77:print('UNVERIFIED: required=mem>='+str(need)+'B observed='+str(observed)+'B missing=memory')
    elif rc==2:print('UNKNOWN: required=memory observed=unknown missing=memory-evidence reason='+basis)
    return rc
if __name__=='__main__':sys.exit(main())

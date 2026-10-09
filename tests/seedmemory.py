#!/usr/bin/env python3
"""Fresh seed-suite memory admission; UNKNOWN is rc2, insufficient known memory rc77.
C RSS: matrixA2, cc -std=c99 -O2 seed/gen.c, same 8ba73354 source.
Printed MiB were truncated: add one MiB to each measured value before summing.
Python cold construction and unmeasured flags/COM remain UNKNOWN.
"""
import argparse, hashlib, pathlib, stat, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
C_SHA='8ba7335401883852044fdae6de212d67cc1cae84ed2ff5cadfcdf5c283dc4d9d'
REFERENCE_KEYS={'gen':'e8d10ecb97107e7b','parse2':'3d19910f75b9932c'}
C_RSS={'e2':184,'e1':330,'e3':1255,'e4':40,'o1':14,'prune':32,'nativeabi':298,
 'lower-lnx-x':192,'lower-lnx-a':215,'lower-osx-x':185,'lower-osx-a':211,
 'lower-win-x':130,'lower-win-a':157,'errorparse':2120,'warnparse':2135,
 'warnunits':196,'warnlex':742,'tokenlex':119,'tokenpp':38,'warnpp':180,
 'obj-lower-x':196,'obj-lower-a':219,'obj-enc-x':156,'obj-enc-a':159,
 'enc-elf':141,'arm-elf':142,'enc-macho':208,'arm-macho':208,'enc-pe':263,'arm-pe':264}
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

def requirement(family,names,tmpdir=None):
    if family=='com':return None,'unmeasured COM construction/generator route'
    if hashlib.sha256((ROOT/'seed/gen.c').read_bytes()).hexdigest()!=C_SHA:return None,'C source differs from measured route'
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
    for name in names:
        f=cache/('py-'+key+'-'+name+'.json')
        try: info=f.stat()
        except FileNotFoundError:return None,'unmeasured cold Python reference: '+name
        if not stat.S_ISREG(info.st_mode) or info.st_size==0:return None,'unmeasured cold Python reference: '+name
    peak=max(sum(peaks[i:i+4]) for i in range(0,len(peaks),4))
    return peak+max((peak+3)//4,512*1024**2),'measured C group RSS + max(25%,512MiB); warm Python references'

def assess(family,names):
    need,basis=requirement(family,names)
    if need is None:return 2,need,None,basis
    try: observed=available()
    except (FileNotFoundError,ValueError,StopIteration):return 2,need,None,'memory availability/cgroup UNKNOWN'
    return (77 if observed<need else 0),need,observed,basis

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('family',choices=DEFAULTS);p.add_argument('names',nargs='*');a=p.parse_args()
    rc,need,observed,basis=assess(a.family,' '.join(a.names).split())
    if rc==77:print('UNVERIFIED: required=mem>='+str(need)+'B observed='+str(observed)+'B missing=memory')
    elif rc==2:print('UNKNOWN: required=memory observed=unknown missing=memory-evidence reason='+basis)
    return rc
if __name__=='__main__':sys.exit(main())

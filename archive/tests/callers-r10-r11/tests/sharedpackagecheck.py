#!/usr/bin/env python3
"""Check shared E2 package audit, resource declarations and routed outputs.
Prepared package/table pairs are explicit inputs. Each child has its own bound.
"""
import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'exec/c'))
from packageformat import read_package
from pack import compact_q

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def normalized(raw):return tuple(tuple(line.split()) for line in raw.splitlines())
def check(pkg,audit,run,driver,reference):
    package=read_package(pkg.read_bytes());records=json.loads((audit/'models.json').read_text())['pairs']
    assert records, 'empty model audit'
    signatures=set()
    for name,row in records.items():
        assert digest(audit/(name+'.net'))==row['network_sha256']
        assert digest(audit/(name+'.tbl'))==row['table_sha256']
        signatures.add(normalized(compact_q((audit/(name+'.net')).read_bytes())))
    assert {normalized(m) for m in package['models']}==signatures, 'package/audit network set differs'
    spec=importlib.util.spec_from_file_location('shared_pp',ROOT/'exec/pp/gen.py');pp=importlib.util.module_from_spec(spec);spec.loader.exec_module(pp)
    assert {k:v for k,v in package['resources'].items() if k.startswith(b'\0predefines/')}==pp.predefine_resources()
    indices={int(r[5]) for r in package['rows'] if r[2]==b'e2'}
    assert len(indices)==2, ('plain/located shared networks expected',indices)
    def netcheck(name):
        r=subprocess.run([str(run),'--check-net',str(audit/(name+'.tbl')),str(audit/(name+'.net'))],timeout=25,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert r.returncode==0,(name,r.returncode,r.stderr)
        return r.stdout.decode().strip()
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as ex:
        checks=list(ex.map(netcheck,sorted(records)))
    with tempfile.TemporaryDirectory(prefix='shared-package-source-') as td:
        td=Path(td);probe=td/'target.c'
        probe.write_text('''#ifdef __linux__
linux
#endif
#ifdef __APPLE__
apple
#endif
#ifdef _WIN32
windows
#endif
#ifdef __aarch64__
arm
#endif
#ifdef __x86_64__
x86
#endif
__UNISA__ __LP64__
''')
        for target in [o+'/'+a for o in ('lnx','osx','win') for a in ('arm64','x86_64')]:
            actual=subprocess.run([str(driver),'--models',str(pkg),'-b',target,'-E',str(probe)],cwd=td,timeout=10,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            expected=subprocess.run([str(reference),'-b',target,'-E',str(probe)],cwd=td,timeout=10,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            assert actual.returncode==expected.returncode==0,(target,actual.stderr,expected.stderr)
            assert actual.stdout==expected.stdout,target
        for target in ['osx/arm64','win/x86_64']:
            args=['-b',target,'-dump-tokens',str(probe)]
            a=subprocess.run([str(driver),'--models',str(pkg),*args],cwd=td,timeout=10,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            b=subprocess.run([str(reference),'-dump-tokens',str(probe)],cwd=td,timeout=10,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            assert a.returncode==b.returncode==0 and a.stdout==b.stdout,('tokens target invariant',target,a.stderr,b.stderr)
    result={'models':len(package['models']),'audit_pairs':len(records),'package_bytes':pkg.stat().st_size,
            'predefinitions_bytes':sum(map(len,pp.predefine_resources().values())),
            'e2_models':len(indices),'target_outputs_equal':6,'public_tokens_equal':2,'full_domain_checks':checks}
    print(json.dumps(result,sort_keys=True))
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ['package','audit','run','driver','reference']:ap.add_argument('--'+name,required=True,type=Path)
    a=ap.parse_args();check(a.package,a.audit,a.run,a.driver,a.reference)

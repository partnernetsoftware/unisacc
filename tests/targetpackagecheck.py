#!/usr/bin/env python3
"""Real P3 six-target closure and exact shard recombination/corruption gate."""
import pathlib
import sys
import tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'exec/c'))
from targetpackage import TARGETS, package_bytes, select_target, export_shards, import_shards, write_package
from packageformat import read_package

def rejected(fn):
    try: fn()
    except (ValueError,FileNotFoundError): return
    raise AssertionError('bad input accepted')

def check(path):
    raw=package_bytes(path.read_bytes());source=read_package(raw)
    assert write_package(source)==raw
    with tempfile.TemporaryDirectory(prefix='unisacc-target-check-') as td:
        td=pathlib.Path(td);manifest=export_shards(raw,td)
        assert import_shards(td)==raw
        n=manifest['networks'][0]['sha256']+'.network';p=td/n;stored=p.read_bytes()
        p.write_bytes(stored[:-1]);rejected(lambda:import_shards(td));p.write_bytes(stored)
        p.unlink();rejected(lambda:import_shards(td));p.write_bytes(stored)
        for target in TARGETS:
            data,ledger=select_target(raw,target);p=read_package(data)
            wanted=[r for r in source['rows'] if r[1].startswith((target+'/').encode()) or
                    not any(r[1].startswith((t+'/').encode()) for t in TARGETS)]
            mapping=ledger['retained_models']
            assert len(p['rows'])==len(wanted)
            assert {r[1] for r in p['rows']}=={r[1] for r in wanted}
            for before,after in zip(wanted,p['rows']):
                assert before[:5]==after[:5]
                assert p['models'][int(after[5])]==source['models'][int(before[5])]
                assert p['wires'][int(after[5])]['stored']==source['wires'][int(before[5])]['stored']
            assert b'tokens' in {r[1] for r in p['rows']}
            assert {k for k in p['resources'] if k.startswith(b'\0kernel/')}=={b'\0kernel/'+target.split('/')[1].encode()}
            assert {k:v for k,v in p['resources'].items() if not k.startswith(b'\0kernel/')}=={
                k:v for k,v in source['resources'].items() if not k.startswith(b'\0kernel/')}
            sub=td/target.replace('/','-');export_shards(data,sub);assert import_shards(sub)==data
            print(target,ledger['networks'],ledger['package_bytes'],'bytes unchanged closure',flush=True)
        rejected(lambda:select_target(raw,'bad/x86_64'))
        only,_=select_target(raw,TARGETS[0]);rejected(lambda:select_target(only,TARGETS[-1]))
    print('target package: six closures, exact recombination, corruption and missing shard reject')

if __name__=='__main__':
    check(pathlib.Path(sys.argv[1]) if len(sys.argv)==2 else ROOT/'unisacc.com')

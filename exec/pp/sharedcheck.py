#!/usr/bin/env python3
"""Private experiment: unchanged legacy E2 vs target-resource E2.
Run one format per <=60s window; plain/located/no-autoinc are not merged.
No product package or driver is modified by this check.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
sys.path.insert(0,str(ROOT/'exec/c'))
sys.path.insert(0,str(HERE))
from pack import build as package, compressed_model, compact_q
from gen import build, predefine_resources, sizes

TARGETS=[o+'/'+a for o in ('lnx','osx','win') for a in ('arm64','x86_64')]

def serialize(g,path):
    path.write_text(json.dumps({'start':'START','states':{k:[m,{str(a):list(v) for a,v in row.items()}] for k,(m,row) in g.st.items()},'seqs':[list(map(list,s)) for s in g.seqs]}))

def call(args,timeout=15,success=True):
    r=subprocess.run(list(map(str,args)),timeout=timeout,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if success and r.returncode: raise AssertionError((args,r.returncode,r.stderr[:500]))
    return r

def check(locations,output):
    output.mkdir(parents=True,exist_ok=True)
    call(['cc','-O2',ROOT/'exec/c/run.c','-o',output/'run'],timeout=25)
    shared=build(locations=locations,shared_predefines=True);serialize(shared,output/'shared.json')
    for extension,tool in [('tbl','tbl.py'),('net','net.py')]:
        source=output/('shared.json' if extension=='tbl' else 'shared.tbl')
        call([sys.executable,ROOT/'exec/c'/tool,source,output/('shared.'+extension)])
    netcheck=call([output/'run','--check-net',output/'shared.tbl',output/'shared.net'])
    resources=output/'resources';resources.mkdir(exist_ok=True)
    for key,value in predefine_resources().items():
        p=resources/key[1:].decode();p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(value)
    selector=resources/'cli/target';selector.parent.mkdir(exist_ok=True)
    probes={
      'predefs':''.join('#ifdef '+n+'\n'+n+'\n#else\nabsent_'+n+'\n#endif\n' for n in sorted({s for v in predefine_resources().values() for s in v.decode().split('\0') if s})),
      'undef':'#undef __UNISA__\n#ifdef __UNISA__\nbad\n#else\nok\n#endif\n',
      'expression':'#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)\nint present;\n#endif\n',
      'stdio':'#include <stdio.h>\nint main(void) { printf("hi %d\\n", 7); return 0; }\n',
      'macro':'#define F(x) (x + 1)\nint a=F(2);\n'}
    records=[];old_bytes=0;old_compressed_bytes=0
    for target in TARGETS:
        selector.write_bytes(target.encode())
        legacy=build(target,locations=locations);serialize(legacy,output/'legacy.json')
        call([sys.executable,ROOT/'exec/c/tbl.py',output/'legacy.json',output/'legacy.tbl'])
        call([sys.executable,ROOT/'exec/c/net.py',output/'legacy.tbl',output/'legacy.net'])
        old_text=(output/'legacy.net').read_bytes()
        old_bytes+=len(old_text)
        old_compressed_bytes+=len(compressed_model(compact_q(old_text),cache=False)[0])
        # Exact target argument independence is a construction-level check.
        candidate=build(target,locations=locations,shared_predefines=True)
        assert candidate.st==shared.st and candidate.seqs==shared.seqs, target
        for stem in ('legacy','shared'):
            manifest=output/(stem+'.tsv');manifest.write_text('pp\te2\tsrc.c\t'+('pp.locations' if locations else 'pp.text')+'\t'+stem+'.net\n')
            (output/(stem+'.pkg')).write_bytes(package([manifest],[('00',resources),('006864722f',ROOT/'include')]))
        for name,text in probes.items():
            source=output/(name+'.c');source.write_text(text)
            a=call([output/'run','--bundle',output/'legacy.pkg','pp',source,source,ROOT/'include'])
            b=call([output/'run','--bundle',output/'shared.pkg','pp',source,source,ROOT/'include'])
            assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),(target,name)
            records.append([target,name,len(b.stdout)])
        print(target,len(probes),'outputs equal',flush=True)
    source=output/'macro.c';valid=selector.read_bytes()
    for bad in [b'',b'unknown/arch']:
        selector.write_bytes(bad)
        manifest=output/'shared.tsv';(output/'bad.pkg').write_bytes(package([manifest],[('00',resources),('006864722f',ROOT/'include')]))
        r=call([output/'run','--bundle',output/'bad.pkg','pp',source],success=False)
        assert r.returncode==1 and b'predefinition' in r.stderr,(bad,r.returncode,r.stderr)
    selector.write_bytes(valid)
    names=resources/'predefines'/valid.decode();original=names.read_bytes()
    for bad in [b'',b'9invalid\0',b'NAME',b'A\0A\0']:
        names.write_bytes(bad)
        (output/'bad.pkg').write_bytes(package([output/'shared.tsv'],[('00',resources),('006864722f',ROOT/'include')]))
        r=call([output/'run','--bundle',output/'bad.pkg','pp',source],success=False)
        assert r.returncode==1 and b'predefinition' in r.stderr,(bad,r.returncode,r.stderr)
    names.write_bytes(original)
    result={'format':'located' if locations else 'plain','autoinc':os.environ.get('E2_AUTOINC','1')!='0',
            'states':len(shared.st),'six_legacy_net_bytes':old_bytes,'shared_net_bytes':(output/'shared.net').stat().st_size,
            'predefine_resource_bytes':sum(map(len,predefine_resources().values())),
            'six_legacy_p3_model_bytes':old_compressed_bytes,
            'shared_p3_model_bytes':len(compressed_model(compact_q((output/'shared.net').read_bytes()),cache=False)[0]),
            'netcheck':netcheck.stdout.decode(),'equal':records,'bad_resource_rejections':6}
    (output/'result.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='equal'},sort_keys=True))

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--locations',action='store_true');ap.add_argument('--output',required=True,type=Path)
    a=ap.parse_args();check(a.locations,a.output)

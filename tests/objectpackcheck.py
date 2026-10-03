#!/usr/bin/env python3
"""Route-construction unit test; models/package IO are stubbed, not a full build."""
import pathlib, sys, tempfile, types
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'exec/c'))
PATH=ROOT/'exec/c/compilerpack.py'
def module(source,name):
    m=types.ModuleType(name);m.__file__=str(PATH);exec(compile(source,str(PATH),'exec'),m.__dict__);return m

def exercise(m,td):
    captured=[];calls=[]
    def model(directory,name,script,args,env=None):
        calls.append((name,pathlib.Path(script).relative_to(ROOT).as_posix(),args))
        return pathlib.Path(directory)/(name+'.net')
    def build(manifests,mounts,**kw):
        captured.extend(pathlib.Path(manifests[0]).read_text().splitlines());return b'fixture'
    m.built_model=model;m.build=build
    manifests=[]
    for target in ('lnx/x86_64','lnx/arm64','osx/x86_64','osx/arm64','win/x86_64','win/arm64'):
        p=td/(target.replace('/','-')+'.tsv');p.write_text(''.join(target+'\t'+stage+'\tin\tout\t'+stage+'.net\n' for stage in ('e2','e1','e3','e4','prune','lower','elf')));manifests.append(p)
    assert m.compiler_package(manifests,td/'o1.net',td,shared_e2=td/'sharedpp.net',shared_nativeabi=td/'nativeabi.net')==b'fixture'
    rows=[r.split('\t') for r in captured]
    for row in rows:row[4]=pathlib.Path(row[4]).name
    return rows,calls

def main():
    current=module(PATH.read_text(),'objectpack_current')
    source=PATH.read_text()
    eligibility="if target in ('lnx/x86_64','lnx/arm64'):"
    assert source.count(eligibility)==1
    baseline=module(source.replace(eligibility,'if False:'),'objectpack_without_object_specs')
    with tempfile.TemporaryDirectory(prefix='unisacc-objectpack-') as tmp:
        td=pathlib.Path(tmp);rows,calls=exercise(current,td);oldrows,_=exercise(baseline,td)
    def isobject(row):return '/object/' in row[0]
    assert [r for r in rows if not isobject(r)]==oldrows,'existing routes changed'
    routes={r[0]:[x for x in rows if x[0]==r[0]] for r in rows if isobject(r)}
    assert len(routes)==24, len(routes)
    for target in ('lnx/x86_64','lnx/arm64'):
        arch=target.split('/')[1]
        for prefix in ('','warn/','multi/','warn/multi/'):
            for level in ('O0','O1','O2'):
                route=target+'/'+prefix+'object/'+level;rs=routes[route]
                assert rs[-2][1]=='lower' and rs[-2][4]=='object-lower-'+arch+'.net',route
                assert rs[-1][1:4]==['elf','in','object-v1'] and rs[-1][4]=='object-enc-'+arch+'.net',route
                assert sum(r[1]=='e4' for r in rs)==(level!='O0'),route
                parse=next(r for r in rs if r[1]=='e3')
                assert parse[2]=='tokens.locations' and parse[4]==('warnparse.net' if prefix.startswith('warn/') else 'errorparse.net'),route
    assert all(r.startswith('lnx/') for r in routes),'unfinished OS route advertised'
    for arch in ('x86_64','arm64'):
        assert ('object-lower-'+arch,'exec/build/gen.py',['lower','--full','--object']+(['--arm64'] if arch=='arm64' else [])) in calls
        assert ('object-enc-'+arch,'exec/enc/'+('arm.py' if arch=='arm64' else 'gen.py'),['--object']) in calls
    print('object pack declarations: 24 routes; original routes unchanged; stubbed construction only')
if __name__=='__main__':main()

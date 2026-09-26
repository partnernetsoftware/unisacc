#!/usr/bin/env python3
"""Exercise production inference, including holes and negative output weights.
Optional .tbl arguments are exhaustively checked too; no generated model cache.
Each compiler/executor invocation has its own 60-second bound.
"""
import os, pathlib, subprocess, sys, tempfile
from net import convert
from tbl import CODE
from pack import build as package_build
ROOT = pathlib.Path(__file__).resolve().parents[2]

def run(args):
    return subprocess.run(list(map(str,args)), capture_output=True, timeout=60)

def require(r):
    assert r.returncode == 0, (r.args,r.returncode,r.stdout,r.stderr)
    return r

with tempfile.TemporaryDirectory(prefix='unisacc-net-') as td:
    d=pathlib.Path(td); exe=d/'run'
    build=[os.environ.get('EXEC_CC','cc'),'-O2','-Wall','-Wextra','-o',exe,ROOT/'exec/c/run.c']
    if os.environ.get('CORE_EXTERNAL')=='1': build += ['-DUNISA_CORE_EXTERNAL',ROOT/'exec/c/core.c']
    require(run(build))
    # Byte -> non-state stack symbol -> register observation -> acceptance.
    # Unspecified byte keys stay missing, even between real transitions.
    src=('T 4 4 1 0 0\n'
         f'Q 1 {CODE["PUSH"]} 9\n'
         f'Q 2 {CODE["LDI"]} 0 2 {CODE["RLD"]} 0\n'
         f'Q 2 {CODE["OUT"]} 89 {CODE["ACCEPT"]}\n'
         f'Q 1 {CODE["ACCEPT"]}\n'
         'R 0 2 -1 0 65 1 0 256 3 3\n'
         'R 1 2 -1 0 -1 3 3 9 2 1\n'
         'R 2 1 -1 0 2 3 2\n'
         'R 0 0 -1 0\n')
    table=d/'tiny.tbl';table.write_text(src)
    for i,path in enumerate([table,*map(pathlib.Path,sys.argv[1:])]):
        out,_,_=convert(path.read_text());net=d/(str(i)+'.net');net.write_text(out)
        r=require(run([exe,'--check-net',path,net]));sys.stdout.write(r.stdout.decode())
        if i: continue
        for data,status,stdout in [(b'A',0,b'Y'),(b'B',2,b''),(b'',0,b'')]:
            inp=d/'input';inp.write_bytes(data)
            a=run([exe,table,inp]);b=run([exe,net,inp])
            assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr)
            assert b.returncode==status and b.stdout==stdout
        lines=out.splitlines();index=next(j for j,s in enumerate(lines) if s.startswith('H '))
        words=lines[index].split();words[5]=str(int(words[5])+1);lines[index]=' '.join(words)
        bad=d/'bad.net';bad.write_text('\n'.join(lines)+'\n')
        assert run([exe,'--check-net',table,bad]).returncode != 0,'mutated weight passed'
    # Exact signed 64-bit decoding, observable through the production OFILL.
    acts=[('LDI',0,-9223372036854775808),('ORES',1,20),('OFILL',1,0,20),('OUT',10),
          ('LDI',0,9223372036854775807),('ORES',1,20),('OFILL',1,0,20),('OUT',10),('ACCEPT',)]
    tokens=' '.join(' '.join(map(str,(CODE[a[0]],*a[1:]))) for a in acts)
    source='T 1 1 2 0 0\nQ 9 '+tokens+'\nR 0 0 0 0\n'
    wide=d/'wide.net';wide.write_text(convert(source)[0]);inp=d/'empty';inp.write_bytes(b'')
    r=require(run([exe,wide,inp]))
    assert r.stdout==b'-9223372036854775808\n 9223372036854775807\n',r.stdout
    for old,new in [('-9223372036854775808','-9223372036854775809'),('9223372036854775807','9223372036854775808')]:
        wide.write_text(convert(source.replace(old,new))[0])
        r=run([exe,wide,inp]);assert r.returncode==2 and b'overflow' in r.stderr,(r.returncode,r.stderr)
    # A pipeline is a sequence of arbitrary byte-stream transducers. No C
    # stage name is built into it. Compare same-process and fresh-process runs.
    def model(name, acts, rows='R 0 0 0 0', strings=()):
        tokens=' '.join(' '.join(map(str,(CODE[a[0]],*a[1:]))) for a in acts)
        body=f'T 1 1 8 {len(strings)} 0\n'+''.join('S '+v.hex()+'\n' for v in strings)
        path=d/(name+'.net')
        path.write_text(convert(body+f'Q {len(acts)} '+tokens+'\n'+rows+'\n')[0])
        return path
    # Leave nonzero registers, indexed memory, a stack entry, an intern and
    # a blob behind. Every subsequent invocation must begin with fresh state.
    reset=model('reset', [('ORES',2,1),('OFILL',2,3,1),('LDX',1,0,100),
        ('ORES',2,1),('OFILL',2,1,1),('LDI',3,7),('STX',0,100,3),
        ('SBCLR',),('SBOUT',97),('SBSAVE',4),('SBINTERN',5),
        ('ORES',2,1),('OFILL',2,4,1),('ORES',2,1),('OFILL',2,5,1),
        ('PUSH',9),('ACCEPT',)], 'R 1 1 -1 0 -1 0 0')
    empty=model('empty-output',[('ACCEPT',)])
    reject=model('reject',[('OUT',88),('REJECT',0)],strings=(b'stopped',))
    loop=model('loop',[('LDI',0,0)])
    inp=d/'input';inp.write_bytes(bytes([0,128,255,10]))
    argv=[exe,'--chain',inp,inp,d]
    for stages,want in [([reset]*20,b'0021'),([empty,reset],b'0021'),([reset,empty],b'')]:
        got=require(run([*argv,*stages]));assert got.stdout==want and not got.stderr
    # A failed stage does not publish an accepted prefix or run a later model.
    missing=d/'must-not-be-opened.net'
    r=run([*argv,reset,reject,missing]);assert (r.returncode,r.stdout,r.stderr)==(1,b'',b'reject: stopped\n')
    r=run(['env','UNISA_MAXSTEPS=2',*argv,reset,loop,missing])
    assert (r.returncode,r.stdout,r.stderr)==(3,b'',b'timeout\n')
    r=run([*argv]);assert r.returncode==2 and not r.stdout
    manifest=d/'routes.tsv'
    manifest.write_text('first\ta\tbytes\tbytes\treset.net\nfirst\tb\tbytes\tbytes\treset.net\nsecond\ta\tbytes\tbytes\tempty-output.net\nthird\ta\tbytes\tbytes\treset.net\n')
    data=package_build([manifest]);assert data.startswith(b'P 1 2 4\n')
    assert data.count(reset.read_bytes())==1, 'shared model duplicated'
    pack=d/'models.pkg';pack.write_bytes(data)
    for route,want in [('first',b'0021'),('second',b''),('third',b'0021')]:
        r=require(run([exe,'--bundle',pack,route,inp]));assert r.stdout==want and not r.stderr
    r=run([exe,'--bundle',pack,'absent',inp]);assert r.returncode==2 and not r.stdout
    # Both construction and runtime enforce the declared format boundary.
    manifest.write_text(manifest.read_text().replace('first\tb\tbytes','first\tb\twrong'))
    try: package_build([manifest])
    except ValueError as e: assert 'format mismatch' in str(e)
    else: raise AssertionError('bad manifest accepted')
    for bad in [data[:-1],data.replace(b'D first b bytes',b'D first b wrong'),
                data.replace(b'D first b bytes',b'D first a bytes'),
                data.replace(b'bytes bytes 0',b'bytes bytes 9',1)]:
        pack.write_bytes(bad);r=run([exe,'--bundle',pack,'first',inp])
        assert r.returncode==2 and not r.stdout,(r.returncode,r.stderr)
    resources=d/'resources';resources.mkdir()
    (resources/'data').write_bytes(b'\0\x80\n');(resources/'empty').write_bytes(b'')
    acts=[('SBCLR',)]+[('SBOUT',c) for c in b'\0data']
    acts += [('SBFIND',0),('SBFIND',1),('ORES',2,1),('OFILL',2,0,1),('ORES',2,1),('OFILL',2,1,1),
             ('INPUSH',0),('COPY',),('ADV',),('COPY',),('ADV',),('COPY',),('INPOP',),('SBCLR',)]
    acts += [('SBOUT',c) for c in b'\0empty']
    acts += [('SBFIND',0),('BLEN',1,0),('ORES',2,1),('OFILL',2,0,1),('ORES',2,1),('OFILL',2,1,1),('ACCEPT',)]
    model('resources',acts)
    manifest.write_text('resource\ta\tbytes\tbytes\tresources.net\nresource\tb\tbytes\tbytes\tresources.net\n')
    data=package_build([manifest],[('00',resources),('00',resources)])
    assert data.startswith(b'P 2 1 2 2\n'), 'identical mounted resources not shared'
    pack.write_bytes(data)
    r=require(run([exe,'--bundle',pack,'resource',inp]));assert r.stdout==b'22\0\x80\n30',r.stdout
    # Packaged bytes take precedence over disk; missing mount/conflicts fail.
    other=d/'other';other.mkdir();(other/'data').write_bytes(b'wrong')
    for mounts in [[('00',resources),('00',other)],[('00',d/'absent')]]:
        try: package_build([manifest],mounts)
        except ValueError: pass
        else: raise AssertionError('invalid mount accepted')
    for bad in [data[:-1],data.replace(b'F 6 0\n',b'F 6 9\n')]:
        pack.write_bytes(bad);r=run([exe,'--bundle',pack,'resource',inp])
        assert r.returncode==2 and not r.stdout,(r.returncode,r.stderr)
    # A container footer locates the same package without copying model bodies.
    container = b'MZ-container-prefix' + data + b'UNIPKG1\n' + len(data).to_bytes(8,'little')
    pack.write_bytes(container)
    r=require(run([exe,'--bundle',pack,'resource',inp]));assert r.stdout==b'22\0\x80\n30'
    env=dict(os.environ,UNISA_CONTAINER=str(pack))
    r=subprocess.run([str(exe),'--embedded','resource',str(inp)],capture_output=True,timeout=60,env=env)
    require(r);assert r.stdout==b'22\0\x80\n30'
    for bad in [container[:-1],container[:-16]+b'BADMAGIC'+container[-8:],
                container[:-8]+bytes(8),container[:-8]+bytes([255])*8]:
        pack.write_bytes(bad);r=run([exe,'--bundle',pack,'resource',inp])
        assert r.returncode==2 and not r.stdout,(r.returncode,r.stderr)
    print('network check: ok (inference, stream reset/failures, package bounds, resources, container footer)')

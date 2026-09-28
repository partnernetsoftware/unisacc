#!/usr/bin/env python3
"""Private prepared prune JSON/TBL/NET/core qualification, every child bounded."""
import json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT)]
import sim
from pack import build
from unisa.prune import prune_text
U=lambda n:struct.pack('<Q',n)
DESC=struct.pack('<6Q',0,4,0,1,8,0)
def metadata(records):
    out=b'USLSIG1\n'+U(len(records))
    for name,link,supported in records:
        name=name.encode();out+=U(len(name))+name+bytes([link,1,0])+U(0)+DESC+U(0)+bytes([supported])
    return out
PRO='  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 0\n'
TAPE=("""_start:
  call main
  .exit r0
main:
"""+PRO+'  ret\npublic_fn:\n'+PRO+'  ret\nhidden_fn:\n'+PRO+'  ret\nfuture_fp:\n'+PRO+'  ret\n__init:\n'+PRO+'  .lea r0, address_fn\n  ret\naddress_fn:\n'+PRO+'  ret\n').encode()
def main(args):
    js,tbl,net,core=map(pathlib.Path,args);delta=json.loads(js.read_text());loaded=sim.load(delta)
    def run(cmd):return subprocess.run(list(map(str,cmd)),cwd=ROOT,capture_output=True,timeout=12)
    p=run([core,'--check-net',tbl,net]);assert p.returncode==0,p.stderr
    with tempfile.TemporaryDirectory(prefix='r10-pruneroot-check-') as name:
        d=pathlib.Path(name);(d/'prune.net').write_bytes(net.read_bytes());manifest=d/'routes';manifest.write_text('library\tprune\ttape\ttape\tprune.net\n')
        resources=d/'resources';(resources/'library').mkdir(parents=True);inp=d/'input';inp.write_bytes(TAPE)
        def execute(meta,tape=TAPE):
            files=sim.Files();mounts=[]
            if meta is not None:
                files.cache[b'\0library/signatures']=meta;(resources/'library/signatures').write_bytes(meta);mounts=[('00',resources)]
            pkg=d/'pkg';pkg.write_bytes(build([manifest],mounts,cache=False));inp.write_bytes(tape)
            verdict,out,_=sim.run(delta,tape,'fixture',files,maxsteps=200000,loaded=loaded)
            p=run([core,'--bundle',pkg,'library',inp])
            if verdict=='accept':assert p.returncode==0 and p.stdout==out,(verdict,p.returncode,p.stderr)
            else:assert p.returncode!=0 and not p.stdout,(verdict,p.returncode,p.stdout)
            return verdict,out
        verdict,plain=execute(None);assert verdict=='accept' and plain==prune_text(TAPE)
        assert b'public_fn:' not in plain and b'future_fp:' not in plain and b'hidden_fn:' not in plain
        assert b'__init:' in plain and b'address_fn:' in plain
        good=metadata([('public_fn',0,1),('hidden_fn',1,0),('future_fp',0,0),('main',0,1)])
        verdict,out=execute(good);assert verdict=='accept'
        assert b'public_fn:' in out and b'future_fp:' in out and b'hidden_fn:' not in out
        assert b'__init:' in out and b'address_fn:' in out
        assert execute(metadata([]))[1]==plain
        # Nonzero parameter storage and unsupported FP descriptors still root public code.
        name=b'future_fp';fpdesc=struct.pack('<6Q',1,4,123,4,8,0)
        params=b'USLSIG1\n'+U(1)+U(len(name))+name+bytes([0,1,0])+U(2)+DESC+U(2)+DESC+fpdesc+bytes([0])
        verdict,out=execute(params);assert verdict=='accept' and b'future_fp:' in out
        malformed=[b'',b'X'+good[1:],good+b'X',metadata([('public_fn',0,1),('public_fn',0,0)]),metadata([('absent',0,0)])]
        # Every prefix truncation, invalid flags/classes/width and unbounded count/name.
        malformed += [good[:i] for i in range(len(good))]
        one=bytearray(metadata([('public_fn',0,1)]));start=16+8+len('public_fn')
        for at in [start,start+1,start+2,len(one)-1]:
            b=bytearray(one);b[at]=2;malformed.append(bytes(b))
        for at,value in [(start+3+8+3*8,7),(start+3+8+4*8,3),(start+3+8+5*8,2),(8,8193),(16,2**64-1),(start+3+8+48,1)]:
            b=bytearray(one);struct.pack_into('<Q',b,at,value);malformed.append(bytes(b))
        for bad in malformed:assert execute(bad)[0]!='accept',bad
        # Original complete fixed hand fixtures retain ordinary fallback/roots.
        fixtures=json.loads((ROOT/'exec/prune/fixtures.json').read_text());tested=0
        for fixture in fixtures:
            for raw in fixture['manual'].values():
                raw=raw.encode();verdict,out=execute(None,raw);assert verdict=='accept' and out==prune_text(raw);tested+=1
        print(json.dumps({'default_manual_fixtures':tested,'default_base_equal':True,'public_unsupported_retained':True,'internal_dead_removed':True,'init_pointer_root_retained':True,'malformed_rejected':len(malformed),'executors':'C constructed network and sim; full check-net'}))
if __name__=='__main__':main(sys.argv[1:])

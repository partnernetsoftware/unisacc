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
def descriptor2(kind=1,width=8,align=8,tag=0,payload=b'',depth=0,base=4,unsigned=0):
    return struct.pack('<7Q',depth,base,0,kind,width,unsigned,align)+bytes([tag])+U(len(payload))+payload
I2=descriptor2();D2=descriptor2(3,8,8,base=8);F2=descriptor2(3,4,4,base=7)
PTR2=descriptor2(2,8,8,depth=1)
PAIR2=descriptor2(5,16,8,1,U(2)+U(0)+U(0)+U(0)+U(8)+D2+U(8)+U(0)+U(0)+U(4)+descriptor2(1,4,4))
def metadata2(name='public_fn',params=(),result=I2,mode=None,variadic=0,supported=1):
    name=name.encode();mode=(int(variadic or len(params)>6) if mode is None else mode)
    return (b'USLSIG2\n'+U(1)+U(len(name))+name+bytes([0,1,variadic,mode])+U(len(params))+
            result+U(len(params))+b''.join(params)+bytes([supported]))
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
        # Independent V2 fixture: all nine descriptors, including an ordered
        # double/int struct payload, survive regardless native support status.
        full=metadata2(params=(I2,D2,F2,PTR2,PAIR2,I2,D2,I2,I2),result=PAIR2)
        verdict,out=execute(full);assert verdict=='accept' and b'public_fn:' in out
        unsupported=metadata2(name='future_fp',params=(PAIR2,),supported=0)
        verdict,out=execute(unsupported);assert verdict=='accept' and b'future_fp:' in out
        array=descriptor2(5,16,8,3,U(2)+U(8)+I2)
        union=descriptor2(5,8,8,2,U(1)+U(0)+U(0)+U(0)+U(8)+I2)
        for layout in (array,union):
            verdict,out=execute(metadata2(params=(layout,),supported=0));assert verdict=='accept' and b'public_fn:' in out
        assert execute(b'USLSIG2\n'+U(0))[1]==plain
        for meta in (metadata2(params=(I2,)*1024),metadata2(variadic=1,supported=0),
                     metadata2(result=descriptor2(0,0,0,base=0)),
                     metadata2(result=descriptor2(6,0,0),supported=0),
                     metadata2(params=(descriptor2(4,8,8),),supported=0)):
            verdict,out=execute(meta);assert verdict=='accept' and b'public_fn:' in out
        v2bad=[full+b'X',b'USLSIG3\n'+full[8:],metadata2(params=(I2,)*9,mode=0),metadata2(mode=1),metadata2(variadic=1,mode=0)]
        # Every truncation of a complete seven-word descriptor record, then
        # each framing boundary of the larger recursive/nine-parameter record.
        small=metadata2();v2bad += [small[:i] for i in range(len(small))]
        v2bad += [full[:i] for i in range(0,len(full),8)]
        begin=24+len('public_fn');desc=begin+4+8
        for at,value,size in [(begin,2,1),(begin+1,0,1),(begin+1,2,1),(begin+2,2,1),
                              (begin+3,2,1),(len(small)-1,2,1),(8,8193,8),(begin+4,1025,8),(desc+24,7,8),(desc+32,3,8),
                              (desc+40,2,8),(desc+48,0,8),(desc+48,3,8),(desc+48,131072,8),
                              (desc+56,5,1),(desc+57,2**64-1,8),(desc+65,1,8)]:
            b=bytearray(small)
            if size==1:b[at]=value
            else:struct.pack_into('<Q',b,at,value)
            v2bad.append(bytes(b))
        # Primitive payloads are forbidden, aggregate tags must carry payload,
        # and a declared payload can never extend beyond the resource extent.
        for result in (descriptor2(payload=b'x'),descriptor2(5,16,8,0),descriptor2(5,16,8,1),
                       descriptor2(5,16777217,8,1,U(0)),descriptor2(3,1,1),descriptor2(2,4,4),
                       descriptor2(0,0,8),descriptor2(6,1,0),descriptor2(4,8,8,4,b'x')):
            v2bad.append(metadata2(result=result,supported=0))
        for bad in malformed+v2bad:assert execute(bad)[0]!='accept',bad
        # Original complete fixed hand fixtures retain ordinary fallback/roots.
        fixtures=json.loads((ROOT/'exec/prune/fixtures.json').read_text());tested=0
        for fixture in fixtures:
            for raw in fixture['manual'].values():
                raw=raw.encode();verdict,out=execute(None,raw);assert verdict=='accept' and out==prune_text(raw);tested+=1
        print(json.dumps({'default_manual_fixtures':tested,'default_base_equal':True,'public_unsupported_retained':True,'internal_dead_removed':True,'init_pointer_root_retained':True,'malformed_v1_rejected':len(malformed),'malformed_v2_rejected':len(v2bad),'v2_all_nine_params':True,'v2_parameter_cap_1024_accepted':True,'v2_void_unknown_fnptr_retained':True,'v2_struct_array_union_framing':True,'executors':'C constructed network and sim; full check-net'}))
if __name__=='__main__':main(sys.argv[1:])

"""PE writer reference bytes plus independent section/import/relocation checks."""
import pathlib,struct,subprocess,sys,tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.assemble import assemble
from unisa import image
from tins import parse


def verify(buf):
    u16=lambda p:struct.unpack_from('<H',buf,p)[0]
    u32=lambda p:struct.unpack_from('<I',buf,p)[0]
    u64=lambda p:struct.unpack_from('<Q',buf,p)[0]
    assert buf[:2]==b'MZ';p=u32(60);assert buf[p:p+4]==b'PE\0\0'
    assert u16(p+4)==0xaa64 and u16(p+6)==4 and u32(p+8)==0
    o=p+24;assert u16(o)==0x20b and u16(p+20)==240
    ib=u64(o+24);assert ib==0x140000000
    sa,fa=u32(o+32),u32(o+36);assert (sa,fa)==(4096,512)
    sections=[]
    for i in range(4):
        q=o+240+i*40;name=buf[q:q+8].rstrip(b'\0');vs,va,fs,fo=struct.unpack_from('<IIII',buf,q+8)
        assert va%sa==0 and fo%fa==0 and fs%fa==0 and fo+fs<=len(buf)
        sections.append((name,vs,va,fs,fo))
    assert [s[0] for s in sections]==[b'.text',b'.rdata',b'.data',b'.reloc']
    def rva(a):
        matches=[s[4]+a-s[2] for s in sections if s[2]<=a<s[2]+s[3]]
        assert len(matches)==1,(a,matches)
        return matches[0]
    imp,ims=struct.unpack_from('<II',buf,o+112+8);assert ims==40
    d=rva(imp);ilt,_,_,dll,iat=struct.unpack_from('<IIIII',buf,d)
    assert buf[rva(dll):].split(b'\0',1)[0]==b'KERNEL32.dll'
    names=[];i=0
    while u64(rva(ilt)+i*8):
        a=u64(rva(ilt)+i*8);assert u64(rva(iat)+i*8)==a
        hint=rva(a);assert u16(hint)==0
        names.append(buf[hint+2:].split(b'\0',1)[0]);i+=1
    assert len(names)==14 and names[0]==b'GetStdHandle' and names[-1]==b'MoveFileExA' and len(set(names))==14
    cfg,sz=struct.unpack_from('<II',buf,o+112+80);assert sz==320
    cookie=u64(rva(cfg)+88)-ib;assert cookie%8==0
    assert sections[2][2]<=cookie<sections[2][2]+sections[2][1]
    rel,rs=struct.unpack_from('<II',buf,o+112+40);at=rva(rel);end=at+rs;fixups=[]
    while at<end:
        page,bs=struct.unpack_from('<II',buf,at);assert page%4096==0 and bs>=8 and bs%4==0 and at+bs<=end
        for q in range(at+8,at+bs,2):
            x=u16(q)
            if x:assert x>>12==10;fixups.append(page+(x&4095))
        at+=bs
    assert at==end and fixups==sorted(set(fixups)) and cfg+88 in fixups
    assert u32(o+56)==sections[-1][2]+((sections[-1][1]+4095)&-4096)
    return fixups,cfg+88,sections[2][2]


def main():
    assert len(sys.argv)==4
    cmds=[[sys.argv[1],sys.argv[2]],[sys.executable,'exec/pp/sim.py',sys.argv[3]]]
    cases=[(0,0,[]),(16,65536,[]),(64,65536,[56,0,8,0]),(8200,64,[8192,4088,4096]),(2000100,65536,[2000000])]
    with tempfile.TemporaryDirectory() as d:
        f=pathlib.Path(d)/'in'
        for size,bss,rels in cases:
            data=bytearray(size)
            for at in rels:data[at:at+8]=(256).to_bytes(8,'little')
            src='@target win/arm64\n@data '+(data.hex() or '-')+'\n@data_len '+str(size)+'\n@bss '+str(bss)+'\n@relocs '+(','.join(map(str,rels)) or '-')+'\n_start:\nimm x0, 0\nret\n'
            tp=parse(src);text,st=assemble(tp);assert st['encoded']==st['insns']
            want=image.build(tp,text,image.relocate(tp,tp.data,st['data_va']-256),st['entry'])
            f.write_text(src)
            for cmd in cmds if size<10000 else cmds[:1]:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                if r.returncode or r.stdout!=want:
                    at=next((i for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b),min(len(want),len(r.stdout)))
                    raise AssertionError((size,bss,rels,r.returncode,r.stderr,at,len(want),len(r.stdout)))
                fix,cookie,dt=verify(r.stdout);assert fix==sorted(set([cookie]+[dt+x for x in rels]))
            print('PE ARM',size,'data',bss,'BSS',len(rels),'relocs',len(want),'bytes equal',flush=True)
        for src in ['@target lnx/arm64\nret','@target win/arm64\n@data -\n@data_len 7\n@relocs 0\nret','@target win/arm64\n@data -\n@bss -1\nret']:
            f.write_text(src+'\n')
            for cmd in cmds:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr
        print('PE invalid target/relocation/BSS rejected on both')
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Independent VERSIONINFO/section reader plus real llvm-readobj extraction.
Requires llvm-readobj; missing tool or invalid real PE head fails, never skips.
No VM or signing credentials needed, no compiler/model rebuild.
"""
import argparse, hashlib, os, re, shutil, struct, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from unisa.peversion import add_version_info


def layout(blob):
    pe=struct.unpack_from('<I',blob,60)[0];count=struct.unpack_from('<H',blob,pe+6)[0]
    opt=pe+24;table=opt+struct.unpack_from('<H',blob,pe+20)[0]
    sections=[]
    for n in range(count):
        at=table+40*n;vsize,rva,size,raw=struct.unpack_from('<4I',blob,at+8)
        sections.append({'header':blob[at:at+40],'name':blob[at:at+8].rstrip(b'\0'),
                         'rva':rva,'raw':raw,'size':size,'vsize':vsize,'bytes':blob[raw:raw+size]})
    return pe,opt,table,sections


def resource_bytes(blob):
    pe,opt,table,sections=layout(blob)
    rva,size=struct.unpack_from('<II',blob,opt+128)
    sect=next(s for s in sections if s['rva']<=rva<s['rva']+s['vsize'])
    base=sect['raw']+rva-sect['rva'];at=0
    for ident in [16,1,0x0409]:
        assert struct.unpack_from('<HH',blob,base+at+12)==(0,1)
        got,target=struct.unpack_from('<II',blob,base+at+16);assert got==ident
        if ident!=0x0409:assert target&0x80000000
        else:assert not target&0x80000000
        at=target&0x7fffffff
    data_rva,n,codepage,reserved=struct.unpack_from('<4I',blob,base+at)
    assert codepage==1200 and reserved==0 and n<=size
    raw=sect['raw']+data_rva-sect['rva'];assert raw+n<=base+size
    return blob[raw:raw+n]


def parse_block(blob,at=0,end=None):
    end=len(blob) if end is None else end
    length,value_length,kind=struct.unpack_from('<HHH',blob,at)
    stop=at+length;assert length>=8 and stop<=end and kind in [0,1]
    pos=at+6;key=[]
    while pos+2<=stop:
        word=blob[pos:pos+2];pos+=2
        if word==b'\0\0':break
        key.append(word)
    else:raise AssertionError('unterminated VERSIONINFO key')
    key=b''.join(key).decode('utf-16le');pos=(pos+3)//4*4
    n=value_length*(2 if kind else 1);assert pos+n<=stop
    value=blob[pos:pos+n];pos+=n;children={}
    while (pos+3)//4*4<stop:
        pos=(pos+3)//4*4
        child,nextpos=parse_block(blob,pos,stop);assert nextpos>pos
        assert child['key'] not in children;children[child['key']]=child;pos=nextpos
    return {'key':key,'kind':kind,'value':value,'children':children},stop


def fields(info, expected_version='0.0.9'):
    root,stop=parse_block(info);assert stop==len(info) and root['key']=='VS_VERSION_INFO' and root['kind']==0
    fixed=struct.unpack('<13I',root['value']);assert fixed[0]==0xFEEF04BD and fixed[1]==0x10000
    components=[int(x) for x in expected_version.split('.')]
    assert len(components) in [3,4] and all(0<=x<=65535 for x in components)
    components += [0]*(4-len(components))
    ms=components[0]<<16|components[1];ls=components[2]<<16|components[3]
    assert fixed[2:6]==(ms,ls,ms,ls)
    assert fixed[6:]==(0x3f,0,0x40004,1,0,0,0)
    table=root['children']['StringFileInfo']['children']['040904B0']['children']
    strings={key:block['value'].decode('utf-16le').removesuffix('\0') for key,block in table.items()}
    assert strings=={'ProductName':'Unisacc','ProductVersion':expected_version,'FileDescription':'Unisacc','FileVersion':expected_version}
    trans=root['children']['VarFileInfo']['children']['Translation'];assert trans['value']==struct.pack('<HH',0x409,1200)
    return strings


def synthetic():
    blob=bytearray(1536);blob[:2]=b'MZ';struct.pack_into('<I',blob,60,64);blob[64:68]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',blob,68,0x8664,1,0,0,0,240,0x22)
    opt=88;struct.pack_into('<H',blob,opt,0x20b)
    struct.pack_into('<II',blob,opt+32,4096,512);struct.pack_into('<II',blob,opt+56,8192,1024)
    struct.pack_into('<I',blob,opt+108,16)
    struct.pack_into('<8sIIIIIIHHI',blob,opt+240,b'.text\0\0\0',1,4096,512,1024,0,0,0,0,0x60000020)
    blob[1024]=0xc3;return bytes(blob)


def external_fields(blob,label,tool,d,expected_version):
    info=resource_bytes(blob);fields(info,expected_version)
    path=d/(label+'.exe');path.write_bytes(blob)
    p=subprocess.run([tool,'--coff-resources',str(path)],capture_output=True,text=True,timeout=15)
    assert p.returncode==0,(p.returncode,p.stderr)
    assert 'Type: VERSIONINFO (ID 16)' in p.stdout and 'Language: (ID 1033)' in p.stdout and 'Codepage: 1200' in p.stdout
    # Decode bytes actually extracted by the external reader, then inspect its fields.
    hexrows=re.findall(r'^\s+[0-9A-Fa-f]+:\s+([0-9A-Fa-f ]+)\s+\|',p.stdout,re.M)
    extracted=bytes.fromhex(''.join(hexrows));assert extracted==info
    return fields(extracted,expected_version)


def product_version():
    declared=re.findall(r'^\s*#define\s+UNISACC_VERSION\s+"([0-9]+(?:\.[0-9]+){2,3})"\s*$',
                        (ROOT/'src/version.h').read_text(),re.M)
    assert len(declared)==1,'src/version.h must declare exactly one product version'
    return declared[0]


def check(source,label,tool,d):
    out=add_version_info(source,'Unisacc','0.0.9')
    assert out==add_version_info(source,'Unisacc','0.0.9')
    before=layout(source);after=layout(out)
    assert len(after[3])==len(before[3])+1 and after[3][-1]['name']==b'.rsrc'
    for a,b in zip(before[3],after[3]):assert a==b,('original section changed',a['name'])
    pe,opt,table,sections=before
    assert struct.unpack_from('<I',out,opt+8)[0]==struct.unpack_from('<I',source,opt+8)[0]+after[3][-1]['size']
    assert struct.unpack_from('<I',out,opt+56)[0]>=after[3][-1]['rva']+after[3][-1]['vsize']
    assert source[2:pe]==out[2:pe], 'DOS/shell stub changed'
    info=resource_bytes(out);assert fields(info)['ProductName']=='Unisacc'
    external_fields(out,label,tool,d,'0.0.9')
    print(label+': llvm-readobj ProductName=Unisacc ProductVersion=0.0.9; original sections unchanged')
    return source


def rejects(source):
    pe,opt,table,sections=layout(source);cases=[]
    def patch(at,data):
        b=bytearray(source);b[at:at+len(data)]=data;return bytes(b)
    cases.extend([b'',source[:80],source+b'overlay',patch(opt,b'\x0b\x01'),
                  patch(opt+128,struct.pack('<II',1,1)),patch(opt+144,struct.pack('<II',1,1)),
                  patch(table+40*len(sections),b'x'*40),patch(opt+36,struct.pack('<I',3)),
                  patch(opt+8,struct.pack('<I',0xffffffff)),patch(opt+56,struct.pack('<I',0xfffff000))])
    file_align=struct.unpack_from('<I',source,opt+36)[0]
    insufficient_headers=(table+40*len(sections)+39)//file_align*file_align
    cases.append(patch(opt+60,struct.pack('<I',insufficient_headers)))
    cases.append(add_version_info(source,'Unisacc','0.0.9'))
    for bad in cases:
        try:add_version_info(bad,'Unisacc','0.0.9')
        except ValueError:pass
        else:raise AssertionError('invalid PE accepted')
    for product,version in [('', '0.0.9'),('x\0y','0.0.9'),('x','65536.0.0'),('x','1.2'),('x','a.b.c')]:
        try:add_version_info(source,product,version)
        except ValueError:pass
        else:raise AssertionError('invalid version metadata accepted')
    print('rejections: signed/resource/overlay/header-slack/truncation/alignment/uint32/metadata')


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--ape',type=Path,default=ROOT/'unisacc.com');ap.add_argument('--llvm-readobj')
    ns=ap.parse_args();tool=ns.llvm_readobj or shutil.which('llvm-readobj')
    if not tool:
        for p in ['/opt/homebrew/opt/llvm/bin/llvm-readobj','/usr/local/opt/llvm/bin/llvm-readobj']:
            try:Path(p).stat()
            except FileNotFoundError:continue
            tool=p;break
    if not tool:raise RuntimeError('llvm-readobj required; missing is not a passing check')
    ape=ns.ape.read_bytes();parts=layout(ape);end=max(s['raw']+s['size'] for s in parts[3]);head=ape[:end]
    assert end<len(ape),'expected complete frozen APE with appended Unix slices'
    with tempfile.TemporaryDirectory(prefix='unisacc-peversion-') as td:
        d=Path(td);base=synthetic();check(base,'synthetic',tool,d)
        if struct.unpack_from('<II',head,parts[1]+128)!=(0,0):
            expected=product_version();found=external_fields(head,'frozen-ape-pe-head',tool,d,expected)
            print('frozen-ape-pe-head: llvm-readobj ProductName='+found['ProductName']+
                  ' ProductVersion='+found['ProductVersion']+' matches src/version.h')
            try:add_version_info(head,'Unisacc','0.0.9')
            except ValueError:pass
            else:raise AssertionError('existing real VERSIONINFO accepted for replacement')
            rejects(base)
        else:
            check(head,'frozen-ape-pe-head',tool,d);rejects(head)
        try:add_version_info(ape,'Unisacc','0.0.9')
        except ValueError:pass
        else:raise AssertionError('already packed APE accepted')
    print('real head sha256='+hashlib.sha256(head).hexdigest())
if __name__=='__main__':main()

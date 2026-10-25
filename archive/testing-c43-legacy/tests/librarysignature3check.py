#!/usr/bin/env python3
"""Independent manual USLSIG3 host wire oracle; no model or ABI classification."""
import argparse, json, pathlib, shutil, struct, subprocess, tempfile, hashlib
U=lambda x: struct.pack('<Q',x)
def d(kind=1,width=4,alignment=4,depth=0,tag=0,payload=b'',rank=0,fmt=0,natural=0,flags=0,known=0,origin=0,version=3):
    words=b''.join(U(x) for x in (depth,17,23,kind,width,0,alignment))
    if version==1:return words[:48]
    meta=bytes((rank,fmt))+U(natural)+bytes((flags,known,origin)) if version==3 else b''
    return words+meta+bytes((tag,))+U(len(payload))+payload
def entry(i,k,child,off=0,bo=0,bw=0,storage=4,align=4):
    return U(i)+bytes((k,))+U(align)+b''.join(U(x) for x in (off,bo,bw,storage))+child
def agg(entries,width=4,tag=1):return d(5,width,4,tag=tag,payload=U(len(entries))+b''.join(entries))
def wire(name,result,args=(),support=0,version=3):
    n=name.encode(); head=bytes((0,1,0))+(bytes((0,)) if version>=2 else b'')
    return ('USLSIG%d\n'%version).encode()+U(1)+U(len(n))+n+head+U(len(args))+result+U(len(args))+b''.join(args)+bytes((support,))
def cb(payload):return d(4,8,8,1,4,payload)
def definition(result,args=(),support=0,id=1):return bytes((1,0))+U(id)+bytes((0,0))+U(len(args))+result+U(len(args))+b''.join(args)+bytes((support,))
def ref(id):return cb(bytes((1,1))+U(id))
C=r"""
#include "libraryexports.h"
#include <assert.h>
static unsigned char *readbytes(const char *path,size_t *n){FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long z=ftell(f);assert(z>=0);rewind(f);unsigned char*b=malloc(z?z:1);assert(b);assert(fread(b,1,z,f)==(size_t)z);fclose(f);*n=z;return b;}
int main(int argc,char **argv){assert(argc==3);FILE*f=fopen(argv[1],"r");assert(f);char line[4096],error[128];us_exports set={0};unsigned good=0,bad=0,cuts=0;
 while(fgets(line,sizeof line,f)){int expected;char path[4000];assert(sscanf(line,"%d %3999s",&expected,path)==2);size_t n;unsigned char*b=readbytes(path,&n);us_export*old=set.items;unsigned char*oldwire=set.count?set.items[0].wire:NULL;size_t oldlen=set.count?set.items[0].wire_length:0;
 int rc=us_exports_load_bridge(&set,b,n,error,sizeof error);assert((rc==0)==expected);
 if(!expected){assert(set.items==old);if(oldwire){assert(set.items[0].wire==oldwire&&set.items[0].wire_length==oldlen);}bad++;}
 else{good++;assert(set.count==1);us_export*x=set.items;assert(x->wire_length==n&&!memcmp(x->wire,b,n));
  if(x->version==3){assert(!us_export_supported(x)&&!us_export_bridge_supported(x));assert(!x->result.ffi);for(size_t j=0;j<x->stored;j++){assert(us_export_arg(x,j)==x->argtypes+j);assert(!x->argtypes[j].ffi);}}
  if(!strcmp(x->name,"facts")){us_export_type*t=&x->result;assert(t->natural_alignment==4&&t->layout_flags==1&&t->layout_known_mask==3&&t->layout_origin==1);assert(t->nmembers==3);assert(t->members[0].ordinal==0&&t->members[0].entry_kind==1&&t->members[0].bit_width==3);assert(t->members[1].ordinal==1&&t->members[1].entry_kind==2&&t->members[1].bit_offset==3);assert(t->members[2].entry_kind==3&&t->members[2].offset==4&&t->members[2].storage==4&&t->members[2].effective_alignment==4);}
  if(!strcmp(x->name,"cycle")){assert(x->graph.count==2);us_export_signature*a=x->graph.signatures[0],*c=x->graph.signatures[1];assert(x->result.signature==a&&a->result.signature==c&&c->result.signature==a&&a->argtypes[0].signature==a&&x->argtypes[0].signature==c);}
  if(!strcmp(x->name,"ld")){assert(x->result.fp_rank==3&&x->result.fp_format==3&&x->result.width==16);}
  for(size_t cut=0;cut<n;cut++){us_export*kept=set.items;assert(us_exports_load_bridge(&set,b,cut,error,sizeof error));assert(set.items==kept);cuts++;}
 }
 free(b);
 }fclose(f);us_exports_clear(&set);us_exports_clear(&set);printf("{\"good\":%u,\"bad\":%u,\"truncations\":%u}\n",good,bad,cuts);return 0;}
"""
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--out');a=ap.parse_args();root=pathlib.Path(__file__).resolve().parents[1]
    out=pathlib.Path(a.out or tempfile.mkdtemp(prefix='r10-sig3-host-'));out.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(root/'exec/c/libraryexports.h',out/'libraryexports.h');(out/'fixture.c').write_text(C)
    cases=[]
    def add(name,b,yes=True):cases.append((name,b,yes))
    integer=d(); ordinary=entry(0,0,integer)
    facts=agg([entry(0,1,integer,bw=3),entry(1,2,integer,bo=3,bw=5),entry(2,3,integer,off=4)])
    facts=bytearray(facts);facts[58:66]=U(4);facts[66:69]=bytes((1,3,1));facts=bytes(facts)
    add('facts',wire('facts',facts));add('scalar',wire('scalar',integer,[d(3,8,8,rank=2,fmt=2)]))
    add('ld',wire('ld',d(3,16,16,rank=3,fmt=3,natural=16,known=3,origin=1)))
    add('quad',wire('quad',d(3,16,16,rank=3,fmt=4)));add('winld',wire('winld',d(3,8,8,rank=3,fmt=2)))
    add('unknownfp',wire('unknownfp',d(3,16,16)));add('external',wire('external',d(natural=4,known=1,origin=2)))
    add('array',wire('array',d(5,8,4,tag=3,payload=U(2)+U(4)+integer)))
    add('anonymous',wire('anonymous',agg([entry(0,4,agg([ordinary]),storage=4)])))
    cycle=cb(definition(cb(definition(ref(1),id=2)),[ref(1)]))
    add('cycle',wire('cycle',cycle,[ref(2)]));add('emptycallback',wire('emptycallback',cb(b'')))
    add('v1',wire('v1',d(version=1),[d(version=1)],support=1,version=1))
    add('v2',wire('v2',d(version=2),[d(version=2)],support=1,version=2))
    add('maxentries',wire('maxentries',agg([entry(i,0,integer) for i in range(128)],tag=2)))
    for label,result in [('ranknonfp',d(rank=1,fmt=1)),('formatwidth',d(3,8,8,rank=1,fmt=1)),('rankmismatch',d(3,8,8,rank=2,fmt=3)),('unknowncomplete',d(3,8,8,origin=1)),('knownmissing',d(known=1,natural=4,origin=1)),('natbad',d(natural=3)),('flagsunknown',d(flags=1)),('badknown',d(known=4)),('badorigin',d(origin=3)),('fpunknownrank',d(3,8,8,rank=2)),('badordinal',agg([entry(1,0,integer)])),('badkind',agg([entry(0,5,integer)])),('badentryalign',agg([entry(0,0,integer,align=3)])),('ordinarybits',agg([entry(0,0,integer,bw=1)])),('zeronamed',agg([entry(0,1,integer)])),('outsidebits',agg([entry(0,1,integer,bo=31,bw=2)])),('badstorage',agg([entry(0,1,integer,bw=1,storage=2)])),('barrierbits',agg([entry(0,3,integer,bw=1)])),('barrieroffset',agg([entry(0,3,integer,off=5)])),('badanonymous',agg([entry(0,4,integer)])),('tooentries',agg([entry(i,0,integer) for i in range(129)],tag=2)),('zerowidth',agg([entry(0,3,integer)],width=0)),('forward',ref(1)),('badid',cb(definition(integer,id=2))),('duplicate',cb(definition(cb(definition(integer,id=1))))),('nestedproof',cb(definition(integer,support=1))),('badstride',d(5,8,4,tag=3,payload=U(2)+U(3)+integer))]:add(label,wire(label,result),False)
    add('unknownexternalfp',wire('unknownexternalfp',d(3,16,16,origin=2)),False)
    add('unknowncompletefp',wire('unknowncompletefp',d(3,16,16,natural=16,known=3,origin=1)),False)
    add('topproof',wire('topproof',integer,support=1),False);add('trailer',wire('trailer',integer)+b'x',False)
    lines=[]
    for i,(name,b,yes) in enumerate(cases):
        path=out/(str(i)+'-'+name+'.sig');path.write_bytes(b);lines.append('%d %s\n'%(yes,path))
    (out/'manifest').write_text(''.join(lines))
    subprocess.run(['cc','-std=c11','-g','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(out/'fixture.c'),'-lffi','-o',str(out/'probe')],check=True,timeout=20)
    r=subprocess.run([str(out/'probe'),str(out/'manifest'),'reserved'],check=True,capture_output=True,text=True,timeout=20)
    receipt=json.loads(r.stdout);receipt.update(header_sha256=hashlib.sha256((out/'libraryexports.h').read_bytes()).hexdigest(),private=str(out),sanitizers='address,undefined',policy='V3 diagnostic support0 only')
    (out/'evidence.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()

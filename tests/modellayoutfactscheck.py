#!/usr/bin/env python3
"""Independent real-source ordered layout facts, simulator and constructed C net.
The only instrumentation appends bounded generic bank reads before ACCEPT.
This is source-facts coverage, not product/native ABI qualification.
"""
import argparse,copy,hashlib,json,os,pathlib,re,shutil,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelcallbacksourcecheck import decode,U
BANKS={n:(600+i)<<40 for i,n in enumerate(('COUNT','KIND','BASE','DEPTH','ARRAY','OFFSET','BITOFFSET','BITWIDTH','STORAGE','ALIGN','CHILD','MEMBER','SHAPE','SIGNED','UNION','FLATTEN','SEEN'))}
FIELDS=('KIND','BASE','DEPTH','ARRAY','OFFSET','BITOFFSET','BITWIDTH','STORAGE','ALIGN','CHILD','MEMBER','SHAPE','SIGNED')
SOURCE="""struct Shared {unsigned int a:3;signed int b:5;unsigned int c:6;};
struct Gaps {unsigned int :3;unsigned int n:5;unsigned int :0;unsigned int z:4;};
struct Ordinary {int n;double d;};
struct Outer {struct {unsigned int a:3;signed int b:5;};long tail;};
struct Array {int a[3];short z;};
union Choice {unsigned int a:3;double d;};
struct Reordered {double d;int n;};
struct Holder {struct Ordinary *p;};
struct Shared shared(struct Shared x){return x;}
struct Gaps gaps(struct Gaps x){return x;}
struct Ordinary ordinary(struct Ordinary x){return x;}
struct Outer outer(struct Outer x){return x;}
struct Array array(struct Array x){return x;}
union Choice choice(union Choice x){return x;}
struct Reordered reordered(struct Reordered x){return x;}
struct Holder holder(struct Holder x){return x;}
int main(void){return 0;}
"""
class Files:
 def __init__(self,enabled=True,library=True):self.enabled=enabled;self.library=library
 def get(self,key):
  if self.library and key in (b'\0library/module',b'\0library/symbols'):return U(1)
  if key==b'\0library/layoutfacts' and self.enabled:return U(1)
  return None

def instrument(original):
 d=copy.deepcopy(original);prefix='LAYOUTTEST.'
 def state(name,nxt,acts,case=None):
  ix=len(d['seqs']);d['seqs'].append(acts);d['states'][prefix+name]=['r',{str(k):[(prefix+case[k] if case and k in case else prefix+nxt),ix] for k in range(257)}]
 def words(reg):return sum(([['A64I','shr','lf_test_byte',reg,i*8],['OUTW','lf_test_byte']] for i in range(8)),[])
 def bank(name,key):return [['LDX','lf_test_value',key,BANKS[name]]]+words('lf_test_value')
 # Existing transition actions retain all production output; only final ACCEPT moves.
 hooks=0
 for mode,row in list(d['states'].values()):
  for key,(nxt,seqid) in list(row.items()):
   seq=d['seqs'][seqid]
   if seq and seq[-1][0]=='ACCEPT':
    ix=len(d['seqs']);d['seqs'].append(seq[:-1]);row[key]=[prefix+'start',ix];hooks+=1
 assert hooks>0,'no production ACCEPT hook'
 state('start','scan', [['OUT',b] for b in b'USLFACT1\n']+[['LDI','lf_test_sid',1]])
 state('scan','scanbranch',[['CMPI','lf_test_sid',129]])
 state('scanbranch','seen',[],{1:'done'})
 state('seen','seenbranch', [['LDX','lf_test_seen','lf_test_sid',BANKS['SEEN']],['CMPI','lf_test_seen',0]])
 state('seenbranch','header',[],{1:'next'})
 header=words('lf_test_sid')
 for name in ('COUNT','UNION','FLATTEN','SEEN'):header+=bank(name,'lf_test_sid')
 header += [['LDX','lf_test_count','lf_test_sid',BANKS['COUNT']],['LDI','lf_test_item',0],['CMPI','lf_test_count',129]]
 state('header','headerbound',header)
 state('headerbound','item',[],{2:'reject',1:'reject'})
 state('item','itembranch',[['C64U','lf_test_item','lf_test_count']])
 state('itembranch','next',[],{0:'entry'})
 acts=[['ALUI','mul','lf_test_key','lf_test_sid',128],['ALU','add','lf_test_key','lf_test_key','lf_test_item']]
 for name in FIELDS:acts+=bank(name,'lf_test_key')
 acts += [['ALUI','add','lf_test_item','lf_test_item',1]];state('entry','item',acts)
 state('next','scan',[['ALUI','add','lf_test_sid','lf_test_sid',1]])
 state('reject','reject',[['REJECT','test facts bounds']]);state('done','done',words('lf_test_sid')+[['ACCEPT']])
 return d

def parse_facts(data):
 assert data.startswith(b'USLFACT1\n');at=9;records={}
 def word():
  nonlocal at
  assert at+8<=len(data),'truncated fact dump';n=struct.unpack_from('<Q',data,at)[0];at+=8;return n
 while True:
  sid=word()
  if sid==129:break
  count,union,flatten,seen=(word() for _ in range(4));assert 1<=sid<=128 and sid not in records and count<=128 and seen==1 and flatten==0
  entries=[]
  for _ in range(count):
   item=dict(zip(FIELDS,(word() for _ in FIELDS)));item['MEMBER']=int(item['MEMBER']!=0);item['SHAPE']=int(item['SHAPE']!=0);entries.append(item)
  records[sid]={'union':union,'entries':entries}
 assert at==len(data) and records and sum(len(r['entries']) for r in records.values())>0
 return records

SBB=int(re.search(r'"SBB":(\d+)',(ROOT/'exec/facts/k2-control.tsv').read_text()).group(1))  # a struct's BASE code is SBB+sid (#27 moved it 1000->3840)
def expected():
 def entry(kind,base,offset,storage,alignment,bits=0,bitoffset=0,signed=0,named=1,array=0,child=0,shape=0,depth=0):
  return dict(zip(FIELDS,(kind,base,depth,array,offset,bitoffset,bits,storage,alignment,child,named,shape,signed)))
 b=lambda base,offset,bits,bitoffset=0,signed=0,kind=1,named=1:entry(kind,base,offset,4,4,bits,bitoffset,signed,named)
 # Independent natural C allocation: these widths share one int unit.
 second,third=3,8
 rows={1:[b(20,0,3),b(4,0,5,second,1),b(20,0,6,third)],2:[b(20,0,3,kind=2,named=0),b(20,0,5,second),b(20,4,0,kind=3,named=0),b(20,4,4)],3:[entry(0,4,0,4,4),entry(0,64,8,8,8)],4:[entry(4,SBB+5,0,4,4,named=0,child=5),entry(0,8,8,8,8)],5:[b(20,0,3),b(4,0,5,second,1)],6:[entry(0,4,0,12,4,array=3,shape=1),entry(0,2,12,2,2)],7:[b(20,0,3),entry(0,64,0,8,8)],8:[entry(0,64,0,8,8),entry(0,4,8,4,4)],9:[entry(0,SBB+3,0,8,8,depth=1)]}
 return {sid:{'union':int(sid==7),'entries':entries} for sid,entries in rows.items()}

ORDINARY_SOURCE="""struct Ordinary {int n;double d;};
struct Outer {struct {int a;short b;};long tail;};
struct Array {int a[3];short z;};
union Choice {int n;double d;};
struct Reordered {double d;int n;};
struct Holder {struct Ordinary *p;};
struct Ordinary ordinary(struct Ordinary x){return x;}
struct Outer outer(struct Outer x){return x;}
struct Array array(struct Array x){return x;}
union Choice choice(union Choice x){return x;}
struct Reordered reordered(struct Reordered x){return x;}
struct Holder holder(struct Holder x){return x;}
int main(void){return 0;}
"""

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument('--model',type=pathlib.Path)
 ap.add_argument('--baseline',type=pathlib.Path)
 ap.add_argument('--evidence',type=pathlib.Path)
 a=ap.parse_args()
 evidence={'schema':1,'test_sha256':hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
           'scope':'actual ordered source facts only; no ABI qualification',
           'status':'failed','commands':[],'source':SOURCE,'ordinary_source':ORDINARY_SOURCE}
 with tempfile.TemporaryDirectory(prefix='r10-layout-facts-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   c=[str(ROOT/'tests/bound'),'20',*map(str,args)]
   p=subprocess.run(c,capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'))
   evidence['commands'].append({'command':c,'rc':p.returncode,
       'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')})
   assert p.returncode==0,(c,p.returncode,p.stderr)
   return p.stdout
  try:
   evidence['source_closure']={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest()
       for directory in ('exec/parse2','exec/parse','exec/pp','exec/c','exec/shared')
       for f in sorted((ROOT/directory).rglob('*')) if f.is_file() and f.suffix in ('.py','.tsv','.c','.h')}
   cmd(ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref')
   s=(t/'ref.c').read_bytes()
   old=b'        if (tkind[i] == 2) { __write(1, "=", 1);'
   new=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+old
   assert s.count(old)==1
   (t/'dump.c').write_bytes(s.replace(old,new))
   cmd('cc','-w','-std=c99','-O2',t/'dump.c','-o',t/'dump')
   cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'run')
   model=a.model or t/'e3.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/parse2/gen2.py',model)
   d=json.loads(model.read_text())
   evidence['production_model_sha256']=hashlib.sha256(model.read_bytes()).hexdigest()
   original=load(d);test=instrument(d);m=t/'facts.json'
   m.write_text(json.dumps(test,separators=(',',':')));loaded=load(test)
   cmd(sys.executable,ROOT/'exec/c/tbl.py',m,t/'facts.tbl')
   cmd(sys.executable,ROOT/'exec/c/net.py',t/'facts.tbl',t/'facts.net')
   evidence['full_domain']=cmd(t/'run','--check-net',t/'facts.tbl',t/'facts.net').decode().strip()
   evidence['instrumented_model_sha256']=hashlib.sha256(m.read_bytes()).hexdigest()
   (t/'source.c').write_text(SOURCE)
   raw=cmd(t/'dump','-dump-tokens',t/'source.c');(t/'tokens').write_bytes(raw)
   evidence['tokens_sha256']=hashlib.sha256(raw).hexdigest()
   status,production,_=run(d,raw,'source',files=Files(),loaded=original,maxsteps=5000000)
   assert status=='accept',(status,production[:300])
   status,out,_=run(test,raw,'source',files=Files(),loaded=loaded,maxsteps=5000000)
   assert status=='accept' and out.startswith(production),'test emitter changed production prefix'
   facts=out[len(production):];observed=parse_facts(facts);oracle=expected()
   evidence['observed']=observed;evidence['expected']=oracle
   evidence['oracle_differences']=[{'sid':sid,'ordinal':i,'field':k,'actual':item[k],'expected':want[k]}
       for sid,r in oracle.items() for i,(item,want) in enumerate(zip(observed[sid]['entries'],r['entries']))
       for k in FIELDS if item[k]!=want[k]]
   routes=t/'routes';routes.write_text('parse\te3\ttokens\ttape\tfacts.net\n')
   resources=t/'resources';resources.mkdir()
   for key in ('module','symbols','layoutfacts'):(resources/key).write_bytes(U(1))
   pkg=t/'bundle';pkg.write_bytes(build([routes],[('006c6962726172792f',resources)],cache=False))
   native=cmd(t/'run','--bundle',pkg,'parse',t/'tokens')
   assert native==out,'C constructed network differs from simulator'
   evidence['sim_C_network_equal']=True
   ds,disabled,_=run(d,raw,'source',files=Files(False,False),loaded=original,maxsteps=5000000)
   ts,dumped,_=run(test,raw,'source',files=Files(False,False),loaded=loaded,maxsteps=5000000)
   assert ds==ts=='accept' and disabled and dumped==disabled+b'USLFACT1\n'+U(129)
   offpkg=t/'offbundle';offpkg.write_bytes(build([routes],[],cache=False))
   assert cmd(t/'run','--bundle',offpkg,'parse',t/'tokens')==dumped
   evidence['default_resource_off_empty_facts']=True
   if a.baseline:
    baseline=json.loads(a.baseline.read_text());base_loaded=load(baseline)
    evidence['baseline_model_sha256']=hashlib.sha256(a.baseline.read_bytes()).hexdigest()
    bs,oldout,_=run(baseline,raw,'source',files=Files(),loaded=base_loaded,maxsteps=5000000)
    assert bs=='accept' and oldout!=production,'bitfield correction must differ from old wrong signature'
    _,oldrecords=decode(oldout);_,newrecords=decode(production)
    predicted=[('shared',1,8,3),('shared',2,16,8),('gaps',0,8,3),('outer',1,8,3)]
    evidence['bitfield_correction_vs_old']=[]
    for fname,ordinal,oldbit,newbit in predicted:
     oldlayout=oldrecords[fname]['params'][0]['child'][ordinal][0]
     newlayout=newrecords[fname]['params'][0]['child'][ordinal][0]
     assert oldlayout[1]==oldbit and newlayout[1]==newbit,(fname,oldlayout,newlayout)
     evidence['bitfield_correction_vs_old'].append({'function':fname,'ordinal':ordinal,
         'old_layout':oldlayout,'corrected_layout':newlayout})
    (t/'ordinary.c').write_text(ORDINARY_SOURCE)
    ordinary=cmd(t/'dump','-dump-tokens',t/'ordinary.c');(t/'ordinary.tokens').write_bytes(ordinary)
    for enabled in (True,False):
     files=Files(enabled,enabled)
     bs,bout,_=run(baseline,ordinary,'ordinary',files=files,loaded=base_loaded,maxsteps=5000000)
     ns,nout,_=run(d,ordinary,'ordinary',files=files,loaded=original,maxsteps=5000000)
     assert bs==ns=='accept' and bout==nout and nout,'ordinary-only production baseline changed'
    ns,ordinarydump,_=run(test,ordinary,'ordinary',files=Files(),loaded=loaded,maxsteps=5000000)
    assert ns=='accept' and cmd(t/'run','--bundle',pkg,'parse',t/'ordinary.tokens')==ordinarydump
    evidence['ordinary_only_tape_and_USLSIG2_baseline_equal']=True
   tape,signatures=decode(production)
   assert tape and len(signatures)==9
   evidence['production_output_sha256']=hashlib.sha256(production).hexdigest()
   evidence['facts_sha256']=hashlib.sha256(facts).hexdigest()
   evidence['record_count']=len(observed)
   evidence['entry_count']=sum(len(r['entries']) for r in observed.values())
   assert evidence['record_count']==9 and evidence['entry_count']==20
   evidence['production_tape_and_signatures_prefix_unchanged']=True
   evidence['anonymous_child_once_no_flattened_duplicates']=True
   assert observed==oracle,('independent natural ordered facts',evidence['oracle_differences'])
   evidence['status']='passed'
  except Exception as exc:evidence['error']=repr(exc)
 if a.evidence:
  a.evidence.parent.mkdir(parents=True,exist_ok=True)
  a.evidence.write_text(json.dumps(evidence,indent=2)+'\n')
 print(json.dumps({k:evidence[k] for k in ('status','scope','full_domain','record_count','entry_count',
       'sim_C_network_equal','default_resource_off_empty_facts',
       'ordinary_only_tape_and_USLSIG2_baseline_equal','error') if k in evidence}))
 return 0 if evidence['status']=='passed' else 1
if __name__=='__main__':raise SystemExit(main())

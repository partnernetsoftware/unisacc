#!/usr/bin/env python3
"""Actual E3 anonymous facts and bounded graph equality; private inputs only.
Usage: anonymoussignaturecheck.py PLAIN_MODEL_JSON EXECUTOR TYPED_DUMPER DRIVER PACKAGE
This tests parser facts/script calls, not a native callback ABI bridge.
"""
import json, os, pathlib, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/parse2')]
import sim
from libraryexports import PARAMMARK, PARAMDEPTH, PARAMBASE, PARAMSHAPE, SIGEPOCH, VARIADIC
RD,RB,MODE,COUNT,RSH=(i<<40 for i in (55,56,57,61,59))

def main():
 model,core,dumper=map(pathlib.Path,sys.argv[1:4]);delta=json.loads(model.read_text());loaded=sim.load(delta)
 assert delta['states']['FS.query'][1]['0'][0] != 'CL.signature', 'legacy query still shadows complete pool'
 assert not any(k.startswith('DL.') for k in delta['states']), 'plain typed tokens require a plain model; errors models need located tokens'
 assert len(sys.argv)==6, 'actual driver/package execution is a required acceptance step'
 driver,package=map(pathlib.Path,sys.argv[4:6])
 with tempfile.TemporaryDirectory(prefix='unisacc-anonymous-signatures-') as name:
  temp=pathlib.Path(name)
  def command(*args):
   return subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,args)],capture_output=True,env=dict(os.environ,UA_TYPESPELL='1'))
  def good(*args):
   p=command(*args);assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout
  tbl=temp/'parser.tbl';good(sys.executable,ROOT/'exec/c/tbl.py',model,tbl)
  net=temp/'parser.net';good(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  good(core,'--check-net',tbl,net)
  def parse(text,label,accepted=True):
   src=temp/(label+'.c');src.write_text(text);tok=good(dumper,'-dump-tokens',src);inp=temp/(label+'.tokens');inp.write_bytes(tok)
   facts={}
   def trace(frame,event,arg):
    if frame.f_code is sim.run.__code__:
     frame.f_trace_lines=False
     if event=='return':facts.update(frame.f_locals['W'])
     return trace
   sys.settrace(trace)
   try:status,out,_=sim.run(delta,tok,label,loaded=loaded,maxsteps=4000000)
   finally:sys.settrace(None)
   actual=command(core,net,inp,label)
   assert (status=='accept')==accepted,(label,status,out)
   assert (actual.returncode==0)==accepted,(label,actual.returncode,actual.stderr)
   if accepted:assert actual.stdout==out,(label,'sim/table divergence')
   return facts,out,src
  facts,tape,src=parse((ROOT/'exec/parse2/probes/callback_full_signature.c').read_text(),'nested')
  assert b'cvtds r0, r0' in tape, 'indirect ninth/seventeenth double arguments must convert to float'
  get=lambda sig,bank:facts.get(sig+bank,0)
  assert facts['fs_next']>=12
  assert get(128,COUNT)==get(129,COUNT)==9
  assert get(131,COUNT)==get(132,COUNT)==get(133,COUNT)==17
  assert get(130,RD)==1 and get(130,RB)==128
  assert facts[130*1024+PARAMBASE]==129
  for sig,count in ((128,9),(129,9),(131,17),(132,17),(133,17),(134,9)):
   epoch=get(sig,SIGEPOCH);assert epoch>0
   for i in range(count):assert facts[sig*1024+i+PARAMMARK]==epoch,(sig,i)
   assert get(sig,MODE)==1
  assert facts[131*1024+16+PARAMBASE]==65
  assert facts[133*1024+16+PARAMBASE]==64
  assert get(134,VARIADIC)==1 and all(get(s,VARIADIC)==0 for s in (128,129,130,131,132,133))
  # Regression for the earlier test gap: facts existed, but expanded legacy
  # FS.query still won. Exercise the actual query/convert states in both VMs.
  for sig,index in ((128,8),(131,16)):
   actions=[]
   for key,value in facts.items():
    if isinstance(key,int) and key>=55<<40:
     actions += [['LDI','test_addr',key],['LDI','test_value',value],['STX','test_addr',0,'test_value']]
   actions += [['LDI','call_sig',sig],['LDI','na',index],['LDI','vt',0],['LDI','vb',64]]
   d=dict(delta,start='test.query',states=dict(delta['states']),seqs=list(delta['seqs']))
   n=len(d['seqs']);d['seqs'] += [actions,[['OUTW','t']],[['OUTW','vt'],['OUTW','vb'],['ACCEPT']]]
   d['states']['test.query']=['r',{str(i):['FS.query',n] for i in range(257)}]
   d['states']['test.signature']=d['states']['CL.signature']
   d['states']['CL.signature']=['r',{str(i):['test.signature',n+1] for i in range(257)}]
   d['states']['CL.a2']=['r',{str(i):['CL.a2',n+2] for i in range(257)}]
   status,out,_=sim.run(d,b'','query',maxsteps=4000000)
   assert status=='accept' and out[:1]==b'A' and out[-2:]==bytes([0,65]) and b'cvtds r0, r0' in out,(sig,index,status,out)
   js=temp/'query.json';qt=temp/'query.tbl';qn=temp/'query.net';empty=temp/'empty';empty.write_bytes(b'');js.write_text(json.dumps(d,separators=(',',':')))
   good(sys.executable,ROOT/'exec/c/tbl.py',js,qt);good(sys.executable,ROOT/'exec/c/net.py',qt,qn)
   assert good(core,qn,empty,'query')==out
  # Run the model's equality entry on actual parsed facts, in sim AND C.

  def equal(left,right,expected,overrides=None):
   seeded=dict(facts);seeded.update(overrides or {})
   actions=[]
   for key,value in seeded.items():
    if isinstance(key,int) and key>=55<<40:
     actions += [['LDI','test_addr',key],['LDI','test_value',value],['STX','test_addr',0,'test_value']]
   actions += [['LDI','fs_eqdepth',0],['LDI','fs_eqepoch',0],['LDI','fs_l',left],['LDI','fs_r',right],['PUSH','test.done']]
   d=dict(delta,start='test.start',states=dict(delta['states']),seqs=list(delta['seqs']))
   n=len(d['seqs']);d['seqs'] += [actions,[['OUTW','fs_result'],['ACCEPT']]]
   d['states']['test.start']=['r',{str(i):['FS.TYPEEQ',n] for i in range(257)}]
   d['states']['test.done']=['r',{str(i):['test.done',n+1] for i in range(257)}]
   mode,row=d['states']['RET'];d['states']['RET']=[mode,dict(row, **{'test.done':['test.done',0]})]
   status,out,_=sim.run(d,b'','equality',maxsteps=4000000);assert status=='accept' and out==bytes([expected]),(left,right,status,out)
   js=temp/'equal.json';et=temp/'equal.tbl';empty=temp/'empty';empty.write_bytes(b'');js.write_text(json.dumps(d,separators=(',',':')))
   good(sys.executable,ROOT/'exec/c/tbl.py',js,et);assert good(core,et,empty,'equality')==out
  equal(128,129,1);equal(131,132,1);equal(131,133,0);equal(130,137,1)
  equal(128,129,0,{129+VARIADIC:1})
  cyclic={}
  for sig in (128,129):
   cyclic[sig+RD]=1;cyclic[sig+RB]=sig
  equal(128,129,1,cyclic)
  cyclic[129*1024+8+PARAMBASE]=64;equal(128,129,0,cyclic)
  # Exact bound and overflow use real parser signatures, never a reduced cap.
  for count in (1024,1025):
   f,_,_=parse('typedef float (*Max)('+','.join(['float']*count)+');int main(void){return 0;}',str(count),count==1024)
   if count==1024:
    assert f[128+COUNT]==1024 and f[128*1024+1023+PARAMBASE]==65
  for opt in ('-O0','-O1','-O2'):
   good(driver,'--models',package,opt,'-run',src)
  print(json.dumps(dict(actual_parser_sim_and_network=True,query9_and17_conversion=True,driver_three_optimizations=True,full9=True,full17=True,nested_owner_epochs=True,true_variadic_separate=True,equality_cases=7,recursive_pairs=True,bound1024=True,native_callback_bridge=False)))
if __name__=='__main__':main()

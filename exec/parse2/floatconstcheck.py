"""Host-cc bit oracle for the delta floating converter (also checks its net)."""
import json, subprocess, pathlib, sys
T=pathlib.Path(sys.argv[2]); T.mkdir(exist_ok=True)
d=json.load(open(sys.argv[1]))
def state(n,nxt,acts):
 i=len(d['seqs']);d['seqs'].append(acts);d['states'][n]=['r',{str(k):[nxt,i] for k in range(257)}]
state('TEST','SPANNUM',[['PUSH','TEST.done']]);state('TEST.done','HALT',sum(([['A64I','shr','test_byte','nv',i*8],['OUTW','test_byte']] for i in range(8)),[])+[['ACCEPT']]);d['start']='TEST';i=len(d['seqs']);d['seqs'].append([['POP']]);d['states']['RET'][1]['TEST.done']=['TEST.done',i]
(T/'h.json').write_text(json.dumps(d,separators=(',',':')))
for a in [['cc','-O2','exec/c/run.c','-o',str(T/'run')],['python3','exec/c/tbl.py',str(T/'h.json'),str(T/'h.tbl')]]:subprocess.run(a,check=True,timeout=60,capture_output=True)
values=['00.5','0.0','0.1','1.0','2.0','1.5','123456789.125','1e10','1e-10','.5','1.0715086071862673e301','9.3326361850321888e-302','1e308','1e309','1e-324','4.9406564584124654e-324','2.4703282292062327e-324','2.4703282292062328e-324','2.2250738585072014e-308','2.225073858507201e-308','1.7976931348623157e308','1.7976931348623159e308','1.00000000000000011102230246251565404236316680908203125','1.00000000000000011102230246251565404236316680908203126','0.1f','1e-45f','7.0064923216240853546186479164495e-46f','7.0064923216240853546186479164500e-46f','1.1754943508222875e-38f','3.4028234663852886e38f','3.4028235677973367e38f','1.000000059604644775390625f','1.000000059604644775390626f']
values.append('1'+'0'*1000+'.0e-1000')
values += ['0x0p0','0x1.8p1','0XAp-2','0x.8p+1','0x1.p0',
           '0x1.00000000000008p0','0x1.000000000000080001p0',
           '0x1.00000000000018p0','0x1p-1074','0x1p-1075',
           '0x1.0000000000001p-1075','0x1.fffffffffffffp-1023',
           '0x1p-1022','0x1.fffffffffffffp1023','0x1.fffffffffffff8p1023',
           '0x1p100000','0x1p-100000','0x0p100000',
           '0x1p-149f','0x1p-150f','0x1.000002p-150f',
           '0x1.fffffep127f','0x1.ffffffp127f',
           '0x1.000001p0f','0x1.0000010001p0f','0x1.000003p0f',
           '0x1'+'0'*1000+'p-4000']

s='#include <stdio.h>\nint main(void){\n'
for v in values:
 ty='float' if v.endswith('f') else 'double';ui='unsigned int' if ty=='float' else 'unsigned long long';fmt='%08x' if ty=='float' else '%016llx'
 s+='{ union { '+ty+' f; '+ui+' u; } x; x.f='+v+'; printf("'+fmt+'\\n",x.u); }\n'
s+='return 0;}\n';(T/'oracle.c').write_text(s)
subprocess.run(['cc','-w',str(T/'oracle.c'),'-o',str(T/'oracle')],check=True,timeout=60)
expected=subprocess.run([str(T/'oracle')],capture_output=True,check=True,timeout=10).stdout.decode().split()
assert len(expected) == len(values)
subprocess.run(['python3','exec/c/net.py',str(T/'h.tbl'),str(T/'h.net')],check=True,timeout=60,capture_output=True)
subprocess.run([str(T/'run'),'--check-net',str(T/'h.tbl'),str(T/'h.net')],check=True,timeout=60,capture_output=True)
for v,h in zip(values,expected):
 (T/'input').write_text(v+'\n')
 for model in ('h.tbl','h.net'):
  p=subprocess.run([str(T/'run'),str(T/model),str(T/'input')],capture_output=True,timeout=10)
  if p.returncode or len(p.stdout)!=8 or int.from_bytes(p.stdout,'little')!=int(h,16):
   raise RuntimeError(('BAD',v,model,h,p.returncode,p.stdout.hex(),p.stderr))
# Failure paths are exercised on the same converter, before ACCEPT.
badvalues = ('1e', '1e+', '1.2.3', '1.0ff', '9'*1600+'.0', '0xp0', '0x1p', '0x1p+', '0x1.2.3p0', '0x1p0ff')
for v in badvalues:
 (T/'input').write_text(v+'\n')
 for model in ('h.tbl','h.net'):
  p=subprocess.run([str(T/'run'),str(T/model),str(T/'input')],capture_output=True,timeout=10)
  assert p.returncode==1 and not p.stdout and b'decimal floating' in p.stderr,(v[:20],model,p.returncode,p.stderr)
print('floating bits:',len(values),'host-cc expectations, table/network agree;',len(badvalues),'rejects each')

#!/usr/bin/env python3
"""Independent SHA digest oracle; generated algorithm runs on both executors."""
import hashlib,json,pathlib,subprocess,sys
from sha256delta import install
import arm
E=arm.E
install(E)
# Two calls on the same blob check per-call state reset and input restoration.
E.P('START').a(('LDI','sh_zero',0),('XLEN','sh_end'),('BLOBSAVE','sh_blob','sh_zero','sh_end')).call('SHA256').call('SHA256').a(('ACCEPT',)).goto('DEAD')
E.g.finish()
d={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
out=pathlib.Path(sys.argv[2]);out.write_text(json.dumps(d,separators=(',',':')))
table=out.with_suffix('.tbl')
subprocess.run([sys.executable,'exec/c/tbl.py',str(out),str(table)],check=True,timeout=60)
known=[(b'', 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'),
       (b'abc','ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad'),
       (b'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq','248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1')]
cases=known+[(bytes((i*37+11)%256 for i in range(n)),None) for n in (1,55,56,63,64,65,119,120,127,128,4095,4096,4097,8192)]
f=out.with_suffix('.input')
for data,fixed in cases:
    want=hashlib.sha256(data).digest()
    if fixed:assert want.hex()==fixed
    f.write_bytes(data)
    for cmd in ([sys.argv[1],str(table),str(f)],[sys.executable,'exec/pp/sim.py',str(out),str(f)]):
        r=subprocess.run(cmd,capture_output=True,timeout=60)
        assert r.returncode==0 and r.stdout==want*2,(len(data),r.returncode,r.stderr,r.stdout.hex(),want.hex())
    print('SHA256',len(data),'bytes: both executors, repeated calls equal',flush=True)
print('SHA256 states',len(d['states']),'table',table.stat().st_size,'B')

"""Section-local branches: reference bytes, worked expectations, native flow."""
import json
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.assemble import assemble


def run(cmd): return subprocess.run(cmd,capture_output=True,timeout=60)


def check(args,native_host):
    cases=[
        ('forward','jump end\nnop\nend:\n','020000141f2003d5'),
        ('backward','start:\nnop\njump start\n','1f2003d5ffffff17'),
        ('zero','again:\njump again\n','00000014'),
        ('jz','jumpz x0, end\nnop\nend:\n','400000b41f2003d5'),
        ('shared','jump x0\nx0:\nother:\n','01000014'),
        ('call','call target\nnop\ntarget:\nret\n','91000010e72000d1f10000f9020000941f2003d5f10040f9e720009120025fd6'),
        ('variable','jump end\nimm x0, -1\n.ld x0, x1, -32769, 8\nend:\njumpz x3, end\n',None),
        ('call-back','jump entry\nbody:\nret\nentry:\ncall body\n',None),
    ]
    loop='imm x0, 0\nimm x1, 1\nimm x2, 5\nloop:\nadd64 x0, x0, x1\nsub64 x2, x2, x1\njumpz x2, done\njump loop\ndone:\n'
    invoke='call body\njump end\nbody:\nimm x0, 42\nret\nend:\n'
    cases += [('loop',loop,None),('invoke',invoke,None)]
    with tempfile.TemporaryDirectory() as tmp:
        d=pathlib.Path(tmp);f=d/'in';outputs={}
        cmds=[[args[0],args[1]], [sys.executable,'exec/pp/sim.py',args[2]]]
        for name,source,expected in cases:
            tp=parse(source,target='lnx/arm64')
            for ins in tp.code:
                if ins.op in ('jump','call','jumpz'): ins.meta['reloc']='arm19' if ins.op=='jumpz' else 'arm26'
            want,stats=assemble(tp);assert stats['encoded']==stats['insns']
            if expected is not None: assert want.hex()==expected,(name,want.hex(),expected)
            f.write_text(source)
            for cmd in cmds:
                r=run(cmd+[str(f)]);assert r.returncode==0 and r.stdout==want,(name,r.returncode,r.stderr,r.stdout.hex(),want.hex())
            outputs[name]=want
        for source in ['a:\na:\n','jump absent\n','call absent\n','jumpz x0, absent\n',':\n','a: nop\n','jumpz 0, a\na:\n']:
            f.write_text(source)
            for cmd in cmds:
                r=run(cmd+[str(f)]);assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(source,r.returncode,r.stderr)
        print('ARM64 branches: 10 fixtures on both executors; 6 worked byte expectations; 7 rejects')
        # Synthetic helper contexts: exercise the actual BR.range states without
        # materialising a 128 MB section. These are not full-layout fixtures.
        delta=json.loads(pathlib.Path(args[2]).read_text())
        def seq(actions):
            delta['seqs'].append(actions);return len(delta['seqs'])-1
        cases=[]; row={}
        for bits,shift in ((19,5),(26,0)):
            for value in (-(1<<(bits-1))-1,-(1<<(bits-1)),(1<<(bits-1))-1,1<<(bits-1)):
                key=65+len(cases);cases.append((key,bits,shift,value))
                acts=[['LDI','disp',value*4],['LDI','bits',bits],['LDI','fieldshift',shift],['PUSH','BOUND.ret']]
                row[str(key)]=['BR.range',seq(acts)]
        delta['states']['BOUND.start']=['b',row];delta['start']='BOUND.start'
        delta['states']['RET'][1]['BOUND.ret']=['BOUND.ret',seq([['POP']])]
        acts=[]
        for _ in range(4): acts += [['OUTW','disp'],['A64I','shr','disp','disp',8]]
        acts += [['ACCEPT']]
        delta['states']['BOUND.ret']=['b',{str(k):['DEAD',seq(acts)] for k in range(257)}]
        js=d/'bound.json';tb=d/'bound.tbl';js.write_text(json.dumps(delta,separators=(',',':')))
        r=run([sys.executable,'exec/c/tbl.py',str(js),str(tb)]);assert r.returncode==0,r.stderr
        for key,bits,shift,value in cases:
            f.write_bytes(bytes([key]));ok=-(1<<(bits-1))<=value<(1<<(bits-1))
            for cmd in ([args[0],str(tb)],[sys.executable,'exec/pp/sim.py',str(js)]):
                r=run(cmd+[str(f)])
                if ok: assert r.returncode==0 and r.stdout==((value&((1<<bits)-1))<<shift).to_bytes(4,'little')
                else: assert r.returncode==1 and not r.stdout
        print('ARM64 displacement helper: 8 synthetic signed-field edge contexts, both executors')

        if not native_host:
            print('ARM64 branch execution: SKIPPED (needs macOS arm64)');return
        asm=['.text']
        for name in ('loop','invoke'):
            asm += ['.globl _'+name,'_'+name+':','stp x29,x30,[sp,#-16]!','sub sp,sp,#256','add x7,sp,#256',
                    '.byte '+','.join(map(str,outputs[name])),'add sp,sp,#256','ldp x29,x30,[sp],#16','ret']
        (d/'p.s').write_text('\n'.join(asm)+'\n')
        (d/'p.c').write_text('extern long loop(void),invoke(void);int main(void){return loop()!=5 || invoke()!=42;}')
        r=run(['cc','-o',str(d/'p'),str(d/'p.c'),str(d/'p.s')]);assert r.returncode==0,r.stderr
        r=run([str(d/'p')]);assert r.returncode==0,r.returncode
        print('ARM64 branch native: backward loop and software-stack direct call execute correctly')

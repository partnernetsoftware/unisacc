"""Observation reference: shared Tape/contract/provenance and V3 decoder.
Only the explicitly witnessed variadic frame template is covered; no ABI binding claim.
"""
import sys,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT));sys.path.insert(0,str(ROOT/'exec/facts'))
from load import manifest_facts
from unisa.tape import parse
from unisa.libraryfnlower import frame,selection,check_provenance
from unisa.librarycontract import produce as validate

def fail(reason):raise ValueError('not covered: librarycallsite '+reason)
def fragment(tape,contract,selected,callsites,target,oracle):
    check_provenance(tape);owners=selection(selected);validate(b'USLDEC1\n'+contract[8:])
    if target not in (1,2,3,4) or contract[8]!=target:fail('signature')
    entries={};at=16
    for _ in range(int.from_bytes(contract[12:16],'little')):
        f=struct.unpack_from('<IIBBBBIIQIB3s',contract,at);z,ident,kind,ck,sv,flags,nn,sn,extent,fixed,var,_=f
        entries[ident]=(f,contract[at+36:at+36+nn].decode(),contract[at+36+nn:at+4+z]);at+=4+z
    records=[]
    if callsites is not None:
        if len(callsites)<16 or callsites[:8]!=b'USLCSF1\n':fail('malformed-frame')
        total=int.from_bytes(callsites[8:16],'little');at=16
        if total>4096:fail('malformed-frame')
        for index in range(total):
            if len(callsites)-at<40:fail('malformed-frame')
            length,ordinal,ident,fixed,sn=struct.unpack_from('<5Q',callsites,at)
            if length!=32+sn or sn>1048576 or at+8+length>len(callsites) or ordinal!=index+1 or ident not in entries or fixed>1024:fail('malformed-frame')
            records.append((ordinal,ident,fixed,callsites[at+40:at+8+length]));at+=8+length
        if at!=len(callsites):fail('malformed-frame')
    layout=manifest_facts('../parse2/librarycallsite-layout',ROOT/'exec/facts')
    if layout['word_width']!=8:fail('frame-layout')
    out=bytearray();spans=[];history=[]
    for rec in parse(tape).records:
        if rec[0]=='label':history=[];continue
        if rec[0]!='insn':continue
        _,op,args=rec;args=tuple(args)
        if op=='.libraryfn':
            reg,ident=args
            if owners.get(ident)!=1:raise ValueError('not covered: libraryfn contract selection')
            if ident not in entries:fail('signature')
            f,name,proto=entries[ident]
            if (f[2],f[3],f[4],f[10])!=(0,2,3,1):fail('signature')
            # Walk backwards from the stable-id instruction through the explicit frame header.
            if len(history)<15:fail('frame-layout')
            h=history[-14:]
            if h[0]!=('imm',('r1',0)) or h[1]!=('store64',('r7',layout['plan'],'r1')):fail('frame-layout')
            result=h[5][1][1] if h[5][0]=='imm' else -1
            count=h[8][1][1] if h[8][0]=='imm' else -1
            ordinal=h[10][1][1] if h[10][0]=='imm' else -1
            expected=[('imm',('r1',0)),('store64',('r7',layout['plan'],'r1')),('imm',('r1',layout['args_base'])),('add64',('r1','r7','r1')),('store64',('r7',layout['slots'],'r1')),('imm',('r1',result)),('add64',('r1','r7','r1')),('store64',('r7',layout['result'],'r1')),('imm',('r1',count)),('store64',('r7',layout['count'],'r1')),('imm',('r1',ordinal)),('store64',('r7',layout['ordinal'],'r1')),('imm',('r1',0)),('store64',('r7',layout['reserved'],'r1'))]
            if h!=expected or not isinstance(count,int) or count<0 or count>1024:fail('frame-layout')
            before=history[:-14];copies=[]
            while before and before[-1][0]!='.frame':copies.insert(0,before.pop())
            if not before or len(copies)!=2*count:fail('frame-layout')
            expectedcopies=[]
            for i in range(count):expectedcopies += [('load64',('r2','r6',layout['stack_args_base']+8*i)),('store64',('r7',layout['args_base']+8*i,'r2'))]
            if copies!=expectedcopies or result!=layout['args_base']+8*count or before[-1]!=('.frame',(result+8,)):fail('frame-layout')
            if callsites is None:fail('missing-callsite')
            found=False
            for order,symbol,fixed,concrete in records:
                if symbol!=ident:continue
                if fixed!=f[9]:fail('signature')
                with tempfile.TemporaryDirectory(prefix='callsite-signature-reference-') as d:
                    p=Path(d);(p/'prototype').write_bytes(proto);(p/'concrete').write_bytes(concrete)
                    rc=subprocess.run([oracle,name,str(fixed),str(p/'prototype'),str(p/'concrete'),'0'],capture_output=True,timeout=3).returncode
                    if rc not in (0,1):raise RuntimeError('reference mechanism failure')
                    if rc:fail('signature')
                if order==ordinal:
                    # concrete argument count follows its name and four signature flags.
                    nn=int.from_bytes(concrete[16:24],'little');actual=int.from_bytes(concrete[28+nn:36+nn],'little')
                    if actual!=count:fail('frame-layout')
                    found=True
            if not found:fail('missing-callsite')
            if reg!='r1':fail('frame-layout')
            out.extend(('setreg '+('x1' if target in (2,4) else 'rdi')+', imm ').encode());spans.append((len(out),ident));out.extend(b'0\n')
        history.append((op,args))
    return frame(bytes(out),spans)
if __name__=='__main__':
    try:
        a=sys.argv[1:];result=fragment(Path(a[0]).read_text(),Path(a[1]).read_bytes(),Path(a[2]).read_bytes(),None if a[3]=="--absent" else Path(a[3]).read_bytes(),int(a[4]),a[5])
    except (ValueError,struct.error,UnicodeError) as e:print(e,file=sys.stderr);sys.exit(1)
    except AssertionError as e:print('outside tapecheck domain: '+str(e),file=sys.stderr);sys.exit(2)
    except (OSError,RuntimeError,subprocess.TimeoutExpired) as e:print('reference mechanism failure: '+str(e),file=sys.stderr);sys.exit(2)
    sys.stdout.buffer.write(result)

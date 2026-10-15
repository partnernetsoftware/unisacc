"""Observation only: reuse Tape/provenance/UNILOW1 reference and existing C V3 decoder.
The supported witness is an explicit pruned Tape template with r1; no provider state.
"""
import sys,struct,subprocess,tempfile,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from unisa.libraryfnlower import frame,selection,check_provenance,supported as ordinary
from unisa.librarycontract import produce as validate
from unisa.tape import parse

def fragment(tape,contract,selected,target,library_mode,oracle):
    check_provenance(tape)
    owners=selection(selected)
    validate(b'USLDEC1\n'+contract[8:])
    def fail():raise ValueError('not covered: libraryfn fixed typed ABI')
    if contract[:8]!=b'USLCON1\n' or target not in (1,2,3,4) or contract[8]!=target:fail()
    entries={};at=16
    for _ in range(int.from_bytes(contract[12:16],'little')):
        f=struct.unpack_from('<IIBBBBIIQIB3s',contract,at)
        z,ident,kind,callkind,version,flags,nn,sn,extent,count,var,_=f
        entries[ident]=(f,contract[at+36:at+36+nn].decode('ascii'),contract[at+36+nn:at+4+z]);at+=4+z
    out=bytearray();spans=[]
    for insn in parse(tape).code:
        if insn.op!='.libraryfn':continue
        if not library_mode:raise ValueError('not covered: libraryfn requires library mode')
        reg,ident=insn.args
        if owners.get(ident)!=1:raise ValueError('not covered: libraryfn contract selection')
        if ident not in entries:fail()
        f,name,sig=entries[ident]
        if f[2:5]==(0,0,0):ordinary(contract,ident,target)
        else:
            if (f[2],f[3],f[4],f[10])!=(0,1,3,0):fail()
            with tempfile.TemporaryDirectory(prefix='typed-signature-reference-') as d:
                p=pathlib.Path(d)/'signature';p.write_bytes(sig)
                x=subprocess.run([oracle,name,str(f[9]),str(p)],capture_output=True,timeout=3)
                if x.returncode not in (0,1):raise RuntimeError('reference mechanism failure')
                if x.returncode:fail()
        if reg!='r1':raise ValueError('reference witness register is not covered')
        out.extend(('setreg '+('x1' if target in (2,4) else 'rdi')+', imm ').encode());spans.append((len(out),ident));out.extend(b'0\n')
    return frame(bytes(out),spans)

if __name__=='__main__':
    try:
        args=sys.argv[1:];b=fragment(pathlib.Path(args[0]).read_text(),pathlib.Path(args[1]).read_bytes(),pathlib.Path(args[2]).read_bytes(),int(args[3]),bool(int(args[4])),args[5])
    except (ValueError,struct.error,UnicodeError) as e:print(e,file=sys.stderr);sys.exit(1)
    except AssertionError as e:print("reference fixture is outside tapecheck domain: "+str(e),file=sys.stderr);sys.exit(2)
    sys.stdout.buffer.write(b)

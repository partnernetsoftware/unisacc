"""Real ARM Windows PE from delta encoder/writer, with reference as referee."""
import pathlib,subprocess,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.__main__ import _oracle
from unisa.driver import compile_file
from unisa.lower import lower
from unisa.assemble import assemble
from unisa import image
from tins import dump
from pecheck import verify

def main():
    assert len(sys.argv)==4
    out=pathlib.Path(sys.argv[3]);out.mkdir(parents=True,exist_ok=True)
    oracle=_oracle('built')
    for name in ('hello','fib'):
        source='examples/'+name+'.c'
        tp=lower(compile_file([source],oracle,'win/arm64'),'win/arm64',oracle,drive='built')
        f=out/(name+'.tins');f.write_text(dump(tp,full=True))
        text,st=assemble(tp);assert st['encoded']==st['insns']
        want=image.build(tp,text,image.relocate(tp,tp.data,st['data_va']-256),st['entry'])
        r=subprocess.run([sys.argv[1],sys.argv[2],str(f)],capture_output=True,timeout=60)
        assert r.returncode==0,(name,r.stderr)
        assert r.stdout==want,(name,len(r.stdout),len(want))
        verify(r.stdout);(out/(name+'.exe')).write_bytes(r.stdout)
        host=out/(name+'.host')
        subprocess.run(['cc','-include','stdio.h',source,'-o',str(host)],check=True,timeout=60)
        expected=subprocess.run([str(host)],capture_output=True,timeout=60)
        assert expected.returncode==0
        (out/(name+'.want')).write_bytes(expected.stdout)
        print('PE real win/arm64',name,len(r.stdout),'bytes equal; host cc expected output stored',flush=True)
if __name__=='__main__':main()

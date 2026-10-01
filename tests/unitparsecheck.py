#!/usr/bin/env python3
"""E3 unit-mode linkage and startup match the C reference tape exactly."""
import difflib, json, os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT), str(ROOT/"exec/c")]
from pack import build
from exec.pp.sim import load, run
class Files:
    def __init__(self, unit): self.unit = unit
    def get(self, key): return b'1' if self.unit and key == b'\0cli/funit' else None
CASES = {
    'function': 'int twice(int x){return x*2;}\n',
    'global': 'int count=3; static int hidden; int get(void){return count+hidden;}\n',
    'external': 'extern int count; int twice(int); int get(void){return twice(count);}\n',
    'main': 'int main(void){return 0;}\n',
    'static': 'static int f(void){return 2;} int g(void){return f();}\n',
}
def main():
    delta = json.loads(pathlib.Path(sys.argv[1]).read_bytes()); loaded = load(delta)
    ref = sys.argv[2]; dumper = sys.argv[3]; count = 0
    with tempfile.TemporaryDirectory(prefix='unisacc-unitparse-') as tmp:
        tmp = pathlib.Path(tmp)
        runtime = tmp/'run'; table = tmp/'model.tbl'; net = tmp/'model.net'
        def command(args):
            result = subprocess.run(list(map(str,args)),capture_output=True,timeout=30)
            assert result.returncode == 0, (args,result.returncode,result.stderr[-2000:])
            return result.stdout
        command(['cc','-O2','-o',runtime,ROOT/'exec/c/run.c'])
        command([sys.executable,ROOT/'exec/c/tbl.py',sys.argv[1],table])
        command([sys.executable,ROOT/'exec/c/net.py',table,net])
        command([runtime,'--check-net',table,net])
        manifest=tmp/'routes'; manifest.write_text('parse\tparse\ttokens\ttape\tmodel.net\n')
        resources=tmp/'resources';resources.mkdir();(resources/'funit').write_bytes(b'1')
        package=tmp/'package';package.write_bytes(build([manifest],[('00636c692f',resources)],cache=False))
        for name, source in CASES.items():
            path = pathlib.Path(tmp)/(name+'.c'); path.write_text(source)
            tokens = subprocess.check_output([dumper,'-dump-tokens',str(path)],env=dict(os.environ,UA_TYPESPELL='1'),timeout=10)
            expected = subprocess.check_output([ref,str(path),'-S','-funit','-o','-'],timeout=10)
            status, actual, steps = run(delta,tokens,str(path),Files(True),maxsteps=5000000,loaded=loaded)
            if status != 'accept' or actual != expected:
                if status == 'accept': print(''.join(difflib.unified_diff(expected.decode().splitlines(True),actual.decode().splitlines(True))))
                raise AssertionError((name,status,actual if status != 'accept' else 'tape differs'))
            tokenfile=tmp/'tokens';tokenfile.write_bytes(tokens)
            native=command([runtime,'--bundle',package,'parse',tokenfile])
            assert native == expected, (name,'constructed network differs')
            count += 1
        path = ROOT/'examples/fib.c'
        tokens=subprocess.check_output([dumper,'-dump-tokens',str(path)],env=dict(os.environ,UA_TYPESPELL='1'),timeout=10)
        expected=subprocess.check_output([ref,str(path),'-S','-o','-'],timeout=10)
        status,actual,_=run(delta,tokens,str(path),Files(False),maxsteps=50000000,loaded=loaded)
        assert status=='accept' and actual==expected, 'whole program changed'
    print('unit parse: %d simulator/network/reference tapes identical; whole program unchanged' % count)
if __name__ == '__main__': main()

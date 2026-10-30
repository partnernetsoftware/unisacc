#!/usr/bin/env python3
"""Opt-in native execution of a prepared model driver/package on POSIX.
Run in a copied test tree; argv: driver package. Does not generate models.
The system C compiler is the behavioural referee, not the image writer.
"""
import os,pathlib,subprocess,sys,tempfile
driver=str(pathlib.Path(sys.argv[1]).resolve())
package=str(pathlib.Path(sys.argv[2]).resolve())
def run(cmd,**kw):
    return subprocess.run(cmd,capture_output=True,timeout=15,**kw)
def require(r):
    assert r.returncode==0,(r.args,r.returncode,r.stderr)
probes=['examples/hello.c','examples/fib.c','examples/struct.c',
        'tests/c/b_argv.c','tests/c/b_printf.c','tests/c/b_static.c',
        'tests/c/b_funcptr.c','tests/c/b_file.c','tests/c/b_malloc.c']
with tempfile.TemporaryDirectory(prefix='native-memory-') as td:
    td=pathlib.Path(td);count=0
    for name in probes:
        source=str(pathlib.Path(name).resolve());exe=str(td/'reference')
        require(run(['cc','-include','stdio.h',source,'-o',exe]))
        # The first argument is intentionally the source path on both sides.
        ref=run([source,'one','two'],executable=exe,cwd=td)
        assert 0<=ref.returncode<128,(name,ref.returncode,ref.stderr)
        for level in (0,1,2):
            got=run([driver,'--models',package,'-O'+str(level),'-run',source,'--','one','two'],cwd=td)
            assert (got.returncode,got.stdout,got.stderr)==(ref.returncode,ref.stdout,ref.stderr),(name,level,ref,got)
            count+=1
            print(name,'O'+str(level),'output/status match system cc',flush=True)
    source=td/'env.c'
    source.write_text('#include <stdio.h>\n#include <stdlib.h>\nint main(int n,char **v){printf("%d %s %s\\n",n,v[1],getenv("MEMORY_PROBE"));return 7;}\n')
    got=run([driver,'--models',package,'-O1','-run',str(source),'--','-argument'],env=dict(os.environ,MEMORY_PROBE='present'))
    assert (got.returncode,got.stdout,got.stderr)==(7,b'2 -argument present\n',b''),got
    got=run([driver,'--models',package,'-run',str(td/'absent.c')])
    assert got.returncode==2 and not got.stdout and b'cannot open' in got.stderr,got
    print('native memory:',count,'system-cc comparisons, argv/env/exit and explicit missing input passed')

#!/usr/bin/env python3
"""Run a Python construction tool and record what it read (P8 read-set keys).

readtrace.py LOG ROOT TOOL ARGS...: runs TOOL as __main__ under an audit hook
and writes LOG as JSON: repo files opened (missing ones included), repo
directories listed, and whether any process was spawned (a spawn makes the
read set incomplete, so the caller falls back to the whole-closure key)."""
import atexit,json,os,runpy,sys
log,root=sys.argv[1],os.path.realpath(sys.argv[2])
files=set();dirs=set();spawn=[False]
def _rel(p):
    p=os.path.realpath(os.fsdecode(p))
    if p==root: return '.'
    if not p.startswith(root+os.sep) or '__pycache__' in p.split(os.sep): return None
    return os.path.relpath(p,root).replace(os.sep,'/')
def _hook(ev,args):
    if ev=='open' and args and isinstance(args[0],(str,bytes,os.PathLike)):
        r=_rel(args[0])
        if r is not None: files.add(r)
    elif ev in ('os.listdir','os.scandir'):
        r=_rel(args[0] if args and args[0] is not None else '.')
        if r is not None: dirs.add(r)
    elif ev in ('subprocess.Popen','os.posix_spawn','os.exec','os.system','os.spawn','os.fork','os.forkpty'):
        spawn[0]=True
sys.addaudithook(_hook)
@atexit.register
def _dump():
    with open(log,'w') as f: json.dump({'files':sorted(files),'dirs':sorted(dirs),'spawn':spawn[0]},f)
tool=sys.argv[3];sys.argv=sys.argv[3:];sys.path[0]=os.path.dirname(os.path.abspath(tool))
runpy.run_path(tool,run_name='__main__')

"""Explicit library/module=LE64(1): no process startup, fixed __init role.
Missing/zero preserves program output. __init is generated, zero-argument,
soft-stack ABI, called once per mapping by the host's declared adapter.
"""
from pathlib import Path
import assemble
import json
from finite_rules import install as rules, install_template

def _template(E,name,facts={}):
    install_template(E.g,Path(__file__).parent,'librarymodule',facts,None,section=name)

def install(E,P,start):
    g=E.g
    assemble.run(Path(__file__).resolve().parent/'libraryresources-manifest.tsv',E,E.P,dict(lx=0,lc=0,lmd=1),{})   # libraryresources-manifest.tsv
    rules(g,Path(__file__).parent,'librarymodule',section='read',bindings=dict(start=start))
    _template(E,'start')
    # Split only the declared startup output, preserving positioning/continuation.
    F=assemble.load_facts('k2-librarymodule'); R=E.results   # startup-run named results + header edge facts
    _template(E,'split',{'H':[dict(s=R['lm_hstate'],k=k,before=json.dumps(F['hbefore'])) for k in range(*F['hdomain'])]})
    rules(g,Path(__file__).parent,'librarymodule',section='header',bindings=dict(next=F['hnext']),sequences=dict(header=list(R['lm_header']),tail=[('PUSH',R['lm_hnext'])]))
    # Main requirement and init-tail are existing finite control points.
    state=E.results['lm_main']   # gen2 global3 named result (global-results.tsv)
    normal,reject,keys=F['mainnext'],E.rej(F['mainreason']),[k for k in range(*F['maindomain']) if k not in F['mainok']]
    if R.get('lm_errors'):   # BOUNDARY: errors stage (block 3) re-targets the no-main reject to its own fresh message state
        normal,reject={(n,tuple(g.seqs[q])) for k,(n,q) in g.st[state][1].items() if n!='END.ok'}.pop();reject=list(reject)
    _template(E,'main',{'state':[state],'key':keys})
    state=E.results['lm_initret']
    _template(E,'initmove',{'state':[state]});_template(E,'initend',{'state':[state]})
    tail=E.results['lm_tailret']
    rules(g,Path(__file__).parent,'librarymodule',section='finish',bindings=dict(tail=tail,normal=normal),sequences=dict(ret=E.O('  ret\n'),reject=reject))
    # __init is reserved only for explicit modules; source cannot counterfeit role.
    _template(E,'defmove');_template(E,'definition')
    rules(g,Path(__file__).parent,'librarymodule',section='reserved')
    return 'LMD.start'

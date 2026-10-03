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
    header=list(E.O(E.HEADER)); hits=[]
    for state,(mode,row) in list(g.st.items()):
        for k,(n,q) in list(row.items()):
            seq=list(g.seqs[q])
            for i in range(len(seq)-len(header)+1):
                if seq[i:i+len(header)]==header:hits.append((state,k,n,seq[:i],seq[i+len(header):]))
    assert len({h[0] for h in hits})==1 and hits
    state,k,n,before,after=hits[0]
    for state,k,nn,bb,aa in hits:
        assert (nn,bb,aa)==(n,before,after)
    _template(E,'split',{'H':[dict(s=state,k=k,before=json.dumps([list(x) for x in bb])) for state,k,nn,bb,aa in hits]})
    rules(g,Path(__file__).parent,'librarymodule',section='header',bindings=dict(next=n),sequences=dict(header=header,tail=after))
    # Main requirement and init-tail are existing finite control points.
    state=E.results['lm_main'];mode,row=g.st[state]   # gen2 global3 named result (global-results.tsv)
    reject=None;normal=None;keys=[]
    for k,(n,q) in list(row.items()):
        if n!='END.ok':
            actions=list(g.seqs[q])
            assert normal is None or (normal,reject)==(n,actions)
            normal,reject=n,actions;keys.append(k)
    assert normal is not None
    _template(E,'main',{'state':[state],'key':keys})
    state=E.results['lm_initret']
    _template(E,'initmove',{'state':[state]});_template(E,'initend',{'state':[state]})
    tail=E.results['lm_tailret']
    rules(g,Path(__file__).parent,'librarymodule',section='finish',bindings=dict(tail=tail,normal=normal),sequences=dict(ret=E.O('  ret\n'),reject=reject))
    # __init is reserved only for explicit modules; source cannot counterfeit role.
    _template(E,'defmove');_template(E,'definition')
    rules(g,Path(__file__).parent,'librarymodule',section='reserved')
    return 'LMD.start'

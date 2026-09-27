"""Block-scope static storage, using the shared scope/type/load/store procedures.
LOC encodes a static object's label as -(token ordinal + 1); positive slots
still mean frame offsets or GMARK. No language primitive is added to run.c.
"""

def install(E, P, TIX, SINIT, SIEND, LOC, SKIPS, BOOL):
    import pathlib,re
    from finite_rules import install as install_rules
    # Same bounded ordinal namespace as the product; single-unit labels stay
    # unchanged. This is a declaration constant, not emitted reference code.
    source=(pathlib.Path(E.ROOT)/'src/front_pp.c').read_text()
    limits=re.findall(r'^#define MAXTOK ([0-9]+)\b',source,re.M)
    assert len(limits)==1 and 0<int(limits[0])<(1<<25)
    unit_span=int(limits[0])
    bindings = dict(TIX=TIX, SINIT=SINIT, SIEND=SIEND, LOC=LOC, SKIPS=SKIPS,
                    BASE=E.BASE, ARR=E.ARR, PTR=E.PTR, MAXTOK=unit_span)
    root = pathlib.Path(__file__).parent
    for line in (root/'statics-names.tsv').read_text().splitlines():
        if not line.startswith('#'):
            name, prefix, kind = line.split('\t')
            bindings[name] = P(prefix+'.statics_'+name).fresh(kind)
    saved = ('si_pos','si_lab','si_out','v','bd','tb','td','dar','dsz')
    sequences = {'save': P('SC.saved_push').vpush(*saved).acts,
                 'restore': P('SC.saved_pop').vpop(*saved).acts}
    sequences.update({name: E.rej(text) for text, name in {'not covered: static declarator': 'reject0', 'not covered: expected ;': 'reject1', 'not covered: static array initializer': 'reject2'}.items()})
    classes = {'TK_'+str(i): [E.TK[k]] for i,k in
               enumerate(('eof','(','[','=',';',',','{'))}
    classes.update(TK_ID=[E.TK_ID], TK_STR=[E.TK_STR])
    # A token walk caches source ordinals by position; rewinds reuse them.
    # BOOL is retained in the public signature; shared ELSZ/ASSIGNCV own its rules.
    del E.g.st['NEXT']
    install_rules(E.g, root, 'statics', bindings=bindings,
                  sequences=sequences, classes=classes)

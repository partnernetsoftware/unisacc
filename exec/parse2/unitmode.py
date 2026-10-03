"""Separate compilation linkage and entry selection, as ordinary delta actions."""
DECL = 230 << 40

def install(E, P, start, definitions):
    g = E.g
    # FN clears fstatic after taking the declaration specifier. Keep the
    # storage fact for the later GV.storage/GV.record hooks.
    mode, row = g.st['FN']
    for key, (target, seq) in list(row.items()):
        row[key] = (target, g.seq([('COPYW', 'um_static', 'fstatic')] + list(g.seqs[seq])))
    def original(state):
        name = 'UM.original.' + state
        assert name not in g.st
        g.st[name] = g.st.pop(state)
        for how in ('P', 'L'):
            if (state, how) in definitions:
                definitions[name, how] = definitions.pop((state, how))
        g.labels.add(name)
        return name
    # Stage control lives in unitmode-result.tsv (sections start, header, s2..s8);
    # fresh labels are declared in unitmode-fresh.tsv and allocated in the original
    # order on an unregistered scope. Python keeps only fact-dependent work: the FN
    # row rewrite, original() renames, the header relocation (its matched rows and
    # UM.header.resume continuations), and the END.ok initializer tail/target.
    from pathlib import Path
    from finite_rules import install as rules
    class _Scope:
        def __init__(self, cur): self.cur = cur
    root = Path(__file__).parent
    bindings = dict(start=start, DECL=DECL, DECL1=DECL+1, FND=E.FND,
                    TK_extern=E.TK['type=extern'], TK_assign=E.TK['='])
    sequences = dict(header=list(E.O(E.HEADER)))
    fresh = [l.split('\t') for l in (root/'unitmode-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part, key, kind, prefix in fresh:
            if part == name: bindings[key] = E.P.fresh(_Scope(prefix), kind)
        rules(g, root, 'unitmode', bindings, sequences, None, name)
    section('start')
    # Move the fixed entry sequence from before the source to before __init_u.
    header = list(E.O(E.HEADER))
    matches = []
    for state, (mode, row) in list(g.st.items()):
        for key, (target, seq) in row.items():
            acts = list(g.seqs[seq])
            for at in range(len(acts) - len(header) + 1):
                if acts[at:at+len(header)] == header:
                    matches.append((state, key, target, acts, at))
                    break
    states = {state for state, *_ in matches}
    assert len(states) == 1, states
    for state, key, target, acts, at in matches:
        resume = 'UM.header.resume'
        g.labels.add(resume)
        g.st[state][1][key] = ('UM.header', g.seq(acts[:at] + [('PUSH', resume)]))
        g.on(resume, range(257), target, acts[at+len(header):], 'r')
    section('header')
    original('FN.def1'); section('s2')
    original('GV.storage'); section('s3')
    original('GV.record'); section('s4')
    original('END')
    # Keep the existing initializer replay and footer; replace only its heading.
    init = list(E.O('__init:\n'))
    mode,row = g.st['END.ok']
    targets = set()
    for key,(target,seq) in row.items():
        acts=list(g.seqs[seq]);assert acts[:len(init)]==init
        targets.add((target,tuple(acts[len(init):])))
    assert len(targets)==1
    bindings['init_target'], acts = targets.pop()
    sequences['init_tail'] = list(acts)
    section('s5')
    # cc interop (reference -c -b without -funit): an undefined prototyped
    # call becomes a __ccx_ thunk and an `extern` function defined here a
    # __ccw_ entry.  E3 builds neither: such objects are refused by name.
    original('FN.bodykind'); section('s6')
    # Undefined calls become declarations in source emission order.
    original('UD.error'); section('s7')
    original('UD.ok'); section('s8')
    return 'UM.start'

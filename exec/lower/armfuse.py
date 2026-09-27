"""ARM fusion control is declared in TSV; live SHAPE selects shared scan instances."""
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules


def rules(E, section, bindings, sequences=None):
    for line in Path(__file__).with_name('armfuse-names.tsv').read_text().splitlines():
        if not line.startswith('#'):
            selected, name, owner, kind = line.split('\t')
            if selected == section:
                bindings[name] = E.P(owner + '.binding_' + name).fresh(kind)
    install_rules(E.g, Path(__file__).parent, 'armfuse', bindings=bindings,
                  sequences=sequences or {}, section=section)


def install(E, ids, OP, KIND, ARG):
    bindings = dict(OP=OP, KIND=KIND, ARG=ARG)
    bindings.update(('id_'+key, value) for key, value in ids.items())
    rules(E, 'frame', bindings)


def immediate(E, ids, OP, KIND, ARG, TXT):
    from unisa.tape import SHAPE
    bindings = dict(OP=OP, KIND=KIND, ARG=ARG, TXT=TXT)
    bindings.update(('id_'+key, value) for key, value in ids.items())
    rules(E, 'immediate_head', bindings)
    # Apply declared eligibility to the current SHAPE order and fields.
    policy = [line.split('\t') for line in
              Path(__file__).with_name('armfuse-shapes.tsv').read_text().splitlines()
              if line and not line.startswith('#')]
    shapes = {tuple(value.split(',')) for kind,value in policy if kind=='shape'}
    excluded = {value for kind,value in policy if kind=='exclude'}
    reads = {value for kind,value in policy if kind=='read'}
    current = 'AI.shape'
    initial = load_rules(Path(__file__).with_name('armfuse-result.tsv'), {},
                         bindings=bindings, section='shape_init')['actions'][0][1]
    for i, (op, shape) in enumerate((o,s) for o,s in SHAPE.items() if s in shapes and o not in excluded):
        read, nxt = 'AI.read'+str(i), 'AI.next'+str(i)
        test = E.P(current).fresh('b')
        rules(E, 'shape_match', dict(bindings, **{'from': current, 'test': test, 'hit': read, 'next': nxt, 'op': ids[op]}), {'init': initial})
        initial = []
        for j, kind in enumerate(shape[1:], 1):
            if kind in reads:
                after = 'AI.read'+str(i)+'.'+str(j)
                test = E.P(read).fresh('b')
                rules(E, 'shape_read', dict(bindings, **{'from': read, 'test': test, 'next': after, 'index': j}))
                read = after
        rules(E, 'shape_dest', dict(bindings, **{'from': read}))
        current = nxt
    rules(E, 'shape_end', dict(bindings, **{'from': current}), {'init': initial})
    rules(E, 'immediate_tail', bindings)

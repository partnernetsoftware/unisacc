"""Shared string-span, initializer and parameterized byte-walk rules in TSV."""
from pathlib import Path
from finite_rules import install as install_rules


def rules(E, P, section, bindings=None, owner=None):
    bindings = dict(bindings or {}, TK_STR=E.TK_STR)
    for line in Path(__file__).with_name('strings-names.tsv').read_text().splitlines():
        if not line.startswith('#'):
            selected, name, prefix, kind = line.split('\t')
            if selected == section:
                bindings[name] = P((owner or prefix) + '.strings_' + name).fresh(kind)
    sequences = {name: E.rej(text) for text, name in {'not covered: string prefix': 'reject0', 'not covered: truncated string token': 'reject1', 'not covered: truncated string escape': 'reject2', 'not covered: adjacent string prefix': 'reject3', 'not covered: string escape': 'reject4'}.items()}
    install_rules(E.g, Path(__file__).parent, 'strings', bindings=bindings,
                  sequences=sequences, section=section)


def token_span(E, P):
    del E.g.st['SPANSTR']
    rules(E, P, 'token_span')


def initializer(E, P, esc):
    rules(E, P, 'initializer_head')
    walk(E, P, esc, 'SI.walk', 'SI.byte', 'SI.end')
    rules(E, P, 'initializer_tail')


def walk(E, P, esc, pre, body, done):
    bindings = {'walk'+suffix.replace('.', '_'): pre+suffix for suffix in
                ('', '.w', '.es', '.gap', '.hex', '.hexend', '.oct', '.octstep')}
    bindings.update(body=body, done=done)
    # Keep adjacent-prefix rejection before escape rejection for error-message numbering.
    rules(E, P, 'walk_head', bindings)
    # Only escape-map data is materialized here; digits/x retain their reserved forms.
    for ch, value in esc.items():
        if ch not in '01234567x':
            E.g.on(pre+'.es', [ord(ch)], body, [('ADV',), ('LDI','bv',value)])
    rules(E, P, 'walk_tail', bindings, owner=pre.split('.')[0])

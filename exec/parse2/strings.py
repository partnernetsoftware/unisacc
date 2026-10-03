"""Shared narrow/wide string spans, initialization and code-point walks in TSV."""
from pathlib import Path
from finite_rules import install as install_rules, install_template


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
    install_template(E.g, Path(__file__).parent, 'strings', {}, None, section='token_span_drop')
    rules(E, P, 'token_span')


def initializer(E, P, esc):
    rules(E, P, 'initializer_head')
    walk(E, P, esc, 'SI.walk', 'SI.byte', 'SI.end')
    rules(E, P, 'initializer_tail')


def walk(E, P, esc, pre, body, done):
    bindings = {'walk'+suffix.replace('.', '_'): pre+suffix for suffix in
                ('', '.w', '.es', '.gap', '.hex', '.hexend', '.oct', '.octstep', '.start', '.quote', '.setup', '.setup_test', '.wide', '.char', '.char_test', '.rawbyte', '.utf', '.cont', '.contstep', '.contstep_test', '.min', '.min_test', '.max', '.max_test', '.surrogate', '.surrogate_test', '.surrogateend', '.surrogateend_test', '.bad')}
    bindings.update(body=body, done=done)
    # Keep adjacent-prefix rejection before escape rejection for error-message numbering.
    rules(E, P, 'walk_head', bindings)
    # Only escape-map data is materialized here; digits/x retain their reserved forms.
    reserved = {int(byte) for line in Path(__file__).with_name('strings-escape-policy.tsv').read_text().splitlines()[1:]
                for byte in line.split('\t')[1].split(',')}
    escape = [{'byte': ord(ch), 'value': value} for ch, value in esc.items() if ord(ch) not in reserved]
    install_template(E.g, Path(__file__).parent, 'strings', {'escape': escape}, None, bindings=bindings,
                     section='walk_escape', mode='b', domain=[e['byte'] for e in escape])
    rules(E, P, 'walk_tail', bindings, owner=pre.split('.')[0])

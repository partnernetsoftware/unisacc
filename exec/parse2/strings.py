"""Shared narrow/wide string spans, initialization and code-point walks in TSV."""
from pathlib import Path
from finite_rules import install as install_rules, install_template
import sys as _s; from pathlib import Path as _P; _s.path.insert(0, str(_P(__file__).resolve().parent.parent / 'facts')); from load import facts


def rules(E, P, section, bindings=None, owner=None):
    bindings = dict(bindings or {}, TK_STR=E.TK_STR)
    for line in Path(__file__).with_name('strings-names.tsv').read_text().splitlines():
        if not line.startswith('#'):
            selected, name, prefix, kind = line.split('\t')
            if selected == section:
                bindings[name] = P((owner or prefix) + '.strings_' + name).fresh(kind)
    sequences = {r['name']: E.rej(r['value']) for r in facts('strings') if r['kind'] == 'reject'}
    install_rules(E.g, Path(__file__).parent, 'strings', bindings=bindings,
                  sequences=sequences, section=section)


def token_span(E, P):
    install_template(E.g, Path(__file__).parent, 'strings', {}, None, section='token_span_drop')
    rules(E, P, 'token_span')


# initializer: exec/parse2/strings-initializer-manifest.tsv (gen2 runs it).


def walk(E, P, esc, pre, body, done):
    bindings = {r['name']: pre + r['value'] for r in facts('strings') if r['kind'] == 'walk'}
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

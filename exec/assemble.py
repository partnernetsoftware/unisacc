"""Ordered assembly of finite-rule manifests; fact parsing lives in exec.facts.load."""
import json
import re
from pathlib import Path
from types import SimpleNamespace
from finite_rules import install, install_rows, install_template
from facts.load import manifest_facts, typed_cell as _cell
FACTS = Path(__file__).resolve().parent / "facts"
_cache = {}
def load_facts(stem):
    path = FACTS / (stem + ".tsv")
    if path not in _cache: _cache[path] = manifest_facts(stem, FACTS)
    return _cache[path]

def _path(facts, ref):
    v = facts
    for p in ref.split('.'):
        v = v[int(p)] if isinstance(v, list) else v[p]
    return v

class Run:
    def __init__(self, E, P, flags, env):
        self.E, self.P, self.flags, self.env = (E, P, flags, env)
        self.holders, self.root, self.accum, self.done = {}, None, {}, False
    @staticmethod
    def holder(cur):
        return SimpleNamespace(cur=cur)
    def fresh(self, spec):
        spec = self.interp(spec)
        if spec in ('', '-'):
            return None
        if spec == 'none':
            return lambda k: None
        if spec == 'split':
            return lambda k: self.E.P.fresh(self.holder(k.split('_')[0]), k.split('_')[1])
        kind, _, name = spec.partition(':')
        if spec.startswith('='):
            h = self.holders[spec[1:]]
            if isinstance(h, self.P):
                return h.fresh
        elif kind == 'P':
            return self.P(name).fresh
        elif kind in ('U', 'S'):
            h = self.holder(name)
        elif kind == 'tape':
            return self._tape(name)
        else:
            raise ValueError('fresh ' + spec)
        return lambda k, h=h: self.E.P.fresh(h, k)
    def _tape(self, spec):
        file, _, part = spec.partition('@')
        labels = {}
        for line in (self.root / file).read_text().splitlines():
            fields = line.split('\t')
            if line and not line.startswith('#') and fields[0] == part:
                labels[fields[3]] = self.E.P.fresh(self.holder(fields[1]), fields[2])
        return labels.__getitem__
    def value(self, v, facts):
        v = self.interp(v)
        kind, _, arg = v.partition(':')
        if kind == '@rej': return self.E.rej(arg)
        if kind == '@bytes':
            text = str(_path(facts, arg[1:])) if arg.startswith('=') else arg
            return [('SBOUT', c) for c in text.encode()]
        if kind == '@str':
            return _cell('str', re.sub(r'\{(\w+)\}', lambda m: str(facts[m.group(1)]), arg))
        if kind == '@out': return _out(arg, facts)
        if kind == '@fmt': return _fmt(arg, facts)
        if kind == '@ref': return _path(facts, _fmt(arg, facts))
        if kind == '@seqmap': return self.seqmap(*arg.rsplit(':', 1), facts=facts)
        if kind == '@acts':
            def tup(x): return tuple(map(tup, x)) if isinstance(x, list) else x
            return [tup(a) for a in _path(facts, arg)]
        if kind == '@textf': return self.E.O(_path(facts, arg))
        if kind == '@text': return self.E.O(json.loads(arg))
        if kind == 'fresh':
            scope, k = arg.rsplit(':', 1)
            return self.fresh(_fmt(scope, facts) if '{' in scope else scope)(k)
        if v.startswith('$$'):
            domain, _, key = v[2:].partition(':')
            return self.env[domain][_fmt(key, facts) if re.search(r'\{\w[\w\[\]]*\}', key) else key]
        if v.startswith('$'): return self.env[v[1:]]
        return _path(facts, v)
    def cells(self, s, facts):
        if s in ('', '-'): return None
        pairs = (item.partition('=') for item in s.split(','))
        return {k: self.value(v, facts) for k, _, v in pairs}
    @staticmethod
    def rows(manifest):
        out = []
        for ln in Path(manifest).read_text().split('\n'):
            if ln and (not ln.startswith('#')):
                f = (ln.split('\t') + ['-'] * 9)[:9]
                depth = len(f[0]) - len(f[0].lstrip('.'))
                out.append((depth, [f[0].lstrip('.')] + f[1:]))
        return out
    def run(self, manifest):
        self.root = Path(manifest).parent
        self.block(self.rows(manifest), 0, {})
        return self.env
    def block(self, rows, depth, extra):
        i = 0
        while i < len(rows) and (not self.done):
            d, row = rows[i]
            assert d == depth, rows[i]
            j = i + 1
            while j < len(rows) and rows[j][0] > depth:
                j += 1
            self.one(row, rows[i + 1:j], depth, extra)
            i = j
    def one(self, row, body, depth, extra):
        op, stem, section, when, factn, fresh, seq, bind, opts = row
        facts = dict(self.env)
        for s in [] if factn in ('', '-') else factn.split('+'):
            facts.update(load_facts(s[5:] if s.startswith('load:') else s))
        facts.update(extra)
        if not _when(when, self.flags, facts):
            return
        o = {} if opts in ('', '-') else json.loads(opts)
        if 'once' in o:
            seen = self.E.g.__dict__.setdefault('once', set())
            if o['once'] in seen:
                self.done = True
                return
            seen.add(o['once'])

        def lv(v):
            return [lv(x) for x in v] if isinstance(v, list) else {a: lv(x) for a, x in v.items()} if isinstance(v, dict) else self.value(v, facts) if isinstance(v, str) else v
        for k, v in o.get('let', {}).items():
            facts[k] = lv(v)
        if stem.startswith('@'):
            stem = self.value(stem, facts)
        if op == 'foreach':
            self.foreach(o, body, depth, extra, facts)
            return
        assert not body, 'body under ' + op
        g = self.E.g
        sec = self.value(section, facts) if section.startswith('@') else _section(section, self.flags)
        kw = {}
        if 'classes' in o:
            kw['classes'] = _path(facts, o['classes'])
        if o.get('domain') == '@labels':
            kw['domain'] = sorted(g.labels) + ['BOT']
        elif 'domain' in o:
            kw['domain'] = range(*o['domain'])
        if 'domain_keys' in o:
            kw['domain'] = _path(facts, o['domain_keys'])
        if 'domain_at' in o:
            kw['domain'] = [_path(facts, o['domain_at'])]
        if 'tokens' in o:
            kw['classes'] = dict(kw.get('classes') or {}, **self.tokens(o['tokens']))
        for k, v in o.get('classmap', {}).items():
            kw['classes'] = dict(kw.get('classes') or {})
            x = self.value(v, facts)
            kw['classes'][k] = x if isinstance(x, list) else [x]
        facts.update(o.get('with', {}))
        res = None
        if op in ('holder', 'fresh') and 'cols' in o:
            return self.fresh_table(stem, o)
        bd = self.bindings(o, bind, facts)
        ex = o.get('export', [])
        for k, b in ex.items() if isinstance(ex, dict) else ((k, k) for k in ex):
            self.env[k] = bd[b]
        sq = self.cells(seq, facts)
        tr = self.seqrows(o, facts)
        if tr is not None:
            sq = dict(tr, **sq or {})
        if isinstance(o.get('seqfact'), str):
            sq = dict({k: [tuple(a) for a in v] for k, v in _path(facts, o['seqfact']).items()}, **sq or {})
        if 'seqlist' in o:
            t = lambda x: tuple(map(t, x)) if isinstance(x, list) else x
            base = {}
            for sf in o['seqlist']:
                base.update({k: [t(a) for a in v] for k, v in _path(facts, sf).items()})
            sq = dict(base, **sq or {})
        if o.get('outseq'):
            sq = dict(sq or {}, **{a[1]: [('OUT', c) for c in a[1][2:].encode()] for f in ('-result.tsv', '-byte.tsv') if (self.root / (stem + f)).exists() for ln in (self.root / (stem + f)).read_text().splitlines() if not ln.startswith('#') and ln.count('\t') >= 4 for a in json.loads(ln.split('\t')[4]) if a[:1] == ['@'] and a[1].startswith('O:')})
        if 'mapseq' in o:
            sq = dict(sq or {}, **self.mapseq(o['mapseq'], facts, bd))
        if 'seqenv' in o:
            sq = dict({k: self.env[k] for k in _path(facts, o['seqenv'])}, **sq or {})
        if op == 'template':
            res = install_template(g, self.root, stem, facts, self.fresh(fresh), bindings=bd, sequences=sq, section=sec, mode=o.get('mode', 'r'), overlay=o.get('overlay', False), **kw)
        elif op == 'rows':
            res = install(g, self.root, stem, bindings=bd, sequences=sq, section=sec, **kw)
        elif op == 'table':
            res = install_rows(g, self.root / stem, sq, bindings=bd, section=sec, mode=o.get('mode', 'r'), skip=o.get('skip', ()), ordered=o.get('ordered', False), **kw)
        elif op == 'call':
            sub = Run(self.E, self.P, dict(self.flags, **{k: any((self.flags[f] for f in v)) if isinstance(v, list) else v for k, v in o.get('flags', {}).items()}), dict(self.env, **bd or {}))
            res = sub.run(self.root / (stem + '-manifest.tsv'))
            if o.get('merge'):
                self.env.update(res)
        elif op == 'let':
            if 'exit' in o:
                raise SystemExit(o['exit'])
            self.env.update(bd or {})
            self.env.update(sq or {})
        elif op == 'label':
            g.labels.update(stem.split(','))
        elif op == 'assert-absent':
            assert (stem in g.st) == bool(o.get('present')), stem
        elif op == 'holder':
            k, _, name = fresh.partition(':')
            self.holders[stem] = self.P(name) if k == 'P' else self.holder(name or stem)
            if 'cur' in o:
                self.holders[stem].cur = o['cur']
        else:
            raise ValueError('manifest op ' + op)
        if 'result' in o:
            self.env[o['result']] = res

def _interp_impl(self, v):
    return re.sub('\\{(\\w+)\\}', lambda m: str(self.env[m.group(1)]) if m.group(1) in self.env else m.group(0), v)

def _fresh_table(self, stem, o):
    ki, kd, ko = o['cols']
    w = o.get('where', [])
    for ln in (self.root / stem).read_text().split('\n'):
        f = ln.split('\t')
        if not ln or ln.startswith('#') or any((f[c] != v for c, v in w)):
            continue
        owner = _fmt(o.get('owner', '{owner}'), {'owner': f[ko], 'key': f[ki]})
        if o.get('scope', 'U') == 'P':
            self.env[f[ki]] = self.P(owner).fresh(f[kd])
        else:
            self.env[f[ki]] = self.E.P.fresh(self.holder(owner), f[kd])

def _mapseq(self, spec, facts, bd):
    def cell(c, x):
        m = re.fullmatch('\\{(\\w+)\\}', c) if isinstance(c, str) else None
        if m:
            return x[m.group(1)]
        if isinstance(c, str) and c.startswith('$'):
            n = c[1:]
            return (bd or {})[n] if n in (bd or {}) else self.env[n] if n in self.env else facts[n]
        return _fmt(c, x) if isinstance(c, str) else c
    def emit(acts, a, x):
        if isinstance(a, dict):
            for y in x[a['over']]:
                for b in a['acts']:
                    emit(acts, b, dict(x, **{a.get('as', 'it'): y}))
        elif isinstance(a, str) and a.startswith('$'):
            acts.extend(self.env[a[1:]])
        elif a[0] == '@out':
            acts.extend((('OUT', c) for c in _fmt(a[1], x).encode()))
        elif a[0] == '@bytes':
            acts.extend((('SBOUT', c) for c in _fmt(a[1], x).encode()))
        else:
            acts.append(tuple((cell(c, x) for c in a)))
    out = {}
    for name, parts in spec.items():
        if '{' in name:
            for row in _path(facts, parts['over']):
                acts = []
                ctx = dict(facts, **row)
                for part in parts['parts']:
                    if 'splice' in part:
                        for key in row[part['splice']]:
                            acts.extend(out[key] if key in out else self.env[key])
                    elif all(row.get(k) == v for k, v in part.get('where', {}).items()):
                        for action in part['acts']:
                            emit(acts, action, ctx)
                out[_fmt(name, row)] = acts
            continue
        acts = []
        for part in parts:
            for row in _path(facts, part['over']) if 'over' in part else [facts]:
                if any(str(row[k]) != _fmt(v, facts) for k, v in part.get('where', {}).items()):
                    continue
                ctx = dict(facts, **row) if isinstance(row, dict) and row is not facts else facts
                for action in part['acts']:
                    emit(acts, action, ctx)
        out[name] = acts
    return out
Run.interp, Run.fresh_table, Run.mapseq = (_interp_impl, _fresh_table, _mapseq)
def run(manifest, E, P, flags, env=None):
    """Assemble MANIFEST onto E.g; returns the (updated) environment."""
    return Run(E, P, dict(flags), dict(env or {})).run(manifest)
def _when(w, flags, facts):
    if w in ('', '-'): return True
    for term in w.split('&'):
        neg = term.startswith('!')
        key = term[neg:]
        value = facts.get(key[5:]) if key.startswith('fact:') else flags[key]
        if bool(value) == neg: return False
    return True

def _section(s, flags):
    if s in ('', '-'):
        return None
    return re.sub('\\{(!?)(\\w+)\\?([^}]*)\\}', lambda m: m.group(3) if bool(flags[m.group(2)]) != bool(m.group(1)) else '', s)

def _out(text, facts):
    if text.startswith('='):
        return [('OUT', c) for c in str(_path(facts, text[1:])).encode()]
    t = re.sub('\\{(\\w+)\\}', lambda m: str(facts[m.group(1)]), text)
    t = re.sub('\\\\x([0-9a-f]{2})', lambda m: chr(int(m.group(1), 16)), t)
    t = _cell('str', t)
    return [('OUT', c) for c in t.encode()]

_FMT = re.compile('\\{\\{|\\}\\}|\\{([A-Za-z_]\\w*)((?:\\[[^\\[\\]{}]+\\])*)\\}|[{}]')

def _fmt_field(m, facts):
    if m.group(0) in ('{{', '}}'): return m.group(0)[0]
    if m.group(1) is None: raise ValueError('@fmt: stray brace')
    value = facts[m.group(1)]
    for key in re.findall(r'\[([^\]]+)\]', m.group(2)):
        value = value[int(key)] if key.isdigit() else value[key]
    return str(value)
def _fmt(f, facts):
    return _FMT.sub(lambda m: _fmt_field(m, facts), f)
def _foreach(self, o, body, depth, extra, facts):
    if o['over'].startswith('$'):
        rows = [{'key': k, 'value': v} for k, v in self.env[o['over'][1:]].items()]
        j = o.get('join')
        if j:
            table = _path(facts, j['over'])

            def match(r):
                hit = [t for t in table if t[j['on']] == r['key']]
                if hit:
                    return dict(hit[0], **r)
                return dict({c: _fmt(v, r) if isinstance(v, str) else v for c, v in j['default'].items()}, **r)
            rows = [match(r) for r in rows]
    else:
        rows = _path(facts, _fmt(o['over'], facts) if '{' in o['over'] else o['over'])
    for col, allowed in o.get('where', {}).items():
        allowed = [_fmt(_section(a, self.flags), facts) for a in allowed]
        rows = [r for r in rows if r[col] in allowed]
    ch = o.get('chain')
    cur = ch and self.value(ch['start'], facts)
    for x in rows:
        ex = dict(extra, **{o.get('as', 'it'): x})
        for k, v in o.get('pre', []):
            ex[k] = self.value(v, dict(facts, **ex))
        if ch:
            nxt = self.value(ch['next'], dict(facts, **ex))
            ex.update({ch['entry']: cur, 'next': nxt})
            cur = nxt
        self.block(body, depth + 1, ex)
    if ch:
        self.env[ch['result']] = cur

def _header_rows(path):
    lines = path.read_text().splitlines()
    cols = lines[0].lstrip('# ').split('\t')
    return [dict(zip(cols, line.split('\t'))) for line in lines[1:] if line and not line.startswith('#')]

def _bindings(self, o, bind, facts):
    acc = o.get('accumulate')
    out = dict(self.accum.setdefault(acc, {})) if acc else {}
    for bm in [o['bindmap']] if isinstance(o.get('bindmap'), str) else o['bindmap'] if isinstance(o.get('bindmap'), list) else []:
        out.update(_path(facts, self.interp(bm)))
    if isinstance(o.get('freshrows'), str):
        file, _, part = o['freshrows'].partition('@')
        for row in _header_rows(self.root / file):
            if part and row.get('part', row.get('section')) != part: continue
            key = row.get('key', row.get('name'))
            out[key] = self.E.P.fresh(self.holder(row.get('owner', row.get('prefix'))), row['kind'])
    if o.get('cellsfirst'):
        out.update(self.cells(bind, facts) or {})
        bind = '-'
    for spec in [] if isinstance(o.get('freshrows'), str) else o.get('freshrows', []):
        src = _header_rows(self.root / spec['file']) if 'file' in spec else _path(facts, spec['over'])
        for i, r in enumerate(src):
            ctx = dict(facts, i=i, **r)
            wctx = dict(facts, i=i) if 'file' in spec else ctx
            if all((str(r[c]) == _fmt(v, wctx) for c, v in spec.get('where', {}).items())):
                owner = _fmt(spec['owner'], ctx)
                if spec.get('lookup'):
                    owner = out[owner[1:]] if owner.startswith('$') else out.get(owner, owner)
                kind = _fmt(spec['kind'], ctx)
                out[_fmt(spec['key'], ctx)] = self.E.P.fresh(self.holder(owner), kind) if spec.get('holder') else self.E.P(owner).fresh(kind)
    cells = self.cells(bind, facts)
    if cells is None and (not out) and (not acc) and ('bindmap' not in o):
        return None
    out.update(cells or {})
    if isinstance(o.get('bindmap'), dict):
        out.update({k: self.value(v, facts) for k, v in o['bindmap'].items()})
    if acc:
        self.accum[acc] = out
        if 'keep' in o:
            self.env[o['keep']] = dict(out)
    return out
Run.foreach = _foreach
Run.bindings = _bindings
def _seqmap(self, lst, tmpl, facts):
    out = []
    for x in _path(facts, _fmt(lst, facts)):
        for a in _path(facts, tmpl):
            if isinstance(a, str):
                out += self.env[a[1:]]
            else:
                out.append(tuple((x if c == '{x}' else c for c in a)))
    return out
Run.seqmap = _seqmap
def _seqrows(self, o, facts):
    if not any(k in o for k in ('textrows', 'bufrows', 'msgrows')): return None
    def rows(file):
        return [line.split('\t') for line in (self.root / file).read_text().splitlines()[1:]]
    sq = {}
    for name, value in rows(o['textrows']) if 'textrows' in o else []:
        sq[name] = self.E.O(json.loads(value))
    for name, value in rows(o['bufrows']) if 'bufrows' in o else []:
        sq[name] = [('SBOUT', c) for c in json.loads(value).encode()]
    if 'msgrows' in o:
        file, fact, prefix = o['msgrows']
        pairs = rows(file)
        facts[fact] = [name for name, _ in pairs]
        sq.update({prefix + name: [('SBOUT', c) for c in json.loads(value).encode()]
                   for name, value in pairs})
    return sq
Run.seqrows = _seqrows

def _tokens(self, spec):
    if isinstance(spec, str):
        spec = dict((ln.split('\t') for ln in (self.root / spec).read_text().splitlines()[1:]))
    lit = {'number': self.E.TK_NUM, 'string': self.E.TK_STR, 'identifier': self.E.TK_ID}
    return {k: [lit[t] if t in lit else self.E.TK[t]] for k, t in spec.items()}
Run.tokens = _tokens

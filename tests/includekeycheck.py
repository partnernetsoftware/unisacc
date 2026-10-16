#!/usr/bin/env python3
"""includekeycheck (S2 prerequisite): the include keys the pp reference (exec/pp/sim.py) asks for equal the keys the
product actually requests, byte for byte and in order, with the same presence.

The product's requests are read from the RF capture ledger (UNISACC_RF_CAPTURE, exec/c/run.c; S1).  The sim is given
exactly the product's explicit \\0cli/ inputs (the same bytes, from the same ledger), and nothing else: the include
keys themselves are NOT injected, so the sim computes them from the disk, as the reference does.  Presence is compared
too: PRESENT vs ABSENT (a missing file is ABSENT, never a pass).

usage: tests/includekeycheck.py PRODUCT DELTA.json [WORKDIR]
  PRODUCT    the compiler to measure (MODEL_COM); DELTA.json is the pp delta from exec/pp/gen-delta.sh.
  Run from the repository root.  Exit 0 only when every case is equal; 1 on any difference; 2 on bad arguments.
"""
import json, os, pathlib, shutil, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'exec' / 'pp'))
import sim  # noqa: E402

# (name, files, product flags, source) -- the five forms of the S2 baseline plus one nested relative directory
CASES = [
    ('quote', {'m.c': '#include "sub/h1.h"\nint main(void){return h1;}\n',
               'sub/h1.h': '#include "h1b.h"\nint h1=0;\n', 'sub/h1b.h': 'int h1b=0;\n'}, [], 'm.c'),
    ('nested-rel', {'m.c': '#include "sub/a.h"\nint main(void){return b;}\n',
                    'sub/a.h': '#include "b.h"\n', 'sub/b.h': 'int b=0;\n'}, [], 'm.c'),
    ('angle-I', {'m.c': '#include <h2.h>\nint main(void){return h2;}\n', 'inc/h2.h': 'int h2=0;\n'},
     ['-I', 'inc'], 'm.c'),
    ('inc-I', {'m.c': '#include <h3.h>\nint main(void){return h3;}\n', 'inc/h3.h': 'int h3=0;\n'},
     ['-I', 'inc'], 'm.c'),
    ('forced', {'m.c': 'int main(void){return forced_v;}\n', 'f.h': 'int forced_v=0;\n'},
     ['-include', 'f.h'], 'm.c'),
    ('missing', {'m.c': '#include "nope.h"\nint main(void){return 0;}\n'}, [], 'm.c'),
]


def make_case(root, name, files):
    d = root / name
    for rel, text in files.items():
        p = d / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)
    return d


def product_requests(product, d, flags, capdir):
    """The product's requests for this case: (cli inputs {key: bytes|None}, [(include key, present)])."""
    shutil.rmtree(capdir, ignore_errors=True)
    capdir.mkdir(parents=True)
    env = dict(os.environ, UNISACC_RF_CAPTURE=str(capdir))
    result = subprocess.run(['sh', product] + flags + ['-S', 'm.c', '-o', str(capdir / 'out.tape')], cwd=d, env=env,
                            capture_output=True, timeout=50)
    expected_rc = 1 if d.name == 'missing' else 0
    if result.returncode != expected_rc:
        raise ValueError('include-key-product-rc expected=%d actual=%d: %s' %
                         (expected_rc, result.returncode, result.stderr.decode(errors='replace')))
    lines = (capdir / 'resources.tsv').read_text().splitlines()
    if lines and lines[0].startswith('#SC1'):
        fields = lines[0].split('\t')
        if (len(fields) != 3 or fields[0] != '#SC1' or not fields[1].startswith('stage=') or
            not 1 <= len(fields[1][6:]) <= 127 or any(ord(c) < 32 or ord(c) == 127 for c in fields[1][6:]) or
            not fields[2].startswith('input_sha256=') or len(fields[2][13:]) != 64 or
            any(c not in '0123456789abcdef' for c in fields[2][13:])):
            raise ValueError('include-key-resource-identity-malformed')
        lines = lines[1:]
    cli, inc = {}, []
    for line in lines:
        if line.startswith('#SC1'):
            raise ValueError('include-key-resource-identity-misplaced')
        keyhex, present, fname, _len, _sha = line.split('\t')
        key = bytes.fromhex(keyhex)
        value = (capdir / fname).read_bytes() if present == '1' else None
        if key.startswith(b'\0cli/'):
            cli[key] = value
        elif key.endswith(b'.h'):
            inc.append((key, present == '1'))
    return cli, inc


def sim_requests(delta, loaded, d, cli):
    """The sim's requests for the same explicit inputs: cli bytes injected, include keys computed from the disk."""
    old = os.getcwd()
    os.chdir(d)
    try:
        fs = sim.Files()
        for key, value in cli.items():
            fs.cache[key] = value
        res, _val, _steps = sim.run(delta, (d / 'm.c').read_bytes(), 'm.c', fs, set(), maxsteps=20000000,
                                    loaded=loaded)
    finally:
        os.chdir(old)
    if res not in ('accept', 'reject'):
        return res, None
    inc = [(k, v is not None) for k, v in fs.cache.items() if k not in cli and k.endswith(b'.h')]
    return res, inc


def main(argv):
    if len(argv) < 3:
        print('usage: includekeycheck.py PRODUCT DELTA.json [WORKDIR]', file=sys.stderr)
        return 2
    product, delta_path = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    work = pathlib.Path(argv[3]) if len(argv) > 3 else pathlib.Path(tempfile.mkdtemp(prefix='includekey-'))
    delta = json.load(open(delta_path))
    loaded = sim.load(delta)
    bad = 0
    for name, files, flags, _src in CASES:
        d = make_case(work, name, files)
        cli, prod = product_requests(product, d, flags, work / 'cap' / name)
        res, simk = sim_requests(delta, loaded, d, cli)
        if simk is None:
            print('includekeycheck %-10s FAIL sim did not finish: %s' % (name, res)); bad += 1; continue
        if prod == simk:
            print('includekeycheck %-10s PASS %d include keys equal (%s)' %
                  (name, len(prod), ', '.join(k.decode('latin-1').replace('\0', '\\0') for k, _ in prod) or 'none'))
            continue
        bad += 1
        n = next((i for i, (a, b) in enumerate(zip(prod, simk)) if a != b), min(len(prod), len(simk)))
        print('includekeycheck %-10s FAIL first difference at %d: product %r sim %r' %
              (name, n, prod[n:n + 1], simk[n:n + 1]))
    print('includekeycheck %s' % ('FAIL: %d case(s) differ' % bad if bad else 'PASS: all %d cases equal' % len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

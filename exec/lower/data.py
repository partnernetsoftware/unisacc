"""Tape data directives and zero-last layout as ordinary delta actions.
Input is the actual textual tape; non-data lines are retained verbatim for the
next lowering pass. No Python tape parser or zero_last runs in this path.
"""
# Separate virtual addresses from compact byte indices. A large .bss never
# allocates byte cells; wide namespaces prevent virtual offsets aliasing tables.
RAW, START, NAME, FLAG, NEW, REMAP, DEFINED, RSTART = (i << 40 for i in range(1,9))


def install(E, done='ACCEPTDATA', code_start='H.code', target='lnx/x86_64'):
    if target not in ('lnx/x86_64', 'lnx/arm64', 'osx/x86_64', 'osx/arm64', 'win/x86_64', 'win/arm64'):
        raise ValueError('unsupported lowering target: '+target)
    from unisa.lower import SCRATCH, PRINTMAX, WIN_EXTRA, WIN_STACK
    win=target.startswith("win/")
    extra=WIN_EXTRA if win else SCRATCH+PRINTMAX
    bss=WIN_STACK if win else 0
    from pathlib import Path
    from finite_rules import install as install_rules
    from unisa.tape import ESCAPES
    E.prn()
    labels = (('WORD', 'b'), ('D', 'b'), ('D', 'b'), ('D', 'b'),
              ('D', 'b'), ('BSS', 'b'), ('BSS', 'b'), ('STR', 'b'),
              ('BOUNDS', 'b'), ('L', 'b'), ('L', 'r'), ('L', 'b'),
              ('R', 'b'), ('R', 'r'), ('R', 'b'), ('R', 'b'),
              ('R', 'b'), ('R', 'b'), ('R', 'b'), ('R', 'b'),
              ('R', 'b'), ('R', 'b'), ('HEAD', 'b'), ('H', 'b'),
              ('H', 'r'), ('H', 'r'), ('HEX', 'b'), ('H', 'b'),
              ('H', 'r'), ('H', 'r'))
    bindings = {'label'+str(i): E.P(owner).fresh(kind)
                for i, (owner, kind) in enumerate(labels)}
    bindings.update({name: globals()[name] for name in
                     ('RAW', 'START', 'NAME', 'FLAG', 'NEW', 'REMAP', 'DEFINED', 'RSTART')})
    bindings.update(done=done, code_start=code_start, extra=extra,
                    data_limit=2147483647-extra-bss-7)
    bindings.update({'escape'+str(i): ord(value) for i, value in enumerate(ESCAPES.values())})
    classes = {'escape'+str(i): [ord(key)] for i, key in enumerate(ESCAPES)}
    sequences = {
        'text0': E.O('@target '+target+'\n@data '),
        'text1': E.O('-'),
        'text2': E.O('\n'),
        'text3': E.O('@sym '),
        'text4': E.O(' '),
        'text5': E.O('\n'),
        'text6': E.O('@src_os '+target.split('/')[0]+'\n@data_len '),
        'text7': E.O('\n@bss '+str(bss)+'\n@relocs -\n'),
        'reject': E.rej('not covered: tape data directive'),
    }
    install_rules(E.g, Path(__file__).parent, 'data', bindings=bindings,
                  sequences=sequences, classes=classes)

"""Signed decimal printer encoding. Runtime algorithm stays in emitted ARM code.
ADRP targets are resolved by the delta, using the final section layout. No
encoder oracle or numeric formatting primitive is called by the executor.
Scratch x9-x17 are outside the allocated tape value registers.
"""
PREFIX = [
        0xF9400000|(9<<5)|10,                 # load value
        0xD2800000|11,                       # negative flag = 0
        0xF100001F|(10<<5),                  # cmp x10,0
        0x54000000|(3<<5)|10,                # b.ge over negate/flag
        0xCB000000|(10<<16)|(31<<5)|10,
        0xD2800000|(1<<5)|11,
        0xD2800000|(10<<5)|12,               # divisor 10
        0xAA0003E0|(10<<16)|13,
        0xD2800000|14,
        0x9AC00800|(12<<16)|(13<<5)|13,      # count digits, unsigned
        0x91000400|(14<<5)|14,
        0xB5000000|(((-2)&0x7ffff)<<5)|13,
        0x8B000000|(11<<16)|(14<<5)|14,
    ]
COUNT_STORE = [0xF9000000|(9<<5)|14]
SUFFIX = [
        0x8B000000|(14<<16)|(9<<5)|13,        # end of digit area
        0x9AC00800|(12<<16)|(10<<5)|15,
        0x9B008000|(12<<16)|(10<<10)|(15<<5)|17,
        0x91000000|(48<<10)|(17<<5)|17,
        0xD1000400|(13<<5)|13,
        0x39000000|(13<<5)|17,
        0xAA0003E0|(15<<16)|10,
        0xB5000000|(((-6)&0x7ffff)<<5)|10,
        0xB4000000|(3<<5)|11,
        0xD2800000|(45<<5)|17,
        0x39000000|(9<<5)|17,
    ]

def install(E,word):
    from pathlib import Path
    from functools import partial
    from finite_rules import install as install_rules
    rules = partial(install_rules, E.g, Path(__file__).parent, 'armitoa')
    entry = 'EMIT.37'
    for arg, values in (('a0',()), ('a2',PREFIX), ('a1',COUNT_STORE)):
        prefix = E.P('itoa.template')
        for value in values: word(prefix.a(('LDI','w',value)))
        first, nxt = E.P('EMIT').fresh('r'), E.P('EMIT').fresh('r')
        rules(section='address', bindings=dict(entry=entry, first=first, next=nxt, arg=arg),
              sequences={'prefix': prefix.acts})
        entry = nxt
    suffix = E.P('itoa.template')
    for value in SUFFIX: word(suffix.a(('LDI','w',value)))
    rules(section='finish', bindings={'entry': entry}, sequences={'suffix': suffix.acts})

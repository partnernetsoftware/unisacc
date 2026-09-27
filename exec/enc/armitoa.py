"""Signed decimal printer encoding. Runtime algorithm stays in emitted ARM code.
ADRP targets are resolved by the delta, using the final section layout. No
encoder oracle or numeric formatting primitive is called by the executor.
Scratch x9-x17 are outside the allocated tape value registers.
"""
def install(E,word):
    p=E.P('EMIT.37')
    def words(values):
        for value in values: word(p.a(('LDI','w',value)))
    def address(arg):
        p.a(('LDI','ad_r',9),('COPYW','ad_v',arg)).call('AD.data').call('ADRP')
    address('a0')
    words([
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
    ])
    address('a2')
    words([0xF9000000|(9<<5)|14])
    address('a1')
    words([
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
    ])
    p.goto('LINE')

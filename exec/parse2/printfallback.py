"""Tape templates for the product's undeclared-printf fallback.
The pfconv truth table selects a routine. These templates preserve the
reference fallback's ignored widths/precision and zero return, not libc's
full printf contract. Declared printf still uses the ordinary call path.
"""
KINDS=['int','u32','hex','HEX','oct','chr','str']
SLEN='''__slen:
  mov r2, r0
  imm r1, 0
__slen_top:
  add64 r4, r2, r1
  .ld r5, [r4+0], 1
  jumpz r5, __slen_end
  imm r5, 1
  add64 r1, r1, r5
  jump __slen_top
__slen_end:
  mov r0, r1
  ret
'''
ITOAB='''.bss __xbuf 24
__itoab:
  .frame 16
  store64 [r7+0], r1
  store64 [r7+8], r2
  mov r2, r0
  .lea r1, __xbuf
  imm r3, 24
  add64 r1, r1, r3
  imm r4, 0
itoab_loop:
  load64 r3, [r7+0]
  .umod r5, r2, r3
  .udiv r2, r2, r3
  imm r3, 10
  slt64 r0, r5, r3
  jumpz r0, itoab_alpha
  imm r3, 48
  jump itoab_add
itoab_alpha:
  load64 r3, [r7+8]
  imm r0, 10
  sub64 r5, r5, r0
itoab_add:
  add64 r5, r5, r3
  imm r3, 1
  sub64 r1, r1, r3
  .st [r1+0], r5, 1
  add64 r4, r4, r3
  jumpz r2, itoab_done
  jump itoab_loop
itoab_done:
  .frame -16
  mov r0, r1
  mov r1, r4
  ret
'''
def install(E, P):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root = Path(__file__).parent
    def rows(suffix):
        return (line.split('\t') for line in (root/('printfallback-'+suffix+'.tsv')).read_text().splitlines()
                if not line.startswith('#'))
    texts = {'SLEN':SLEN, 'ITOAB':ITOAB}
    sequences = {name:E.O(texts[value] if kind=='binding' else json.loads(value))
                 for name,kind,value in rows('text')}
    templates = {kind:json.loads(value) for kind,value in rows('kinds')}
    bindings = {}
    def rules(section):
        for selected,name,prefix,kind in rows('names'):
            if selected == section:
                bindings[name] = P(prefix+'.pf_'+name).fresh(kind)
        install_rules(E.g, root, 'printfallback', bindings=bindings,
                      sequences=sequences, section=section)
    rules('head')
    for index,kind in enumerate(KINDS):
        bindings['convert'] = 'PF.convert.'+kind
        E.g.on(bindings['PF_b1'], [index], bindings['convert'], [], 'r')
        sequences['body'] = [op for action in templates.get(kind, [])
                             for op in (E.O(action[1]) if action[0]=='text' else [tuple(action)])]
        rules('body')
    rules('tail')

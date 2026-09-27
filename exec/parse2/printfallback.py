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
def install(P):
    P('PF.convert').branch({i:'PF.convert.'+k for i,k in enumerate(KINDS)},'DEAD', [('RLD','pfkind')])
    for k in KINDS:
        p=P('PF.convert.'+k)
        if k=='int':p.o('  .print r0\n')
        elif k=='u32':p.o('  imm r2, 4294967295\n  and64 r0, r0, r2\n  .print r0\n')
        elif k in ('hex','HEX','oct'):
            p.a(('LDI','pf_xb',1)).o('  imm r1, '+str(8 if k=='oct' else 16)+'\n  imm r2, '+str(65 if k=='HEX' else 97)+'\n  call __itoab\n  .write r0, r1\n')
        elif k=='chr':p.a(('LDI','pf_ch',1)).o('  mov r2, r0\n  .lea r0, __chb\n  .st [r0+0], r2, 1\n  imm r1, 1\n  .write r0, r1\n')
        elif k=='str':p.a(('LDI','pf_sl',1)).o('  .frame 8\n  store64 [r7+0], r0\n  call __slen\n  mov r1, r0\n  load64 r0, [r7+0]\n  .frame -8\n  .write r0, r1\n')
        p.ret()
    P('PF.helpers').branch({1:'PF.helpers.s'},'PF.helpers.c0',[('CMPI','pf_sl',1)])
    P('PF.helpers.s').o(SLEN).goto('PF.helpers.c0')
    P('PF.helpers.c0').branch({1:'PF.helpers.c'},'PF.helpers.x0',[('CMPI','pf_ch',1)])
    P('PF.helpers.c').o('.bss __chb 8\n').goto('PF.helpers.x0')
    P('PF.helpers.x0').branch({1:'PF.helpers.x'},'RET',[('CMPI','pf_xb',1)])
    P('PF.helpers.x').o(ITOAB).ret()

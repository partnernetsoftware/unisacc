"""_Bool conversion is comparison with zero, never integer truncation.
Keep a distinct descriptor, using the existing u8 axis for arithmetic.
The procedures emit ordinary tape operations; no executor action is added.
"""
def install(P, BOOL, DBL, FLT):
    P('TO.b').branch({1:'TB.scalar'},'TB.integer',[('CMPI','vt',0)])
    P('TB.scalar').branch({1:'TB.double'},'TB.single',[('CMPI','vb',DBL)])
    P('TB.single').branch({1:'TB.float'},'TB.integer',[('CMPI','vb',FLT)])
    for name,width in [('double',64),('float',32)]:
        P('TB.'+name).o('  imm r1, 0\n  feq%d r0, r0, r1\n  imm r1, 1\n  xor64 r0, r0, r1\n'%width).ret()
    P('TB.integer').o('  imm r2, 0\n  ne r0, r0, r2\n').ret()
    P('BOOLCV').vpush('vt','vb').a(('COPYW','vt','rvt'),('COPYW','vb','rvb')).call('TO.b').vpop('vt','vb').ret()
    P('BOOLTARGET').branch({1:'BT.scalar'},'RET',[('CMPI','vt',0)])
    P('BT.scalar').branch({1:'BOOLCV'},'RET',[('CMPI','vb',BOOL)])

"""Scalar condition conversion, matching front_parse.c ftruthy/unary !.
Floating +0/-0 are false; NaN is true. Pointer/integer values retain the
existing tests. Type descriptors choose f32/f64; no host primitive added.
"""
def install(P,DBL,FLT):
    P('FTRUTH').branch({1:'FT.kind'},'RET',[('CMPI','vt',0)])
    P('FT.kind').branch({1:'FT.double'},'FT.single',[('CMPI','vb',DBL)])
    P('FT.single').branch({1:'FT.float'},'RET',[('CMPI','vb',FLT)])
    for name,width in [('double',64),('float',32)]:
        P('FT.'+name).o('  imm r1, 0\n  feq%d r0, r0, r1\n  imm r1, 1\n  xor64 r0, r0, r1\n'%width).a(('LDI','vb',4)).ret()
    P('FNOT').branch({1:'FN.kind'},'FN.integer',[('CMPI','vt',0)])
    P('FN.kind').branch({1:'FN.double'},'FN.single',[('CMPI','vb',DBL)])
    P('FN.single').branch({1:'FN.float'},'FN.integer',[('CMPI','vb',FLT)])
    for name,op in [('double','feq64'),('float','feq32'),('integer','eq')]:
        P('FN.'+name).o('  imm r1, 0\n  %s r0, r0, r1\n'%op).a(('LDI','vt',0),('LDI','vb',4)).ret()

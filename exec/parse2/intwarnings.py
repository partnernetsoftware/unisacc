"""Reference intptr_check for local scalar initializers and assignments.
This preserves the reference's syntactic lone-zero exemption and call flag;
it is not a general C constant-expression or conversion checker.
"""
from tokenlocations import TOKEN_POS
MESSAGE=b'incompatible integer to pointer conversion [-Wint-conversion]'

def install(E,P,DBL,FLT,FPB,SBB):
    P('WI.expr').a(('COPYW','wi_start','tpos'),('LDI','wi_called',0)).vpush('wi_start','wi_target').call('EXPR').vpop('wi_start','wi_target').call('WI.check').ret()
    P('WI.check').branch({1:'RET'},'WI.call',[('CMPI','wi_target',0)])
    P('WI.call').branch({1:'RET'},'WI.ptr',[('CMPI','wi_called',1)])
    P('WI.ptr').branch({1:'WI.kind'},'RET',[('CMPI','vt',0)])
    p=P('WI.kind')
    for i,base in enumerate((DBL,FLT,FPB)):
        nxt='WI.kind'+str(i)
        p.branch({1:'RET'},nxt,[('CMPI','vb',base)]);p=P(nxt)
    p.branch({(1,2):'RET'},'WI.zero',[('CMPI','vb',SBB)])
    # Re-read only the first token, preserving parser cursor/descriptors. The
    # byte offset after one NEXT must be the current token's start, so `(0)`
    # or `0+0` is not mistaken for the reference's lone numeric zero.
    P('WI.zero').a(('COPYW','wi_end','tpos'),('JUMP','wi_start')).call('NEXT').tok({E.TK_NUM:'WI.number'},'WI.warnback')
    P('WI.number').branch({1:'WI.next'},'WI.warnback',[('LDI','wi_zero',0),('C64','nv','wi_zero')])
    P('WI.next').call('NEXT').branch({1:'RET'},'WI.warnback',[('CMP','tpos','wi_end')])
    P('WI.warnback').a(('JUMP','wi_end')).call('NEXT').a(
        ('LDX','diag_pos','wi_start',TOKEN_POS),('LDI','diag_warning',1),
        ('SBCLR',),*[('SBOUT',c) for c in MESSAGE],('SBSAVE','diag_message')).call('DIAG.report').a(
        ('ALU','add','wr_count','wr_count','diag_reported')).ret()

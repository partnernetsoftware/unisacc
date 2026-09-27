"""The reference's laststmt/return warning, not a new flow analysis.
Enabled only by the development --warnings mode. Other warning kinds are
still pending, so this mode is not exposed as the compiler's -Wall route.
"""
from tokenlocations import TOKEN_POS
MESSAGE=b'non-void function does not return a value in all control paths [-Wreturn-type]'


def install(E,P,SBB):
    g=E.g
    P('STMT').a(('COPYW','wr_back','tpos'),('LDI','wr_inf',0)).call('WR.peek').a(
        ('JUMP','wr_back')).call('NEXT').a(('LDI','wr_last',0)).vpush('wr_inf').call('STMT.body').vpop('wr_inf').branch(
        {1:'WR.forever'},'RET',[('CMPI','wr_inf',1)])
    P('WR.forever').a(('LDI','wr_last',1)).ret()
    P('WR.peek').tok({'return':'WR.yes','while':'WR.while','for':'WR.for',E.TK_ID:'WR.name'},'RET')
    P('WR.yes').a(('LDI','wr_inf',1)).ret()
    p=P('WR.name').a(('INTERN','wr_id','ps','pe'))
    for i,nm in enumerate(('exit','abort','__exit','_exit')):
        nxt='WR.name'+str(i)
        p.a(('SBCLR',),*[('SBOUT',c) for c in nm.encode()],('SBINTERN','wr_test')).branch(
            {1:'WR.yes'},nxt,[('CMP','wr_id','wr_test')])
        p=P(nxt)
    p.ret()
    P('WR.while').call('NEXT').call('NEXT').tok({E.TK_NUM:'WR.number',E.TK_FNUM:'WR.float'},'RET')
    P('WR.number').a(('COPYW','wr_value','nv')).goto('WR.nonzero')
    # numval() on a floating token reads the integer prefix, not FP bits.
    P('WR.float').a(('INPUSHXE','ps','pe'),('LDI','wr_value',0),('LDI','wr_base',10)).goto('WR.floatfirst')
    g.on('WR.floatfirst',[48],'WR.floatdigits',[('LDI','wr_base',8),('ADV',)])
    g.els('WR.floatfirst','WR.floatdigits')
    for d in range(10):g.on('WR.floatdigits',[48+d],'WR.floatdigit',[('LDI','wr_digit',d)])
    g.els('WR.floatdigits','WR.floatend')
    P('WR.floatdigit').branch({0:'WR.floatadd'},'WR.floatend',[('CMP','wr_digit','wr_base')])
    P('WR.floatadd').a(('A64','mul','wr_value','wr_value','wr_base'),
        ('A64','add','wr_value','wr_value','wr_digit'),('ADV',)).goto('WR.floatdigits')
    P('WR.floatend').a(('INPOP',)).goto('WR.nonzero')
    P('WR.nonzero').branch({1:'RET'},'WR.close',[('LDI','wr_zero',0),('C64','wr_value','wr_zero')])
    P('WR.close').call('NEXT').tok({')':'WR.yes'},'RET')
    P('WR.for').call('NEXT').call('NEXT').tok({';':'WR.forsemi'},'RET')
    P('WR.forsemi').call('NEXT').tok({';':'WR.yes'},'RET')
    P('WR.return').branch({1:'WR.type'},'RET',[('CMPI','wr_last',0)])
    P('WR.type').branch({1:'RET'},'WR.struct',[('CMPI','rb',0)])
    P('WR.struct').branch({1:'WR.aggregate'},'WR.main',[('CMPI','rd',0)])
    P('WR.aggregate').branch({(1,2):'RET'},'WR.main',[('CMPI','rb',SBB)])
    P('WR.main').a(('INTERN','wr_id','fns','fne')).branch({1:'RET'},'WR.report',[('CMP','wr_id','mnid')])
    P('WR.report').a(('LDX','diag_pos','tpos',TOKEN_POS),('LDI','diag_warning',1),
        ('SBCLR',),*[('SBOUT',c) for c in MESSAGE],('SBSAVE','diag_message')).call('DIAG.report').a(
        ('ALU','add','wr_count','wr_count','diag_reported')).ret()
    P('WR.summary').branch({1:'RET'},'WR.summary1',[('CMPI','wr_count',0)])
    P('WR.summary1').a(('OSEL',1),('COPYW','dp_num','wr_count')).call('DP.number').branch(
        {1:'WR.one'},'WR.many',[('CMPI','wr_count',1)])
    P('WR.one').o(' warning generated.\n').a(('OSEL',0)).ret()
    P('WR.many').o(' warnings generated.\n').a(('OSEL',0)).ret()

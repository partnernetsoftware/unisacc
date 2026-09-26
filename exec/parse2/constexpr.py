"""Constant-expression evaluation as delta procedures (no emitted tape).
Binding strengths come from the same prec rows as ordinary expressions.
The evaluator uses generic integer actions; it adds no executor primitive.
This slice evaluates signed 64-bit integers, not all C constant-expression
conversions. Division by zero is refused even in an unselected arm; sizeof
and casts remain unsupported. These are coverage limits, not C language rules.
"""

def install(E, P, levels, ops, enum_values, enum_defined):
    g = E.g
    arithmetic = {'+':'add', '-':'sub', '*':'mul', '/':'sdiv', '%':'srem',
                  '<<':'shl', '>>':'sar', '&':'and', '|':'or', '^':'xor'}
    comparisons = {'==':(1,), '!=':(0,2), '<':(0,), '>':(2,), '<=':(0,1), '>=':(1,2)}
    P('CE').call('CE'+str(levels[0])).tok({'?':'CE.cond'}, 'RET')
    P('CE.cond').vpush('cv').call('NEXT').call('CE').vpop('ce_test').vpush('ce_test','cv').expect(':').call('NEXT').call('CE').vpop('ce_test','ce_yes').branch({1:'RET'}, 'CE.yes', [('LDI','ce_zero',0),('C64','ce_test','ce_zero')])
    P('CE.yes').a(('COPYW','cv','ce_yes')).ret()
    for i, level in enumerate(levels):
        name='CE'+str(level); sub='CE'+str(levels[i+1]) if i+1<len(levels) else 'CE.atom'
        P(name).call(sub).label(name+'.loop').tok({o:name+'.'+o for o in ops[level]}, 'RET')
        for op in ops[level]:
            p=P(name+'.'+op).vpush('cv').call('NEXT').call(sub).vpop('ce_left')
            if op in arithmetic:
                p.a(('A64',arithmetic[op],'cv','ce_left','cv'))
                if op in ('/','%'):
                    p.branch({0:name+'.loop'}, ('rej','not covered: zero divisor in constant expression'))
                else:
                    p.goto(name+'.loop')
            elif op in comparisons:
                p.branch({comparisons[op]:name+'.'+op+'.true'}, name+'.'+op+'.false',[('C64','ce_left','cv')])
                P(name+'.'+op+'.true').a(('LDI','cv',1)).goto(name+'.loop')
                P(name+'.'+op+'.false').a(('LDI','cv',0)).goto(name+'.loop')
            else:
                assert op in ('&&','||')
                hit=name+'.'+op+'.hit';rhs=name+'.'+op+'.rhs';no=name+'.'+op+'.no'
                p.branch({1:rhs if op=='||' else no}, hit if op=='||' else rhs,[('LDI','ce_zero',0),('C64','ce_left','ce_zero')])
                P(rhs).branch({1:no},hit,[('LDI','ce_zero',0),('C64','cv','ce_zero')])
                P(hit).a(('LDI','cv',1)).goto(name+'.loop')
                P(no).a(('LDI','cv',0)).goto(name+'.loop')
    P('CE.atom').tok({E.TK_NUM:'CE.num',E.TK_ID:'CE.id','(':'CE.par','+':'CE.plus','-':'CE.minus','~':'CE.inv','!':'CE.not'},('rej','not covered: constant expression'))
    P('CE.num').a(('COPYW','cv','nv')).call('NEXT').ret()
    P('CE.id').a(('INTERN','ce_id','ps','pe'),('LDX','ce_ok','ce_id',enum_defined)).branch({1:'CE.enum'},('rej','not covered: nonconstant bound'),[('CMPI','ce_ok',1)])
    P('CE.enum').a(('LDX','cv','ce_id',enum_values)).call('NEXT').ret()
    P('CE.par').call('NEXT').call('CE').expect(')').call('NEXT').ret()
    P('CE.plus').call('NEXT').call('CE.atom').ret()
    P('CE.minus').call('NEXT').call('CE.atom').a(('LDI','ce_zero',0),('A64','sub','cv','ce_zero','cv')).ret()
    P('CE.inv').call('NEXT').call('CE.atom').a(('A64I','xor','cv','cv',-1)).ret()
    P('CE.not').call('NEXT').call('CE.atom').branch({1:'CE.one'},'CE.zero',[('LDI','ce_zero',0),('C64','cv','ce_zero')])
    P('CE.one').a(('LDI','cv',1)).ret()
    P('CE.zero').a(('LDI','cv',0)).ret()

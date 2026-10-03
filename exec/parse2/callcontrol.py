"""Bind call-control declarations to shared generators and dynamic callee facts."""
import json
import re
from pathlib import Path
from finite_rules import install as install_rules, install_template


def install(E, P, warnings, templates, addr, facts, syscalls, fpu, phase, bindings=None):
    root = Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root / ('callcontrol-' + suffix + '.tsv')).read_text().splitlines()[1:]]
    b = dict(facts) if bindings is None else bindings
    b.update(GMARK=E.GMARK, FND=E.FND, FRB=E.FRB, FRD=E.FRD, VAR=E.VAR, VS_TOP=E.VS-1,
             WF_FORMAT=51 << 40, find_end='CL.b%d' % (len(syscalls)+1))
    formats = {name:json.loads(value) for name,value in rows('format')}
    sequences = {name:E.O(json.loads(value)) for name,value in rows('text')}
    for name,value in rows('template'):
        template,index=json.loads(value)
        sequences[name]=E.O(re.split(r'(\{[^}]*\})',templates[template])[index])
    p=P('callcontrol.bindings.'+phase)
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    sections={line.split('\t')[0] for line in (root/'callcontrol-result.tsv').read_text().splitlines()[1:]}
    def section(name, **extra):
        b.update(extra)
        for part,mode,owner,kind,key in rows('fresh'):
            if part==name and (mode=='all' or warnings):
                p.cur=b[owner[1:]] if owner.startswith('$') else owner
                b[key]=p.fresh(kind)
        for mode in ('all','warnings' if warnings else 'plain'):
            if name+'.'+mode in sections:
                install_rules(E.g,root,'callcontrol',bindings=b,sequences=sequences,classes=classes,section=name+'.'+mode)
    if phase == 'begin':
        section('part0')
        b['f_part1_1427_CL_ad_1'] = addr(P('CL.fpg')).cur
        section('part2')
        section('va0')
        for k,nx in ((1,'CL.va2'),(2,'CL.b1')):
            section('van',va_entry='CL.va%d'%k,va_target='VA%d'%k,va_next=nx,va_register='va%d'%k)
        section('part4')
        b['f_part5_1443_VA0_ad_1'] = addr(P(b['f_part4_1442_VA0_r_1'])).cur
        section('part6')
        b['f_part7_1452_VA1_ad_1'] = addr(P(b['f_part6_1451_VA1_r_1'])).cur
        section('part8')
        install_rules(E.g,root,'varargs',bindings=b,section='aggregate')
        section('part10')
        for k,_ in enumerate(syscalls,1):
            section('sysfind',find_entry='CL.b%d'%k,find_hit='CL.s%d'%k,find_next='CL.b%d'%(k+1) if k<len(syscalls) else 'CL.sj0',sys_register='sy%d'%k,sys_index=k)
        for k in range(6):
            entry = 'CL.sj%d' % k
            sj = dict(entry=entry, hit='CL.sj' if k < 3 else 'CL.lj',
                      next='CL.sj%d' % (k+1) if k < 5 else 'CL.b%d' % (len(syscalls)+1),
                      register='sj%d' % k)
            install_template(E.g, root, 'callcontrol-setjmp', dict(sj=[sj]), P(entry).fresh,
                             section='sjscan')
        # These are tape-level continuations, not host libc calls.  The
        # reference allocates the two resume labels before parsing the buffer.
        p=P('CL.sj')
        sj = dict(entry='CL.sj', open_ok=p.fresh('e'), open_test=p.fresh('b'),
                  next_ret=p.fresh('r'), expr_ret=p.fresh('r'),
                  close_ok=p.fresh('e'), close_test=p.fresh('b'),
                  depth_ret=p.fresh('r'), open_token=E.TK['('], close_token=E.TK[')'])
        install_template(E.g, root, 'callcontrol-setjmp',
                         dict(sj=[sj]), p.fresh, sequences=sequences, section='save-entry')
        for k in range(5):
            following='SJ.save%d'%(k+1)
            state='SJ.save%d'%k
            sequences['sj_text']=E.O('  load64 r2, [r7+%d]\n  store64 [r0+%d], r2\n'%(8*k,24+8*k))
            sj = dict(state=state, emit='SJ.saveemit%d'%k, next=following, threshold=8*(k+1))
            install_template(E.g, root, 'callcontrol-setjmp', dict(sj=[sj]), P(state).fresh,
                             sequences=sequences, section='sjframe')
        p=P('SJ.save5')
        save = dict(entry='SJ.save5', first_ret=p.fresh('r'), second_ret=p.fresh('r'),
                    third_ret=p.fresh('r'))
        install_template(E.g, root, 'callcontrol-setjmp',
                         dict(save=[save]), p.fresh, sequences=sequences, section='restore-entry')
        for k in range(5):
            following='SJ.restore%d'%(k+1)
            state='SJ.restore%d'%k
            sequences['sj_text']=E.O('  load64 r2, [r1+%d]\n  store64 [r7+%d], r2\n'%(24+8*k,8*k))
            sj = dict(state=state, emit='SJ.restoreemit%d'%k, next=following, threshold=8*(k+1))
            install_template(E.g, root, 'callcontrol-setjmp', dict(sj=[sj]), P(state).fresh,
                             sequences=sequences, section='sjframe')
        p=P('SJ.restore5')
        restore = dict(entry='SJ.restore5', first_ret=p.fresh('r'), second_ret=p.fresh('r'),
                       third_ret=p.fresh('r'))
        install_template(E.g, root, 'callcontrol-setjmp', dict(restore=[restore]), p.fresh,
                         sequences=sequences, section='restore-tail')
        p=P('CL.lj')
        longjmp = dict(entry='CL.lj', open_ok=p.fresh('e'), open_test=p.fresh('b'),
                       next1_ret=p.fresh('r'), expr1_ret=p.fresh('r'),
                       comma_ok=p.fresh('e'), comma_test=p.fresh('b'),
                       next2_ret=p.fresh('r'), expr2_ret=p.fresh('r'),
                       close_ok=p.fresh('e'), close_test=p.fresh('b'),
                       next3_ret=p.fresh('r'), postix_ret=p.fresh('r'),
                       open_token=E.TK['('], comma_token=E.TK[','], close_token=E.TK[')'])
        install_template(E.g, root, 'callcontrol-setjmp', dict(longjmp=[longjmp]), p.fresh,
                         sequences=sequences, section='longjmp')
        section('part12')
        for k,(base,suffix) in enumerate(((b['DBL'],'d'),(b['FLT'],'s'))):
            sequences['text12']=E.O(formats['sqrt']%fpu[suffix+'sqrt'])
            section('sqrt',sqrt_entry='CL.sqrt%d'%k,sqrt_body='SQ%d'%k,sqrt_next='CL.sqrt1' if k==0 else 'CL.def',sqrt_convert='TO.'+suffix,sqrt_base=base,sqrt_register='sqrt%d'%k)
    else:
        section('part15')
        install_rules(E.g,root,'call-conversion',bindings=b,classes={k:[b[k]] for k in ('DBL','FLT','BOOL')},section='main')
        section('part17')
        for k,(_,opcode,width) in enumerate(syscalls,1):
            registers=', '.join('r%d'%i for i in range(width))
            if opcode == 'hostcall':
                text = '  imm r2, 0\n  .hostcall r0, r1\n'  # the reference's sysargs zero-fills r0..r2
            elif opcode.startswith('hostaddr'):
                text = '  .hostaddr r0, %s\n' % opcode[-1]
            else:
                text = formats['syscall']%('6' if width==6 else '',opcode,registers)
            sequences['text35']=E.O(text)
            section('sysemit',sys_entry='CL.sysz' if k==1 else 'CL.w%d'%k,sys_hit='CL.y%d'%k,sys_arity='CL.ar%d'%k if opcode.startswith('host') else 'CL.y%d'%k,host_arity_test='CL.art%d'%k,sys_test='CL.z%d'%k,sys_zero='CL.zz%d'%k,sys_emit='CL.x%d'%k,sys_next='CL.w%d'%(k+1) if k<len(syscalls) else 'DEAD',sys_width=width,sys_index=k)
            if opcode.startswith('host'):
                section('hostarity')
        section('part19')
    return b

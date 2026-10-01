"""Separate compilation linkage and entry selection, as ordinary delta actions."""
DECL = 230 << 40

def install(E, P, start, definitions):
    g = E.g
    # FN clears fstatic after taking the declaration specifier. Keep the
    # storage fact for the later GV.storage/GV.record hooks.
    mode, row = g.st['FN']
    for key, (target, seq) in list(row.items()):
        row[key] = (target, g.seq([('COPYW', 'um_static', 'fstatic')] + list(g.seqs[seq])))
    def original(state):
        name = 'UM.original.' + state
        assert name not in g.st
        g.st[name] = g.st.pop(state)
        for how in ('P', 'L'):
            if (state, how) in definitions:
                definitions[name, how] = definitions.pop((state, how))
        g.labels.add(name)
        return name
    def hook(state, proc):
        name = original(state)
        P(state).call(proc).goto(name)
        return name
    p = P('UM.start')
    for key, value in (('funit', 'um_unit'), ('object', 'um_object')):
        p.a(('SBCLR',), *[('SBOUT', c) for c in ('\0cli/' + key).encode()],
            ('SBFIND', 'um_blob'), ('BLEN', value, 'um_blob'))
    p.goto(start)
    # Move the fixed entry sequence from before the source to before __init_u.
    header = list(E.O(E.HEADER))
    matches = []
    for state, (mode, row) in list(g.st.items()):
        for key, (target, seq) in row.items():
            acts = list(g.seqs[seq])
            for at in range(len(acts) - len(header) + 1):
                if acts[at:at+len(header)] == header:
                    matches.append((state, key, target, acts, at))
                    break
    states = {state for state, *_ in matches}
    assert len(states) == 1, states
    for state, key, target, acts, at in matches:
        resume = 'UM.header.resume'
        g.labels.add(resume)
        g.st[state][1][key] = ('UM.header', g.seq(acts[:at] + [('PUSH', resume)]))
        g.on(resume, range(257), target, acts[at+len(header):], 'r')
    P('UM.header').branch({0:'UM.header.emit'},'UM.header.unit',[('RLD','um_unit')])
    P('UM.header.unit').o('.unit 2\n').ret()
    P('UM.header.emit').o(E.HEADER).ret()
    hook('FN.def1', 'UM.function')
    P('UM.function').branch({0:'RET'},'UM.function.static',[('RLD','um_unit')])
    P('UM.function.static').branch({1:'RET'},'UM.function.emit',[('CMPI','fstat_cur',1)])
    P('UM.function.emit').o('.global ').a(('SPAN2','fns','fne')).o('\n').ret()
    # librarydata already records the actual storage class in ld_decl.
    storage = original('GV.storage')
    P('GV.storage').a(('ALU','or','um_enabled','um_unit','um_object')).branch({0:storage},'UM.storage',[('RLD','um_enabled')])
    P('UM.storage').a(('INTERN','um_id','fns','fne'),('LDX','um_decl','um_id',DECL)).branch({1:'UM.extern.test'},'UM.definition',[('CMPI','ld_decl',E.TK['type=extern'])])
    P('UM.extern.test').branch({1:'UM.definition'},'UM.extern',[('CMPI','tk',E.TK['='])])
    P('UM.extern').branch({0:'UM.extern.emit'},'GV.record',[('RLD','um_decl')])
    P('UM.extern.emit').o('.extern g_').a(('SPAN2','fns','fne')).o('\n').a(('LDI','um_decl',1),('STX','um_id',DECL,'um_decl')).goto('GV.record')
    P('UM.definition').a(('LDI','um_defined',2),('STX','um_id',DECL,'um_defined')).branch({0:storage},'UM.global.static',[('RLD','um_unit')])
    P('UM.global.static').branch({1:storage},'UM.global.duplicate',[('CMPI','um_static',1)])
    P('UM.global.duplicate').branch({2:storage},'UM.global.emit',[('RLD','um_decl')])
    P('UM.global.emit').o('.global g_').a(('SPAN2','fns','fne')).o('\n').goto(storage)
    record = original('GV.record')
    P('GV.record').branch({0:record},'UM.gdef.test',[('RLD','um_unit')])
    P('UM.gdef.test').branch({1:'UM.gdef.static'},record,[('CMPI','tk',E.TK['='])])
    P('UM.gdef.static').branch({1:record},'UM.gdef.emit',[('CMPI','um_static',1)])
    P('UM.gdef.emit').o('.gdef g_').a(('SPAN2','fns','fne')).o('\n').goto(record)
    end = original('END')
    P('END').branch({0:end},'UM.end',[('RLD','um_unit')])
    P('UM.end').a(('LDX','t','mnid',E.FND)).branch({1:'UM.entry'},'UM.init',[('CMPI','t',1)])
    P('UM.entry').o(E.HEADER).goto('UM.init')
    # Keep the existing initializer replay and footer; replace only its heading.
    init = list(E.O('__init:\n'))
    mode,row = g.st['END.ok']
    targets = set()
    for key,(target,seq) in row.items():
        acts=list(g.seqs[seq]);assert acts[:len(init)]==init
        targets.add((target,tuple(acts[len(init):])))
    assert len(targets)==1
    target, acts = targets.pop()
    P('UM.init').o('.global __init_u\n__init_u:\n').a(*acts).goto(target)
    # Undefined calls become declarations in source emission order.
    error = original('UD.error')
    P('UD.error').branch({0:error},'UM.unresolved',[('RLD','um_unit')])
    P('UM.unresolved').a(('LDI','ud_one',1),('STX','ud_id',33<<40,'ud_one'),('ALUI','mul','um_extindex','um_extcount',2),('STX','um_extindex',DECL+1,'ud_start'),('ALUI','add','um_extindex','um_extindex',1),('STX','um_extindex',DECL+1,'ud_end'),('ALUI','add','um_extcount','um_extcount',1)).goto('UD.nextline')
    ok = original('UD.ok')
    P('UD.ok').branch({0:ok},'UM.externs',[('RLD','um_unit')])
    P('UM.externs').a(('SPAN2','ud_zero','ud_len'),('LDI','um_exti',0)).goto('UM.externloop')
    P('UM.externloop').branch({0:'UM.externline'},'UM.externfinish',[('CMP','um_exti','um_extcount')])
    P('UM.externline').a(('ALUI','mul','um_extindex','um_exti',2),('LDX','um_exts','um_extindex',DECL+1),('ALUI','add','um_extindex','um_extindex',1),('LDX','um_exte','um_extindex',DECL+1)).o('.extern ').a(('SPAN2','um_exts','um_exte')).o('\n').a(('ALUI','add','um_exti','um_exti',1)).goto('UM.externloop')
    P('UM.externfinish').a(('INPOP',)).ret()
    return 'UM.start'

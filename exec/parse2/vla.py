"""Local VLA storage and scope restoration, on existing generic actions.
The runtime length is evaluated once. Static frame slots retain its byte
count and address; each allocating scope saves the pre-allocation stack.
"""
def install(E,P,SIZE,FRAME,DEP,ENUM,bad,UNS):
    P('VL.enter').a(('ALUI','add','vl_depth','vl_depth',1),('LDI','vl_zero',0),('STX','vl_depth',FRAME,'vl_zero')).ret()
    P('VL.leave').a(('LDX','vl_slot','vl_depth',FRAME)).branch({1:'VL.left'},'VL.restore',[('CMPI','vl_slot',0)])
    P('VL.restore').o('  load64 r7, [r6-').num('vl_slot').o(']\n').goto('VL.left')
    P('VL.left').a(('ALUI','sub','vl_depth','vl_depth',1)).ret()
    P('VL.targets').a(('STX','lcnt',DEP,'vl_depth')).goto('VL.breaktarget')
    P('VL.breaktarget').a(('STX','lbrk',DEP,'vl_depth')).ret()
    P('VL.back').a(('ALUI','add','vl_i','vl_to',1)).goto('VL.search')
    P('VL.search').branch({2:'RET'},'VL.at',[('CMP','vl_i','vl_depth')])
    P('VL.at').a(('LDX','vl_slot','vl_i',FRAME)).branch({1:'VL.next'},'VL.backout',[('CMPI','vl_slot',0)])
    P('VL.next').a(('ALUI','add','vl_i','vl_i',1)).goto('VL.search')
    P('VL.backout').o('  load64 r7, [r6-').num('vl_slot').o(']\n').ret()
    # Mirror isconstdim: an identifier other than an enum makes the first
    # bound dynamic. Preprocessing has already expanded macros.
    P('VL.classify').a(('COPYW','vl_back','tpos'),('LDI','vl_dynamic',0),('LDI','vl_nest',0)).call('NEXT').goto('VL.scan')
    P('VL.scan').tok({'[':'VL.open',']':'VL.close',E.TK_ID:'VL.id','eof':'VL.done'},'VL.more')
    P('VL.open').a(('ALUI','add','vl_nest','vl_nest',1)).goto('VL.more')
    P('VL.close').branch({1:'VL.done'},'VL.down',[('CMPI','vl_nest',0)])
    P('VL.down').a(('ALUI','sub','vl_nest','vl_nest',1)).goto('VL.more')
    P('VL.id').a(('INTERN','vl_key','ps','pe'),('LDX','vl_enum','vl_key',ENUM)).branch({1:'VL.more'},'VL.dynamic',[('CMPI','vl_enum',1)])
    P('VL.dynamic').a(('LDI','vl_dynamic',1)).goto('VL.done')
    P('VL.more').call('NEXT').goto('VL.scan')
    P('VL.done').a(('JUMP','vl_back')).call('NEXT').ret()
    P('VL.decl').call('NEXT').vpush('ips','ipe','td','tb','bd').call('EXPR').vpop('ips','ipe','td','tb','bd').expect(']').call('ELSZ').o('  imm r2, ').num('es').o('\n  mul64 r0, r0, r2\n').a(
        ('ALUI','add','cur','cur',8),('COPYW','vl_size','cur')).call('MAXF').o('  store64 [r6-').num('vl_size').o('], r0\n').a(('LDX','vl_slot','vl_depth',FRAME)).branch({1:'VL.save'},'VL.allocate',[('CMPI','vl_slot',0)])
    P('VL.save').a(('ALUI','add','cur','cur',8),('STX','vl_depth',FRAME,'cur')).call('MAXF').o('  store64 [r6-').num('cur').o('], r7\n').goto('VL.allocate')
    P('VL.allocate').o('  imm r2, 7\n  add64 r0, r0, r2\n  imm r2, -8\n  and64 r0, r0, r2\n  sub64 r7, r7, r0\n').a(
        ('ALUI','add','td','td',1),('LDI','dar',0),('LDI','dsz',8)).call('DECLN').a(('STX','v',SIZE,'vl_size')).o('  store64 [r6-').num('s').o('], r7\n').call('NEXT').tok({',':'S.dcm'},'S.dend')
    P('VL.sizecheck').a(('LDI','vl_bytes',0)).branch({1:'VL.sizeget'},'RET',[('CMPI','drop',0)])
    P('VL.sizeget').a(('LDX','vl_bytes','vid',SIZE)).ret()
    P('VL.sizeclose').call('NEXT').goto('VL.sizeout')
    P('VL.sizeout').branch({1:'VL.sizeload'},bad('static initializer requires a constant size'),[('CMPI','si_active',0)])
    P('VL.sizeload').o('  load64 r0, [r6-').num('vl_bytes').o(']\n').a(('LDI','vt',0),('LDI','vb',UNS+8)).ret()

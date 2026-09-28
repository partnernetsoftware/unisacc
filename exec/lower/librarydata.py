"""Typed borrowed data addresses resolved by the lowering delta.
Real tape data definitions retain ordinary .lea semantics. Other names must
match a writable USBIND1 data record and all four canonical ABI facts.
"""
def install(E,os_,ids):
 from code import REG,TXT
 from data import DEFINED
 from modelbindings import IDS,ADDRESS,DESC,KIND
 P,g=E.P,E.g
 def bytebranch(name,cases,other):
  for c,nx in cases.items():g.on(name,[c],nx,[],'b')
  g.on(name,[c for c in range(257) if c not in cases],other,[],'b')
 g.st['LBD.original.dispatch']=g.st.pop('C.dispatch');g.labels.add('LBD.original.dispatch')
 P('C.dispatch').branch({1:'LBD.capability'},'LBD.original.dispatch',[('CMP','op',ids['.libraryaddr'])])
 P('LBD.capability').branch({1:'LBD.arity' if os_ in ('osx','lnx') else 'LBI.fail'},'LBI.fail',[('CMPI','library_module',1)])
 P('LBD.arity').branch({1:'LBD.reg'},'LBI.fail',[('CMPI','na',6)])
 P('LBD.reg').a(('LDX','ld_reg','a0',REG)).branch({1:'LBI.fail'},'LBD.local',[('CMPI','ld_reg',0)])
 P('LBD.local').a(('LDX','ld_def','a1',DEFINED)).branch({1:'LBD.owned'},'LBD.cached',[('CMPI','ld_def',1)])
 P('LBD.owned').a(('COPYW','op',ids['.lea']),('LDI','na',2)).goto('GENERIC')
 P('LBD.cached').branch({1:'LBD.resolve'},'LBI.read',[('CMPI','lbi_validated',1)])
 g.st['LBD.original.emit']=g.st.pop('LBI.emit');g.labels.add('LBD.original.emit')
 P('LBI.emit').branch({1:'LBD.resolve'},'LBD.original.emit',[('CMP','op',ids['.libraryaddr'])])
 P('LBD.resolve').a(('LDX','ld_blob','a1',TXT),('INPUSH','ld_blob')).goto('LBD.g')
 bytebranch('LBD.g',{103:'LBD.prefix'},'LBI.fail')
 P('LBD.prefix').a(('ADV',)).goto('LBD.underscore')
 bytebranch('LBD.underscore',{95:'LBD.name'},'LBI.fail')
 P('LBD.name').a(('ADV',),('MARK','ld_start'),('XLEN','ld_end'),('INTERN','ld_id','ld_start','ld_end'),('INPOP',),('LDI','ld_i',0)).goto('LBD.find')
 P('LBD.find').branch({1:'LBI.fail'},'LBD.compare',[('CMP','ld_i','lbi_count')])
 P('LBD.compare').a(('LDX','ld_recordid','ld_i',IDS)).branch({1:'LBD.kind'},'LBD.next',[('CMP','ld_recordid','ld_id')])
 P('LBD.next').a(('ALUI','add','ld_i','ld_i',1)).goto('LBD.find')
 P('LBD.kind').a(('LDX','ld_kind','ld_i',KIND)).branch({1:'LBD.fact0'},'LBI.fail',[('CMPI','ld_kind',1)])
 for arg,field,nx in [(2,0,'LBD.fact3'),(3,3,'LBD.fact4'),(4,4,'LBD.fact5'),(5,5,'LBD.emit')]:
  P('LBD.fact'+str(field)).a(('LDX','ld_blob','a'+str(arg),TXT),('INPUSH','ld_blob')).call('LBD.number').a(('INPOP',),('ALUI','mul','ld_index','ld_i',42),('ALUI','add','ld_index','ld_index',field),('LDX','ld_value','ld_index',DESC)).branch({1:nx},'LBI.fail',[('C64U','ld_value','ld_number')])
 P('LBD.number').a(('LDI','ld_number',0),('LDI','ld_digits',0)).goto('LBD.digit')
 bytebranch('LBD.digit',{c:'LBD.digit.'+str(c) for c in range(48,58)},'LBD.numberend')
 for c in range(48,58):
  P('LBD.digit.'+str(c)).a(('ALUI','mul','ld_number','ld_number',10),('ALUI','add','ld_number','ld_number',c-48),('ALUI','add','ld_digits','ld_digits',1),('ADV',)).branch({2:'LBI.fail'},'LBD.digit',[('CMPI','ld_number',1024)])
 bytebranch('LBD.numberend',{256:'LBD.numbervalid'},'LBI.fail')
 P('LBD.numbervalid').branch({1:'LBI.fail'},'RET',[('CMPI','ld_digits',0)])
 P('LBD.emit').o('imm ').a(('COPYW','tok','a0')).call('PRINT').o(', ').a(('LDX','ld_print','ld_i',ADDRESS)).call('LBD.print64').goto('C.nl')
 # Borrowed host addresses are 64-bit; the legacy PRN intentionally uses ALU32.
 P('LBD.print64').a(('LDI','ld_pk',0),('LDI','ld_zero',0)).goto('LBD.printdigit')
 P('LBD.printdigit').a(('A64I','urem','ld_pd','ld_print',10),('A64I','udiv','ld_print','ld_print',10),('STX','ld_pk',199 << 40,'ld_pd'),('ALUI','add','ld_pk','ld_pk',1)).branch({1:'LBD.printout'},'LBD.printdigit',[('C64U','ld_print','ld_zero')])
 P('LBD.printout').a(('ALUI','sub','ld_pk','ld_pk',1),('LDX','ld_pd','ld_pk',199 << 40),('ALUI','add','ld_pd','ld_pd',48),('OUTW','ld_pd')).branch({1:'RET'},'LBD.printout',[('CMPI','ld_pk',0)])

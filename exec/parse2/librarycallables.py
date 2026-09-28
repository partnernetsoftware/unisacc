"""Explicit model callable introduction/typed indirect call and declaration catalog.
Disabled resource follows the exact old states. Handles never use raw callr.
SCRIPT/NATIVE origin is selected after final source definitions are known.
"""
SIGKEY,SIGLIST,FNSEEN,FNNAME,FNSIG,FNID=(i<<40 for i in range(440,446))
assert not set(range(440,446)) & (set(range(72,83))|set(range(300,358))|set(range(400,438)))
def install(E,P,b,start):
 from modelinput import u64
 from libraryimports import BYNAME,ADDRESS
 from libraryvariadic import REQUESTS
 from unresolved import DEFINED
 from libraryexports import VARIADIC
 g=E.g
 def hook(name,enabled):
  old='LC.original.'+name;g.st[old]=g.st.pop(name);g.labels.add(old)
  alias='LC.hook.'+name;P(alias).branch({1:enabled},old,[('CMPI','lc_enabled',1)]);g.st[name]=g.st[alias]
 def blob(p,r):return p.a(('INPUSH',r),('XLEN','lc_len'),('SPAN2','lx_zero','lc_len'),('INPOP',))
 def u(p,r):return p.a(('COPYW','lx_v',r)).call('LX.u64')
 def number(p,r):return p.a(('COPYW','li_print',r)).call('LI.print64')
 u64(E,'LC.resource',b'\0library/callables','lc_enabled','lc_present','LX.fail')
 u64(E,'LC.makeaddr',b'\0library/callablemake','lc_make','lc_makepresent','LX.fail')
 u64(E,'LC.calladdr',b'\0library/callablecall','lc_call','lc_callpresent','LX.fail')
 P('LC.start').call('LC.resource').branch({1:'LC.enabled'},start,[('CMPI','lc_enabled',1)])
 P('LC.enabled').call('LC.makeaddr').call('LC.calladdr').branch({1:'LX.fail'},'LC.callcheck',[('CMPI','lc_make',0)])
 P('LC.callcheck').branch({1:'LX.fail'},start,[('CMPI','lc_call',0)])
 P('LC.key').a(('LDX','lc_key','lc_sig',SIGKEY)).branch({1:'LC.keynew'},'RET',[('CMPI','lc_key',0)])
 P('LC.keynew').a(('ALUI','add','lc_keys','lc_keys',1)).branch({2:'LX.fail'},'LC.keyput',[('CMPI','lc_keys',1024)])
 P('LC.keyput').a(('COPYW','lc_key','lc_keys'),('STX','lc_sig',SIGKEY,'lc_key'),('STX','lc_key',SIGLIST,'lc_sig')).ret()
 hook('FNV.y','LC.fnvalue')
 P('LC.fnvalue').a(('LDX','lc_sig','v',b['FPS_FN'])).call('LC.key').a(('LDX','lc_fn','v',FNSEEN)).branch({1:'LC.fnnew'},'LC.fnemit',[('CMPI','lc_fn',0)])
 P('LC.fnnew').a(('ALUI','add','lc_fns','lc_fns',1),('COPYW','lc_fn','lc_fns'),('STX','v',FNSEEN,'lc_fn'),('STX','lc_fn',FNSIG,'lc_sig'),('STX','lc_fn',FNID,'v'),('OLEN','lc_cut'),('SPAN2','ips','ipe'),('OCUT','lc_name','lc_cut'),('STX','lc_fn',FNNAME,'lc_name')).goto('LC.fnemit')
 p=P('LC.fnemit').o('  call __us_callable_make_');number(p,'lc_fn').o('\n').a(('LDI','vt',1),('LDI','isfn',1),('COPYW','vb','lc_sig'),('LDX','t','v',E.VAR),('STX','vb',b['FPS_VAR'],'t')).ret()
 # Override the continuation immediately after FS.CALLTYPE, preserving parser stack.
 continuations={a[1] for name,(mode,row) in g.st.items() for nx,q in row.values() if nx=='FS.CALLTYPE' for a in g.seqs[q] if a[0]=='PUSH'}
 assert len(continuations)==1,continuations
 name=continuations.pop();old='LC.original.fpclassified';g.st[old]=g.st.pop(name);g.labels.add(old)
 P(name).branch({1:'LC.fpclassified'},old,[('CMPI','lc_enabled',1)])
 P('LC.fpclassified').branch({1:'LX.fail'},'LC.fpkey',[('CMPI','call_sig',0)])
 P('LC.fpkey').a(('COPYW','lc_sig','call_sig')).call('LC.key').a(('LDI','fp_stacked',1)).goto(old)
 hook('CL.vindirect','LC.indirect')
 P('LC.indirect').a(('COPYW','lc_sig','call_sig'),('LDX','lc_var','call_sig',VARIADIC)).branch({1:'LX.fail'},'LC.indirectkey',[('CMPI','lc_var',1)])
 P('LC.indirectkey').call('LC.key').a(('LDX','lc_rd','call_sig',b['FPS_RD']),('LDX','lc_rb','call_sig',b['FPS_RB']),('LDI','lc_resultbytes',8)).branch({1:'LC.resultscalar'},'LC.allocate',[('CMPI','lc_rd',0)])
 P('LC.resultscalar').branch({0:'LC.allocate'},'LC.resultaggregate',[('CMPI','lc_rb',b['SBB'])])
 P('LC.resultaggregate').a(('ALUI','sub','lc_sid','lc_rb',b['SBB']),('LDX','lc_resultbytes','lc_sid',b['SSZ'])).goto('LC.allocate')
 P('LC.allocate').a(('ALU','add','cur','cur','lc_resultbytes'),('COPYW','lc_resultoffset','cur')).call('MAXF').goto('LC.emitcall')
 p=P('LC.emitcall').o('  .frame 48\n  load64 r1, [r7+').a(('ALUI','mul','lc_n','na',8),('ALUI','add','lc_n','lc_n',48));number(p,'lc_n').o(']\n  store64 [r7+0], r1\n  imm r1, ');number(p,'lc_key').o('\n  store64 [r7+8], r1\n  imm r1, 48\n  add64 r1, r7, r1\n  store64 [r7+16], r1\n  imm r1, ');number(p,'lc_resultoffset').o('\n  sub64 r1, r6, r1\n  store64 [r7+24], r1\n  imm r1, ');number(p,'na').o('\n  store64 [r7+32], r1\n  imm r1, 0\n  store64 [r7+40], r1\n  imm r1, ');number(p,'lc_call').o('\n  mov r0, r7\n  .librarycall r1, r0\n  .frame -').a(('ALUI','add','lc_n','na',1),('ALUI','mul','lc_n','lc_n',8),('ALUI','add','lc_n','lc_n',48));number(p,'lc_n').o('\n  imm r0, ');number(p,'lc_resultoffset').o('\n  sub64 r0, r6, r0\n').branch({1:'LC.loadscalar'},'LC.scalarword',[('CMPI','lc_rd',0)])
 P('LC.loadscalar').branch({0:'LC.scalarword'},'LC.resulttype',[('CMPI','lc_rb',b['SBB'])])
 P('LC.scalarword').o('  load64 r0, [r0+0]\n').goto('LC.resulttype')
 P('LC.resulttype').a(('LDI','vt',0),('LDI','vb',8)).call('FS.RESULT').call('EN.VALUE').call('NEXT').ret()
 # Aggregate return expressions may contain a typed indirect call.
 hook('S.rs','LC.structreturn')
 P('LC.structreturn').call('EXPR').branch({1:'LC.structreturnbase'},'LX.fail',[('CMPI','vt',0)])
 P('LC.structreturnbase').branch({1:'LC.structreturntoken'},'LX.fail',[('CMP','vb','rb')])
 P('LC.structreturntoken').branch({1:'LC.structreturncopy'},'LX.fail',[('CMPI','tk',E.TK[';'])])
 P('LC.structreturncopy').o('  mov r1, r0\n  .lea r0, __rv_').a(('SPAN2','fns','fne')).o('\n').call('WCOPY').goto('S.rj')
 # Force the final wrapper pass even when no native imports were registered.
 oldcalls='LC.original.calls0';g.st[oldcalls]=g.st.pop('UD.calls0');g.labels.add(oldcalls)
 P('LC.calls0select').branch({1:'LC.forcewrappers'},oldcalls,[('CMPI','lc_enabled',1)]);g.st['UD.calls0']=g.st['LC.calls0select']
 P('LC.forcewrappers').branch({1:oldcalls},'LI.wrapstart',[('CMPI','li_emitted',1)])
 # Emit source/native introductions only after final source-priority facts exist.
 old='LC.original.wrapend';g.st[old]=g.st.pop('LI.wrapend');g.labels.add(old)
 P('LC.wrapendselect').branch({1:'LC.helpers'},old,[('CMPI','lc_enabled',1)]);g.st['LI.wrapend']=g.st['LC.wrapendselect']
 P('LC.helpers').a(('LDI','lc_fn',1)).goto('LC.helperloop')
 P('LC.helperloop').branch({2:old},'LC.helperload',[('CMP','lc_fn','lc_fns')])
 p=P('LC.helperload').a(('LDX','lc_id','lc_fn',FNID),('LDX','lc_sig','lc_fn',FNSIG),('LDX','lc_name','lc_fn',FNNAME),('LDX','lc_key','lc_sig',SIGKEY)).o('__us_callable_make_');number(p,'lc_fn').o(':\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 56\n').a(('LDX','lc_defined','lc_id',DEFINED)).branch({1:'LC.helperscript'},'LC.helpernative',[('CMPI','lc_defined',1)])
 p=P('LC.helperscript').o('  imm r1, 1\n  store64 [r7+0], r1\n  .lea r1, ');blob(p,'lc_name').o('\n').goto('LC.helperbody')
 P('LC.helpernative').a(('LDX','lc_binding','lc_id',BYNAME)).branch({1:'LX.fail'},'LC.helpernativeaddress',[('CMPI','lc_binding',0)])
 p=P('LC.helpernativeaddress').a(('ALUI','sub','lc_binding','lc_binding',1),('LDX','lc_raw','lc_binding',ADDRESS)).o('  imm r1, 2\n  store64 [r7+0], r1\n  imm r1, ');number(p,'lc_raw').o('\n').goto('LC.helperbody')
 p=P('LC.helperbody').o('  store64 [r7+16], r1\n  imm r1, ');number(p,'lc_key').o('\n  store64 [r7+8], r1\n  imm r1, 48\n  add64 r1, r7, r1\n  store64 [r7+24], r1\n  imm r1, 0\n  store64 [r7+32], r1\n  store64 [r7+40], r1\n  imm r1, ');number(p,'lc_make').o('\n  mov r0, r7\n  .librarycall r1, r0\n  load64 r0, [r7+48]\n  .frame -56\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n').a(('ALUI','add','lc_fn','lc_fn',1)).goto('LC.helperloop')
 old='LC.original.outer';g.st[old]=g.st.pop('LX.outer');g.labels.add(old)
 P('LC.outerselect').branch({1:'LC.outer'},old,[('CMPI','lc_enabled',1)]);g.st['LX.outer']=g.st['LC.outerselect']
 P('LC.outer').a(('OCUT','lx_metadata','lx_zero')).o('USCPLAN1').a(('COPYW','lx_v','lv_requests')).call('LX.u64').a(('LDI','lc_i',0)).goto('LC.requests')
 P('LC.requests').branch({1:'LC.catalogstart'},'LC.requestcopy',[('CMP','lc_i','lv_requests')])
 p=P('LC.requestcopy').a(('LDX','lc_blob','lc_i',REQUESTS));blob(p,'lc_blob').a(('ALUI','add','lc_i','lc_i',1)).goto('LC.requests')
 P('LC.catalogstart').a(('OCUT','lc_calls','lx_zero')).o('USLCALL1\n').a(('COPYW','lx_v','lc_keys')).call('LX.u64').a(('LDI','lc_catalogi',1)).goto('LC.catalogloop')
 P('LC.catalogloop').branch({2:'LC.envelope'},'LC.catalogsignature',[('CMP','lc_catalogi','lc_keys')])
 p=P('LC.catalogsignature').a(('LDX','lx_sig','lc_catalogi',SIGLIST),('OLEN','lc_cut')).o('callable').a(('OCUT','lx_nameblob','lc_cut')).call('LX.signature');u(p,'lc_catalogi').a(('BLEN','lc_siglen','lx_sigblob'));u(p,'lc_siglen');blob(p,'lx_sigblob').a(('ALUI','add','lc_catalogi','lc_catalogi',1)).goto('LC.catalogloop')
 p=P('LC.envelope').a(('OCUT','lc_catalog','lx_zero')).o('USLTAPE3\n')
 for r in ('lx_tape','lx_metadata','lc_calls','lc_catalog'):p.a(('BLEN','lx_v',r)).call('LX.u64')
 for r in ('lx_tape','lx_metadata','lc_calls','lc_catalog'):blob(p,r)
 p.goto('LX.accept')
 return 'LC.start'

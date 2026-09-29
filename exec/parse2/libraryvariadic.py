"""One-pass, model-owned promoted variadic call graphs and source priority.
The host receives only final length-framed declarations; no source decisions.
"""
SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS=(i<<40 for i in range(400,408))
def install(E,P,b,integers):
    from libraryexports import RETURNRANK, PARAMRANK
    from valueranks import LVSITERANK
    from libraryimports import BYNAME,FORMAT,IDS,PLAN,DISPATCH,CANON,RETKIND,RETWIDTH,SUPPORTED,NAMES as IMPORTNAMES
    from unresolved import DEFINED
    from libraryexports import SIGEPOCH
    import modelsignature,modelcandidates,libraryimports,libraryexports
    owned={SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS}
    for module in (modelsignature,modelcandidates,libraryimports,libraryexports):
        assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40), module.__name__
    g=E.g
    def hook(name,proc):
        old='LV.original.'+name;g.st[old]=g.st.pop(name);g.labels.add(old)
        alias='LV.hook.'+name
        P(alias).a(('PUSH',old)).goto(proc);g.st[name]=g.st[alias]
    def blob(p,r):return p.a(('INPUSH',r),('XLEN','lv_len'),('SPAN2','lx_zero','lv_len'),('INPOP',))
    def u(p,r):return p.a(('COPYW','lx_v',r)).call('LX.u64')
    hook('FPCALL','LV.indirect')
    P('LV.indirect').a(('LDI','lv_site',0)).ret()
    hook('CL.ok','LV.begin')
    P('LV.begin').a(('LDI','lv_site',0),('INTERN','lv_fn','ips','ipe'),('LDX','lv_binding','lv_fn',BYNAME)).branch({1:'RET'},'LV.beginformat',[('CMPI','lv_binding',0)])
    P('LV.beginformat').a(('ALUI','sub','lv_binding','lv_binding',1),('LDX','lv_format','lv_binding',FORMAT)).branch({2:'LV.allocate'},'RET',[('RLD','lv_format')])
    P('LV.allocate').branch({2:'LI.fail'},'LV.alloc',[('CMPI','lv_sites',8191)])
    P('LV.alloc').a(('ALUI','add','lv_sites','lv_sites',1),('COPYW','lv_site','lv_sites'),('STX','lv_site',SITEFN,'lv_fn'),('STX','lv_site',SITEBIND,'lv_binding'),('LDI','lx_zero',0),('OCUT','lv_old','lx_zero')).o('__us_vcall_').a(('COPYW','li_print','lv_site')).call('LI.print64').a(('OCUT','lv_name','lx_zero'),('STX','lv_site',NAMES,'lv_name')).goto('LV.restorebegin')
    blob(P('LV.restorebegin'),'lv_old').ret()
    hook('CL.a2','LV.argument')
    P('LV.argument').branch({1:'RET'},'LV.arglimit',[('CMPI','lv_site',0)])
    P('LV.arglimit').branch({2:'LI.fail'},'LV.argprefix',[('CMPI','na',1023)])
    P('LV.argprefix').a(('LDX','lv_sig','fid',b['FPS_FN']),('LDX','lv_fixed','lv_sig',b['FPS_COUNT'])).branch({0:'LV.argstore'},'LV.tail',[('CMP','na','lv_fixed')])
    P('LV.tail').branch({1:'LV.tailscalar'},'LV.argstore',[('CMPI','vt',0)])
    narrow={code:'LV.promoteint' for _,code,width,_,_ in integers if width<4};narrow[b['BOOL']]='LV.promoteint';narrow[b['FLT']]='LV.promotefloat'
    P('LV.tailscalar').branch(narrow,'LV.argstore',[('RLD','vb')])
    i32=next(code for name,code,_,_,_ in integers if name=='i32')
    # CL.default already emitted TO.d, which preserves its input descriptor.
    # Capture the effective double kind without converting those bits twice.
    P('LV.promotefloat').a(('LDI','vb',b['DBL']),('LDI','value_rank',2),('LDI','type_shape',0)).goto('LV.argstore')
    P('LV.promoteint').a(('LDI','vb',i32),('LDI','type_shape',0)).goto('LV.argstore')
    P('LV.argstore').a(('ALUI','mul','lv_index','lv_site',1024),('ALU','add','lv_index','lv_index','na'),('STX','lv_index',DEPTH,'vt'),('STX','lv_index',BASE,'vb'),('STX','lv_index',LVSITERANK,'value_rank'),('STX','lv_index',SHAPE,'type_shape')).ret()
    hook('CL.done','LV.complete')
    P('LV.complete').branch({1:'RET'},'LV.completecount',[('CMPI','lv_site',0)])
    P('LV.completecount').a(('LDX','lv_sig','fid',b['FPS_FN']),('LDX','lv_fixed','lv_sig',b['FPS_COUNT'])).branch({0:'LI.fail'},'LV.completeput',[('CMP','na','lv_fixed')])
    P('LV.completeput').a(('STX','lv_site',COUNT,'na')).ret()
    # Replace only the callee span of stacked direct calls. The following
    # return-type query still uses the real source name and signature.
    old='LV.original.direct';g.st[old]=g.st.pop('CL.vdirect');g.labels.add(old)
    P('CL.vdirect').branch({1:old},'LV.direct',[('CMPI','lv_site',0)])
    mode,row=g.st[old];assert mode=='r' and len(set(row.values()))==1
    _,seq=next(iter(row.values()));actions=list(g.seqs[seq]);at=actions.index(('SPAN2','cls','cle'))
    actions[at:at+1]=[('LDX','lv_name','lv_site',NAMES),('INPUSH','lv_name'),('XLEN','lv_len'),('SPAN2','lx_zero','lv_len'),('INPOP',)]
    g.st['LV.direct']=(mode,{k:(nx,g.seq(actions)) for k,(nx,q) in row.items()})
    # Source definitions are final now, after all units. Native source-priority
    # is never delegated to the host. A source winner gets a tail redirect.
    g.st['LV.original.wrapend']=g.st.pop('LI.wrapend');g.labels.add('LV.original.wrapend')
    P('LV.wrapend').a(('LDI','lv_iter',1)).goto('LV.wrapper');g.st['LI.wrapend']=g.st['LV.wrapend']
    P('LV.wrapper').branch({2:'LV.original.wrapend'},'LV.wrapperload',[('CMP','lv_iter','lv_sites')])
    P('LV.wrapperload').a(('LDX','li_id','lv_iter',SITEFN),('LDX','li_i','lv_iter',SITEBIND),('LDX','li_name','lv_iter',NAMES),('INPUSH','li_name'),('XLEN','lv_len'),('INTERN','lv_nameid','lx_zero','lv_len'),('INPOP',),('LDX','lv_collision','lv_nameid',DEFINED)).branch({1:'LI.fail'},'LV.sourcewinner',[('CMPI','lv_collision',1)])
    P('LV.sourcewinner').a(('LDX','lv_defined','li_id',DEFINED)).branch({1:'LV.redirect'},'LV.nativesupport',[('CMPI','lv_defined',1)])
    P('LV.nativesupport').a(('LDX','lv_supported','li_i',SUPPORTED)).branch({1:'LV.native'},'LI.fail',[('CMPI','lv_supported',1)])
    p=blob(P('LV.redirect'),'li_name').o(':\n  jump ')
    blob(p.a(('LDX','lv_realname','li_i',IMPORTNAMES)),'lv_realname').o('\n').goto('LV.next')
    P('LV.native').a(('LDX','lx_sig','li_id',b['FPS_FN']),('LDX','lx_nameblob','li_i',IMPORTNAMES)).call('LX.signature').a(('COPYW','mg_left_blob','lx_sigblob'),('BLEN','mg_left_len','lx_sigblob'),('LDX','mg_right_blob','li_i',libraryimports.TYPEDSIG),('BLEN','mg_right_len','mg_right_blob')).call('MG.equal').branch({1:'LV.signature'},'LI.fail',[('CMPI','mg_equal',1)])
    p=P('LV.signature').a(('LDX','lv_total','lv_iter',COUNT),('LDX','lv_fixed','lx_sig',b['FPS_COUNT']),('LDI','lx_supported',1),('LDI','lx_recursion',0),('LDI','lx_nodes',0),('OCUT','lv_old','lx_zero')).call('LX.support').call('LCG.begin').call('LX.magic').a(('LDI','lx_v',1)).call('LX.u64').a(('LDX','lv_realname','li_i',IMPORTNAMES),('BLEN','lx_v','lv_realname')).call('LX.u64')
    blob(p,'lv_realname').a(('OUTW','lx_zero'),('LDI','lv_one',1),('OUTW','lv_one'),('OUTW','lx_zero'),('OUTW','lv_one'))
    u(p,'lv_total').a(('LDX','lx_depth','lx_sig',b['FPS_RD']),('LDX','lx_base','lx_sig',b['FPS_RB']),('LDX','lx_rank','lx_sig',RETURNRANK),('LDX','lx_shape','lx_sig',b['FPS_RSH']),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_arraydimension',0),('LDI','lx_return',1)).call('LX.descriptor')
    u(p,'lv_total').a(('LDI','lv_arg',0),('LDI','lx_return',0)).goto('LV.sigarg')
    P('LV.sigarg').branch({1:'LV.sigend'},'LV.sigargout',[('CMP','lv_arg','lv_total')])
    P('LV.sigargout').a(('ALUI','mul','lv_index','lv_iter',1024),('ALU','add','lv_index','lv_index','lv_arg'),('LDX','lx_depth','lv_index',DEPTH),('LDX','lx_base','lv_index',BASE),('LDX','lx_rank','lv_index',LVSITERANK),('LDX','lx_shape','lv_index',SHAPE),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_arraydimension',0)).call('LX.descriptor').a(('ALUI','add','lv_arg','lv_arg',1)).goto('LV.sigarg')
    P('LV.sigend').a(('OUTW','lx_supported')).branch({1:'LV.request'},'LI.fail',[('CMPI','lx_supported',1)])
    p=P('LV.request').a(('OCUT','lv_signature','lx_zero'),('BLEN','lv_siglen','lv_signature'),('ALUI','add','lx_v','lv_siglen',32)).call('LX.u64')
    u(p,'lv_iter').a(('LDX','lv_template','li_i',PLAN));u(p,'lv_template');u(p,'lv_fixed');u(p,'lv_siglen');blob(p,'lv_signature').a(('OCUT','lv_request','lx_zero'),('STX','lv_requests',REQUESTS,'lv_request'),('ALUI','add','lv_requests','lv_requests',1));blob(p,'lv_old').a(('LDX','li_name','lv_iter',NAMES),('LDX','li_argc','lv_iter',COUNT),('LDX','li_retclass','li_i',RETKIND),('LDX','li_retwidth','li_i',RETWIDTH),('LDI','li_sourcemode',1)).goto('LI.typedemit')
    # The shared wrapper ordinarily reloads the function name. Site wrappers
    # instead keep their unique label, while retaining the same result rules.
    g.st['LV.original.typedemit']=g.st.pop('LI.typedemit');g.labels.add('LV.original.typedemit')
    P('LV.typedselect').branch({1:'LV.original.typedemit'},'LV.typedemit',[('CMPI','lv_iter',0)]);g.st['LI.typedemit']=g.st['LV.typedselect']
    mode,row=g.st['LV.original.typedemit'];old=('LDX','li_name','li_i',IMPORTNAMES)
    g.st['LV.typedemit']=(mode,{k:(nx,g.seq([('LDX','li_name','lv_iter',NAMES) if x==old else x for x in g.seqs[q]])) for k,(nx,q) in row.items()})
    P('LV.siteword').branch({1:'LV.sitezero'},'LV.siteprint',[('CMPI','lv_iter',0)])
    P('LV.sitezero').o('0').ret()
    P('LV.siteprint').a(('COPYW','li_print','lv_iter')).call('LI.print64').ret()
    g.st['LV.original.wrapnext']=g.st.pop('LI.wrapnext');g.labels.add('LV.original.wrapnext')
    P('LV.wrapnext').branch({1:'LV.original.wrapnext'},'LV.next',[('CMPI','lv_iter',0)]);g.st['LI.wrapnext']=g.st['LV.wrapnext']
    P('LV.next').a(('ALUI','add','lv_iter','lv_iter',1)).goto('LV.wrapper')
    # Requests exist only for selected native sites, never source winners.
    g.st['LV.original.outer']=g.st.pop('LX.outer');g.labels.add('LV.original.outer')
    P('LV.outerselect').branch({1:'LV.original.outer'},'LV.outer',[('CMPI','lv_requests',0)]);g.st['LX.outer']=g.st['LV.outerselect']
    p=P('LV.outer').a(('OCUT','lx_metadata','lx_zero')).o('USCPLAN1')
    u(p,'lv_requests').a(('LDI','lv_i',0)).goto('LV.requests')
    P('LV.requests').branch({1:'LV.outerframe'},'LV.requestcopy',[('CMP','lv_i','lv_requests')])
    blob(P('LV.requestcopy').a(('LDX','lv_blob','lv_i',REQUESTS)),'lv_blob').a(('ALUI','add','lv_i','lv_i',1)).goto('LV.requests')
    p=P('LV.outerframe').a(('OCUT','lv_calls','lx_zero')).o('USLTAPE2\n')
    for r in ('lx_tape','lx_metadata','lv_calls'):p.a(('BLEN','lx_v',r)).call('LX.u64')
    for r in ('lx_tape','lx_metadata','lv_calls'):blob(p,r)
    p.goto('LX.accept')

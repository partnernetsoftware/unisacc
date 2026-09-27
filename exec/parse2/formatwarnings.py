"""Format checks consume the real argument type; no speculative parsing.
The format scanner preserves the existing warning policy, not full printf
validation. Conversion identity comes from bytes, never from emitted tape.
"""
from tokenlocations import TOKEN_POS
from unusedwarnings import NAME_TOKEN

def install(E,P,DBL,FLT,FPB,SBB):
    g=E.g
    saved=('wf_start','ips','ipe','v','sys','pfblob')
    # Prepare only a format blob, keyed by the call's token position. No
    # expression is speculatively parsed and no semantic state is rewound.
    P('WF.entry').a(('LDI','wf_zero',0),('STX','tpos',51<<40,'wf_zero'),('LDX','wf_nametok','ips',NAME_TOKEN),('LDX','wf_namepos','wf_nametok',TOKEN_POS),('INPUSH','diag_source'),('JUMP','wf_namepos')).goto('WF.name0')
    for i,c in enumerate(b'printf'):
        g.on('WF.name'+str(i),[c],'WF.name'+str(i+1),[('ADV',)])
        g.els('WF.name'+str(i),'WF.notname')
    g.on('WF.name6',list(b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789'),'WF.notname',[])
    g.els('WF.name6','WF.begin',[('INPOP',)])
    P('WF.notname').a(('INPOP',)).ret()
    P('WF.begin').vpush(*saved).a(('COPYW','wf_start','tpos')).call('NEXT').tok({E.TK_STR:'WF.literal'},'WF.finish')
    P('WF.literal').call('FMT.decode').a(('STX','wf_start',51<<40,'pfblob')).goto('WF.finish')
    P('WF.finish').a(('JUMP','wf_start')).call('NEXT').vpop(*saved).ret()

    # The real argument was already parsed. wf_index is one-based (the
    # ordinary call's argument zero is the format itself).
    P('WF.reset').branch({1:'RET'},'WF.resetindex',[('CMPI','wf_format',0)])
    P('WF.resetindex').branch({1:'RET'},'WF.resetcall',[('CMPI','wf_index',0)])
    P('WF.resetcall').a(('LDI','wi_called',0)).ret()
    P('WF.argcheck').branch({1:'RET'},'WF.index',[('CMPI','wf_format',0)])
    P('WF.index').branch({1:'RET'},'WF.scanbegin',[('CMPI','wf_index',0)])
    P('WF.scanbegin').a(('LDI','wf_count',0),('INPUSH','wf_format')).goto('WF.scan')
    g.on('WF.scan',[37],'WF.flags',[('ADV',)])
    g.on('WF.scan',[256],'WF.stop',[])
    g.els('WF.scan','WF.scan',[('ADV',)])
    g.on('WF.flags',list(b'-+ #0'),'WF.flags',[('ADV',)])
    g.els('WF.flags','WF.width')
    g.on('WF.width',[42],'WF.widtharg',[('ADV',),('ALUI','add','wf_count','wf_count',1)])
    g.els('WF.width','WF.digits')
    P('WF.widtharg').branch({1:'WF.stop'},'WF.digits',[('CMP','wf_count','wf_index')])
    g.on('WF.digits',range(48,58),'WF.digits',[('ADV',)])
    g.on('WF.digits',[46],'WF.precision',[('ADV',)])
    g.els('WF.digits','WF.length0')
    g.on('WF.precision',[42],'WF.precarg',[('ADV',),('ALUI','add','wf_count','wf_count',1)])
    g.els('WF.precision','WF.precdigits')
    P('WF.precarg').branch({1:'WF.stop'},'WF.precdigits',[('CMP','wf_count','wf_index')])
    g.on('WF.precdigits',range(48,58),'WF.precdigits',[('ADV',)])
    g.els('WF.precdigits','WF.length0')
    P('WF.length0').a(('LDI','wf_long',0)).goto('WF.length')
    g.on('WF.length',list(b'hLzjt'),'WF.length',[('ADV',)])
    g.on('WF.length',[108],'WF.length',[('LDI','wf_long',1),('ADV',)])
    g.on('WF.length',[37],'WF.scan',[('ADV',)])
    g.on('WF.length',[256],'WF.stop',[])
    g.els('WF.length','WF.conversion',[('BYTE','wf_char'),('ADV',),('ALUI','add','wf_count','wf_count',1)])
    P('WF.conversion').branch({1:'WF.matched'},'WF.scan',[('CMP','wf_count','wf_index')])
    P('WF.stop').a(('INPOP',)).ret()
    P('WF.matched').a(('INPOP',)).vpush('td','tb').call('WF.check').vpop('td','tb').ret()

    P('WF.check').branch({1:'RET'},'WF.classify',[('CMPI','wi_called',1)])
    P('WF.classify').a(('LDI','wf_ptr',0),('LDI','wf_fn',0),('LDI','wf_float',0)).branch({1:'WF.base'},'WF.pointer',[('CMPI','vt',0)])
    P('WF.pointer').a(('LDI','wf_ptr',1)).branch({1:'WF.fp'},'WF.base',[('CMPI','vb',FPB)])
    P('WF.fp').branch({1:'WF.function'},'WF.base',[('CMPI','isfn',1)])
    P('WF.function').a(('LDI','wf_ptr',0),('LDI','wf_fn',1)).goto('WF.base')
    P('WF.base').branch({1:'WF.floating'},'WF.single',[('CMPI','vb',DBL)])
    P('WF.single').branch({1:'WF.floating'},'WF.dispatch0',[('CMPI','vb',FLT)])
    P('WF.floating').a(('LDI','wf_float',1)).goto('WF.dispatch0')
    g.on('WF.dispatch',[115],'WF.string',[],'r')
    g.on('WF.dispatch',[112],'WF.address',[],'r')
    # wf_char is loaded into the dispatcher register explicitly.
    for c in b'diuxXoc':g.on('WF.dispatch',[c],'WF.integer',[],'r')
    for c in b'fegFEG':g.on('WF.dispatch',[c],'WF.real',[],'r')
    g.els('WF.dispatch','RET',[],'r')
    P('WF.dispatch0').a(('RLD','wf_char')).goto('WF.dispatch')
    P('WF.string').branch({1:'WF.stringfn'},'RET',[('CMPI','wf_ptr',0)])
    P('WF.stringfn').branch({1:'WF.msg.string'},'RET',[('CMPI','wf_fn',0)])
    P('WF.address').branch({1:'WF.addressfn'},'RET',[('CMPI','wf_ptr',0)])
    P('WF.addressfn').branch({1:'WF.msg.address'},'RET',[('CMPI','wf_fn',0)])
    P('WF.integer').branch({1:'WF.msg.pointer'},'WF.intfloat',[('CMPI','wf_ptr',1)])
    P('WF.intfloat').branch({1:'WF.msg.float'},'WF.widthcheck',[('CMPI','wf_float',1)])
    P('WF.widthcheck').a(('COPYW','td','vt'),('COPYW','tb','vb')).call('ELSZ').branch({1:'WF.intwidth'},'WF.longwidth',[('CMPI','wf_long',0)])
    P('WF.intwidth').branch({1:'WF.aggregate'},'RET',[('CMPI','es',8)])
    P('WF.aggregate').branch({0:'WF.msg.long'},'RET',[('CMPI','vb',SBB)])
    P('WF.longwidth').branch({0:'WF.msg.int'},'RET',[('CMPI','es',8)])
    P('WF.real').branch({1:'WF.msg.real'},'RET',[('CMPI','wf_float',0)])
    messages={
      'string':"format specifies type 'char *' but the argument has an integer type",
      'address':"format specifies type 'void *' but the argument has an integer type",
      'pointer':"format specifies an integer type but the argument is a pointer",
      'float':"format specifies an integer type but the argument has a floating type",
      'long':"format specifies type 'int' but the argument has type 'long'",
      'int':"format specifies type 'long' but the argument has type 'int'",
      'real':"format specifies type 'double' but the argument has an integer type",
    }
    for name,message in messages.items():
        P('WF.msg.'+name).a(('SBCLR',),*[('SBOUT',c) for c in (message+' [-Wformat]').encode()],('SBSAVE','diag_message')).goto('WF.report')
    P('WF.report').a(('LDX','diag_pos','wf_at',TOKEN_POS),('LDI','diag_warning',1)).call('DIAG.report').a(('ALU','add','wr_count','wr_count','diag_reported')).ret()

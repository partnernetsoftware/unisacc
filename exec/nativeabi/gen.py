#!/usr/bin/env python3
"""Standalone model-certified FFI_CARRIER first slice; raw USLSIG2 input.
Explicit resource \\0cli/target selects one of six exact profile rules.
MS.canonical validates the entire graph; MG.root indexes its canonical form.
No host ABI classifier or new executor action. Rule/native qualification are
separate: this stage proves its finite rule, not six-platform native execution.
"""
import csv,importlib.util,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT)]
from modelgraphequality import install as graph_install,FIELDS,EDGES,LAYOUT
SIGWIRE,SIGMARK,FRAME=(i<<40 for i in range(480,483))

def install(E):
 P=E.P;graph_install(E)
 owned={SIGWIRE,SIGMARK,FRAME}
 for module in tuple(sys.modules.values()):
  if module is not None and module.__name__!=__name__:
   assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40),module.__name__
 regs=('nc_node','nc_i','nc_count','nc_mode','nc_support','nc_kind','nc_depth','nc_width','nc_unsigned','nc_alignment','nc_sig','nc_si','nc_cbcount','nc_cbmode','nc_wireid','nc_payloadcut','nc_aggnode','nc_aggindex','nc_aggcount','nc_aggend','nc_aggmaxalign','nc_aggtag','nc_aggstride','nc_offset','nc_storage','nc_childwidth','nc_childalign')
 rows=list(csv.DictReader((pathlib.Path(__file__).parent/'rules.tsv').open(),delimiter='\t'))
 profiles={r['profile'] for r in rows}
 assert profiles=={f'{os}/{arch}' for os in ('osx','lnx','win') for arch in ('arm64','x86_64')} and len(rows)==6
 policy={r['profile']:(int(r['integer_min_width']),int(r['homogeneous_fp_carrier_kind'])) for r in rows}
 assert all(r['rule']=='natural_scalar_union_v2' and int(r['mixed_width'])==8 for r in rows)
 assert all(lo in (1,8) and fp in (1,3) for lo,fp in policy.values())
 def word(p,r):return p.a(('COPYW','ms_value',r)).call('MS.write64')
 def constword(p,v):return word(p.a(('LDI','nc_v',v)),'nc_v')
 def blob(p,r,n):return p.a(('INPUSH',r),('SPAN2','nc_zero',n),('INPOP',))
 def field(p,index,out='nc_v',node='nc_node'):return p.a(('ALUI','mul','nc_key',node,16),('ALUI','add','nc_key','nc_key',index),('LDX',out,'nc_key',FIELDS))
 def check(name,index,value,nx,node='nc_node'):
  p=field(P(name),index,node=node);p.branch({1:nx},'NC.unsupported',[('LDI','nc_expect',value),('C64U','nc_v','nc_expect')])
 P('NC.unsupported').a(E.rej('not covered: native ABI carrier rule')).goto('DEAD')
 P('NC.targetfail').a(E.rej('not covered: native ABI target profile')).goto('DEAD')
 p=P('NC.START').a(('LDI','nc_zero',0),('LDI','nc_level',0),('LDI','nc_defs',0),('ALUI','add','nc_epoch','nc_epoch',1),('XLEN','nc_original_len'),('OLEN','nc_cut'),('SPAN2','nc_zero','nc_original_len'),('OCUT','nc_original','nc_cut'),('SBCLR',),*[('SBOUT',x) for x in b'\0cli/target'],('SBFIND','nc_target'),('BLEN','nc_targetlen','nc_target'),('INPUSH','nc_target')).goto('NC.profile')
 # Exact byte trie: no prefix, unknown suffix or absent-resource fallback.
 prefixes={b''}
 for profile in profiles:
  raw=profile.encode();prefixes.update(raw[:i] for i in range(1,len(raw)+1))
 names={p:'NC.profile'+('' if not p else '.'+p.hex()) for p in prefixes}
 for prefix in sorted(prefixes):
  if prefix.decode() in profiles:
   P(names[prefix]).a(('LDI','nc_intminimum',policy[prefix.decode()][0]),('LDI','nc_fpcarrier',policy[prefix.decode()][1]),('MARK','nc_pos')).branch({1:'NC.profileok'},'NC.targetfail',[('C64U','nc_pos','nc_targetlen')])
  else:
   choices={p[len(prefix)]:names[p] for p in prefixes if len(p)==len(prefix)+1 and p.startswith(prefix)}
   P(names[prefix]).a(('MARK','nc_pos')).branch({0:names[prefix]+'.read'},'NC.targetfail',[('C64U','nc_pos','nc_targetlen')])
   P(names[prefix]+'.read').a(('BYTE','nc_byte'),('ADV',)).branch(choices,'NC.targetfail',[('RLD','nc_byte')])
 P('NC.profileok').a(('INPOP',),('COPYW','ms_blob','nc_original'),('COPYW','ms_len','nc_original_len')).call('MS.canonical').a(('COPYW','nc_count','ms_count'),('COPYW','nc_mode','ms_mode'),('COPYW','nc_support','ms_support'),('COPYW','nc_name','ms_name'),('BLEN','nc_namelen','nc_name')).branch({1:'NC.unsupported'},'NC.index',[('CMPI','ms_variadic',1)])
 P('NC.index').a(('LDI','mg_side',0),('LDI','mg_base',0),('LDI','mg_next',0),('LDI','mg_framelevel',0),('INPUSH','ms_canon'),('COPYW','ms_limit','ms_canonlen')).call('MG.root').a(('COPYW','nc_root','mg_lastnode'),('INPOP',),('OLEN','nc_cut')).o('USLSIG2\n').goto('NC.header')
 p=P('NC.header');constword(p,1);word(p,'nc_namelen');blob(p,'nc_name','nc_namelen').a(('OUTW','nc_zero'),('LDI','nc_one',1),('OUTW','nc_one'),('OUTW','nc_zero'),('OUTW','nc_mode'));word(p,'nc_count').a(('LDI','nc_i',0)).goto('NC.loop')
 P('NC.loop').a(('ALUI','mul','nc_key','nc_root',1025),('ALU','add','nc_key','nc_key','nc_i'),('LDX','nc_node','nc_key',EDGES)).call('NC.type').branch({1:'NC.stored'},'NC.next',[('CMPI','nc_i',0)])
 P('NC.stored').a(('COPYW','ms_value','nc_count')).call('MS.write64').goto('NC.next')
 P('NC.next').a(('ALUI','add','nc_i','nc_i',1)).branch({2:'NC.carrierend'},'NC.loop',[('C64U','nc_i','nc_count')])
 P('NC.type').call('NC.push').call('NC.classify').call('NC.pop').ret()
 P('NC.push').a(('ALUI','add','nc_level','nc_level',1)).branch({2:'NC.unsupported'},'NC.pushfields',[('CMPI','nc_level',64)])
 p=P('NC.pushfields')
 for i,r in enumerate(regs):p.a(('ALUI','mul','nc_slot','nc_level',32),('ALUI','add','nc_slot','nc_slot',i),('STX','nc_slot',FRAME,r))
 p.ret()
 p=P('NC.pop')
 for i,r in enumerate(regs):p.a(('ALUI','mul','nc_slot','nc_level',32),('ALUI','add','nc_slot','nc_slot',i),('LDX',r,'nc_slot',FRAME))
 p.a(('ALUI','sub','nc_level','nc_level',1)).ret()
 P('NC.classify').call('NC.loadfields').branch({5:'NC.aggregatekind',4:'NC.callback',0:'NC.void',(1,2,3):'NC.plain'},'NC.unsupported',[('RLD','nc_kind')])
 p=P('NC.loadfields')
 for index,name in ((0,'nc_depth'),(3,'nc_kind'),(4,'nc_width'),(5,'nc_unsigned'),(6,'nc_alignment'),(7,'nc_tag')):field(p,index,name)
 p.ret()
 P('NC.plain').branch({1:'NC.plainkind'},'NC.unsupported',[('CMPI','nc_tag',0)])
 P('NC.plainkind').branch({2:'NC.pointer'},'NC.scalar',[('RLD','nc_kind')])
 P('NC.pointer').branch({1:'NC.unsupported'},'NC.pointerwidth',[('CMPI','nc_depth',0)])
 check('NC.pointerwidth',4,8,'NC.pointeralign');check('NC.pointeralign',6,8,'NC.pointerunsigned');check('NC.pointerunsigned',5,0,'NC.emit')
 P('NC.scalar').branch({1:'NC.scalarwidth'},'NC.unsupported',[('CMPI','nc_depth',0)])
 P('NC.scalarwidth').branch({1:'NC.integer',3:'NC.floating'},'NC.unsupported',[('RLD','nc_kind')])
 P('NC.integer').branch({(1,2,4,8):'NC.scalaralign'},'NC.unsupported',[('RLD','nc_width')])
 P('NC.floating').branch({(4,8):'NC.floatunsigned'},'NC.unsupported',[('RLD','nc_width')])
 check('NC.floatunsigned',5,0,'NC.scalaralign')
 P('NC.scalaralign').branch({1:'NC.emit'},'NC.unsupported',[('C64U','nc_width','nc_alignment')])
 P('NC.void').branch({1:'NC.voiddepth'},'NC.unsupported',[('CMPI','nc_i',0)])
 check('NC.voiddepth',0,0,'NC.voidalign');check('NC.voidalign',6,0,'NC.voidtag');check('NC.voidtag',7,0,'NC.voidunsigned');check('NC.voidunsigned',5,0,'NC.emit')
 # Natural aggregate framing: exact layout words are emitted unchanged.
 P('NC.aggregatekind').branch({1:'NC.aggstart',2:'NC.union',3:'NC.aggstart'},'NC.unsupported',[('RLD','nc_tag')])
 P('NC.aggstart').branch({1:'NC.aggunsigned'},'NC.unsupported',[('CMPI','nc_depth',0)])
 P('NC.aggunsigned').branch({1:'NC.aggalign'},'NC.unsupported',[('CMPI','nc_unsigned',0)])
 P('NC.aggalign').branch({(1,2,4,8):'NC.aggwidth'},'NC.unsupported',[('RLD','nc_alignment')])
 P('NC.aggwidth').branch({1:'NC.unsupported'},'NC.aggwidthbound',[('CMPI','nc_width',0)])
 P('NC.aggwidthbound').branch({2:'NC.unsupported'},'NC.aggcount',[('CMPI','nc_width',1048576)])
 field(P('NC.aggcount'),8,'nc_aggcount').branch({1:'NC.unsupported'},'NC.aggcountbound',[('CMPI','nc_aggcount',0)])
 P('NC.aggcountbound').branch({2:'NC.unsupported'},'NC.aggheader',[('CMPI','nc_aggcount',1024)])
 p=P('NC.aggheader').a(('COPYW','nc_aggnode','nc_node'),('COPYW','nc_aggtag','nc_tag'),('LDI','nc_aggindex',0),('LDI','nc_aggend',0),('LDI','nc_aggmaxalign',1))
 word(p,'nc_depth');constword(p,0);constword(p,0)
 for reg in ('nc_kind','nc_width','nc_unsigned','nc_alignment'):word(p,reg)
 p.a(('OUTW','nc_aggtag'),('OLEN','nc_payloadcut'));word(p,'nc_aggcount').branch({3:'NC.arraystride'},'NC.aggloop',[('RLD','nc_aggtag')])
 field(P('NC.arraystride'),9,'nc_aggstride').goto('NC.arraystridewrite');word(P('NC.arraystridewrite'),'nc_aggstride').goto('NC.aggloop')
 P('NC.aggloop').a(('ALUI','mul','nc_key','nc_aggnode',1025),('ALU','add','nc_key','nc_key','nc_aggindex'),('LDX','nc_child','nc_key',EDGES)).goto('NC.aggchildfields')
 p=P('NC.aggchildfields');field(p,3,'nc_v','nc_child');p.branch({1:'NC.unsupported'},'NC.aggchildsize',[('CMPI','nc_v',0)])
 p=field(P('NC.aggchildsize'),4,'nc_childwidth','nc_child');field(p,6,'nc_childalign','nc_child').branch({(1,2,4,8):'NC.aggchildextent'},'NC.unsupported',[('RLD','nc_childalign')])
 P('NC.aggchildextent').branch({1:'NC.unsupported'},'NC.aggchildbound',[('CMPI','nc_childwidth',0)])
 P('NC.aggchildbound').branch({2:'NC.unsupported'},'NC.aggchildtag',[('C64U','nc_childwidth','nc_width')])
 P('NC.aggchildtag').branch({3:'NC.arraylayout'},'NC.structlayout',[('RLD','nc_aggtag')])
 p=P('NC.structlayout')
 for attr,out in ((0,'nc_offset'),(1,'nc_bitoffset'),(2,'nc_bitwidth'),(3,'nc_storage')):p.a(('ALUI','mul','nc_key','nc_aggnode',1025),('ALU','add','nc_key','nc_key','nc_aggindex'),('ALUI','mul','nc_key','nc_key',4),('ALUI','add','nc_key','nc_key',attr),('LDX',out,'nc_key',LAYOUT))
 p.branch({1:'NC.structbitwidth'},'NC.unsupported',[('CMPI','nc_bitoffset',0)])
 P('NC.structbitwidth').branch({1:'NC.structstorage'},'NC.unsupported',[('CMPI','nc_bitwidth',0)])
 P('NC.structstorage').branch({1:'NC.structoffset'},'NC.unsupported',[('C64U','nc_storage','nc_childwidth')])
 P('NC.structoffset').a(('ALU','add','nc_expected','nc_aggend','nc_childalign'),('ALUI','sub','nc_expected','nc_expected',1),('ALU','div','nc_expected','nc_expected','nc_childalign'),('ALU','mul','nc_expected','nc_expected','nc_childalign')).branch({1:'NC.structwrite'},'NC.unsupported',[('C64U','nc_offset','nc_expected')])
 p=P('NC.structwrite')
 for reg in ('nc_offset','nc_zero','nc_zero','nc_storage'):word(p,reg)
 p.a(('ALU','add','nc_aggend','nc_offset','nc_childwidth')).goto('NC.aggmaxalign')
 P('NC.aggmaxalign').branch({2:'NC.aggsetalign'},'NC.aggchildemit',[('C64U','nc_childalign','nc_aggmaxalign')])
 P('NC.aggsetalign').a(('COPYW','nc_aggmaxalign','nc_childalign')).goto('NC.aggchildemit')
 P('NC.arraylayout').branch({1:'NC.arrayalignment'},'NC.unsupported',[('C64U','nc_aggstride','nc_childwidth')])
 P('NC.arrayalignment').branch({1:'NC.arrayextent'},'NC.unsupported',[('C64U','nc_alignment','nc_childalign')])
 P('NC.arrayextent').a(('ALU','mul','nc_expected','nc_aggcount','nc_aggstride')).branch({1:'NC.aggchildemit'},'NC.unsupported',[('C64U','nc_width','nc_expected')])
 P('NC.aggchildemit').a(('COPYW','nc_node','nc_child')).call('NC.type').branch({3:'NC.cbfinish'},'NC.aggnext',[('RLD','nc_aggtag')])
 P('NC.aggnext').a(('ALUI','add','nc_aggindex','nc_aggindex',1)).branch({1:'NC.structendalign'},'NC.aggloop',[('C64U','nc_aggindex','nc_aggcount')])
 P('NC.structendalign').branch({1:'NC.structendextent'},'NC.unsupported',[('C64U','nc_alignment','nc_aggmaxalign')])
 P('NC.structendextent').a(('ALU','add','nc_expected','nc_aggend','nc_aggmaxalign'),('ALUI','sub','nc_expected','nc_expected',1),('ALU','div','nc_expected','nc_expected','nc_aggmaxalign'),('ALU','mul','nc_expected','nc_expected','nc_aggmaxalign')).branch({1:'NC.cbfinish'},'NC.unsupported',[('C64U','nc_width','nc_expected')])
 # Union objects can occur in any fixed signature node. Their overlapping
 # members are checked exactly and never traversed as callback object values.
 P('NC.union').branch({1:'NC.uniondepth'},'NC.unsupported',[('CMPI','nc_support',0)])
 sequence=[(0,0,'depth'),(3,5,'kind'),(5,0,'unsigned'),(7,2,'tag')]
 for j,(index,value,key) in enumerate(sequence):check('NC.union'+key,index,value,'NC.union'+sequence[j+1][2] if j+1<len(sequence) else 'NC.unionwidth')
 P('NC.unionwidth').branch({(1,2,4,8):'NC.unionalign'},'NC.unsupported',[('RLD','nc_width')])
 P('NC.unionalign').branch({1:'NC.unionmembers'},'NC.unsupported',[('C64U','nc_width','nc_alignment')])
 field(P('NC.unionmembers'),8,'nc_members').branch({1:'NC.unsupported'},'NC.unionmembersbound',[('CMPI','nc_members',0)])
 P('NC.unionmembersbound').branch({2:'NC.unsupported'},'NC.unionmembersstart',[('CMPI','nc_members',1024)])
 P('NC.unionmembersstart').a(('LDI','nc_member',0),('LDI','nc_floatseen',0),('LDI','nc_intseen',0),('LDI','nc_maxwidth',0),('LDI','nc_fpwidth',0),('LDI','nc_fpmixed',0)).goto('NC.memberlayout')
 P('NC.memberlayout').a(('LDI','nc_attr',0)).goto('NC.memberattr')
 P('NC.memberattr').a(('ALUI','mul','nc_key','nc_node',1025),('ALU','add','nc_key','nc_key','nc_member'),('ALUI','mul','nc_key','nc_key',4),('ALU','add','nc_key','nc_key','nc_attr'),('LDX','nc_v','nc_key',LAYOUT)).branch({3:'NC.memberstorage'},'NC.memberzero',[('RLD','nc_attr')])
 P('NC.memberzero').branch({1:'NC.attrnext'},'NC.unsupported',[('CMPI','nc_v',0)])
 P('NC.memberstorage').a(('COPYW','nc_storage','nc_v')).goto('NC.attrnext')
 P('NC.attrnext').a(('ALUI','add','nc_attr','nc_attr',1)).branch({1:'NC.memberchild'},'NC.memberattr',[('CMPI','nc_attr',4)])
 P('NC.memberchild').a(('ALUI','mul','nc_key','nc_node',1025),('ALU','add','nc_key','nc_key','nc_member'),('LDX','nc_child','nc_key',EDGES)).goto('NC.memberdepth')
 check('NC.memberdepth',0,0,'NC.membertag',node='nc_child');check('NC.membertag',7,0,'NC.memberfields',node='nc_child')
 p=P('NC.memberfields')
 for index,out in ((3,'nc_mkind'),(4,'nc_mwidth'),(5,'nc_munsigned'),(6,'nc_malign')):field(p,index,out,node='nc_child')
 p.branch({1:'NC.memberinteger',3:'NC.memberfloating'},'NC.unsupported',[('RLD','nc_mkind')])
 P('NC.memberinteger').branch({(1,2,4,8):'NC.memberintegerseen'},'NC.unsupported',[('RLD','nc_mwidth')])
 P('NC.memberintegerseen').a(('ALUI','add','nc_intseen','nc_intseen',1)).goto('NC.membernatural')
 P('NC.memberfloating').branch({(4,8):'NC.memberfloatunsigned'},'NC.unsupported',[('RLD','nc_mwidth')])
 P('NC.memberfloatunsigned').branch({1:'NC.memberfpwidth'},'NC.unsupported',[('CMPI','nc_munsigned',0)])
 P('NC.memberfpwidth').branch({1:'NC.memberfpset'},'NC.memberfpcompare',[('CMPI','nc_fpwidth',0)])
 P('NC.memberfpset').a(('COPYW','nc_fpwidth','nc_mwidth')).goto('NC.memberfloatseen')
 P('NC.memberfpcompare').branch({1:'NC.memberfloatseen'},'NC.memberfpmixed',[('C64U','nc_fpwidth','nc_mwidth')])
 P('NC.memberfpmixed').a(('LDI','nc_fpmixed',1)).goto('NC.memberfloatseen')
 P('NC.memberfloatseen').a(('ALUI','add','nc_floatseen','nc_floatseen',1)).goto('NC.membernatural')
 P('NC.membernatural').branch({1:'NC.memberextent'},'NC.unsupported',[('C64U','nc_mwidth','nc_malign')])
 P('NC.memberextent').branch({1:'NC.memberrootextent'},'NC.unsupported',[('C64U','nc_mwidth','nc_storage')])
 P('NC.memberrootextent').branch({2:'NC.unsupported'},'NC.membermaximum',[('C64U','nc_mwidth','nc_width')])
 P('NC.membermaximum').branch({2:'NC.membersetmax'},'NC.membernext',[('C64U','nc_mwidth','nc_maxwidth')])
 P('NC.membersetmax').a(('COPYW','nc_maxwidth','nc_mwidth')).goto('NC.membernext')
 P('NC.membernext').a(('ALUI','add','nc_member','nc_member',1)).branch({1:'NC.unioncomplete'},'NC.memberlayout',[('C64U','nc_member','nc_members')])
 P('NC.unioncomplete').branch({1:'NC.unionflavor'},'NC.unsupported',[('C64U','nc_maxwidth','nc_width')])
 P('NC.unionflavor').branch({1:'NC.unionintegeronly'},'NC.unionhasfloat',[('CMPI','nc_floatseen',0)])
 P('NC.unionintegeronly').branch({0:'NC.unsupported'},'NC.mapinteger',[('C64U','nc_width','nc_intminimum')])
 P('NC.unionhasfloat').branch({1:'NC.unionhomogeneous'},'NC.unionmixed',[('CMPI','nc_intseen',0)])
 P('NC.unionhomogeneous').branch({1:'NC.mapfloating'},'NC.unsupported',[('CMPI','nc_fpmixed',0)])
 P('NC.unionmixed').branch({1:'NC.mapinteger'},'NC.unsupported',[('CMPI','nc_width',8)])
 P('NC.mapfloating').a(('COPYW','nc_kind','nc_fpcarrier')).branch({1:'NC.mapinteger'},'NC.mapfloatfinish',[('CMPI','nc_kind',1)])
 P('NC.mapfloatfinish').a(('LDI','nc_unsigned',0)).goto('NC.mapped')
 P('NC.mapinteger').a(('LDI','nc_kind',1),('LDI','nc_unsigned',1)).goto('NC.mapped')
 P('NC.mapped').a(('LDI','nc_depth',0)).goto('NC.emit')
 callbackchecks=[(0,1,'depth'),(4,8,'width'),(5,0,'unsigned'),(6,8,'alignment'),(7,4,'tag'),(10,1,'edgecount')]
 P('NC.callback').goto('NC.callbackdepth')
 for j,(index,value,name) in enumerate(callbackchecks):check('NC.callback'+name,index,value,'NC.callback'+callbackchecks[j+1][2] if j+1<len(callbackchecks) else 'NC.cbemit')
 p=P('NC.cbemit');word(p,'nc_depth');constword(p,0);constword(p,0)
 for reg in ('nc_kind','nc_width','nc_unsigned','nc_alignment'):word(p,reg)
 p.a(('LDI','nc_v',4),('OUTW','nc_v'),('OLEN','nc_payloadcut'),('ALUI','mul','nc_key','nc_node',1025),('LDX','nc_sig','nc_key',EDGES),('LDX','nc_mark','nc_sig',SIGMARK)).branch({1:'NC.cbref'},'NC.cbdefine',[('C64U','nc_mark','nc_epoch')])
 p=P('NC.cbref').a(('LDI','nc_v',1),('OUTW','nc_v'),('OUTW','nc_v'),('LDX','nc_wireid','nc_sig',SIGWIRE));word(p,'nc_wireid').goto('NC.cbfinish')
 P('NC.cbdefine').a(('ALUI','add','nc_defs','nc_defs',1)).branch({2:'NC.unsupported'},'NC.cbregister',[('CMPI','nc_defs',1024)])
 p=P('NC.cbregister').a(('COPYW','nc_wireid','nc_defs'),('STX','nc_sig',SIGWIRE,'nc_wireid'),('STX','nc_sig',SIGMARK,'nc_epoch'),('LDI','nc_v',1),('OUTW','nc_v'),('OUTW','nc_zero'));word(p,'nc_wireid').goto('NC.cbvar')
 field(P('NC.cbvar'),0,node='nc_sig').branch({1:'NC.cbcount'},'NC.unsupported',[('CMPI','nc_v',0)])
 p=field(P('NC.cbcount'),4,'nc_cbcount','nc_sig');field(p,5,'nc_cbmode','nc_sig').a(('OUTW','nc_zero'),('OUTW','nc_cbmode'));word(p,'nc_cbcount').a(('LDI','nc_si',0)).goto('NC.cbloop')
 P('NC.cbloop').a(('ALUI','mul','nc_key','nc_sig',1025),('ALU','add','nc_key','nc_key','nc_si'),('LDX','nc_node','nc_key',EDGES),('COPYW','nc_i','nc_si')).call('NC.type').branch({1:'NC.cbstored'},'NC.cbnext',[('CMPI','nc_si',0)])
 P('NC.cbstored').a(('COPYW','ms_value','nc_cbcount')).call('MS.write64').goto('NC.cbnext')
 P('NC.cbnext').a(('ALUI','add','nc_si','nc_si',1)).branch({2:'NC.cbend'},'NC.cbloop',[('C64U','nc_si','nc_cbcount')])
 P('NC.cbend').a(('OUTW','nc_one')).goto('NC.cbfinish')
 p=P('NC.cbfinish').a(('OCUT','nc_payload','nc_payloadcut'),('BLEN','nc_payloadlen','nc_payload'));word(p,'nc_payloadlen');blob(p,'nc_payload','nc_payloadlen').ret()
 p=P('NC.emit');word(p,'nc_depth');constword(p,0);constword(p,0)
 for reg in ('nc_kind','nc_width','nc_unsigned','nc_alignment'):word(p,reg)
 p.a(('OUTW','nc_zero'));constword(p,0).ret()
 P('NC.carrierend').a(('OUTW','nc_one'),('OCUT','nc_carrier','nc_cut'),('BLEN','nc_carrierlen','nc_carrier')).o('USLNCAR1\n').goto('NC.envelope')
 p=P('NC.envelope');word(p,'nc_targetlen');blob(p,'nc_target','nc_targetlen');word(p,'nc_original_len');blob(p,'nc_original','nc_original_len');word(p,'nc_carrierlen');blob(p,'nc_carrier','nc_carrierlen').a(('ACCEPT',)).goto('NC.DONE')
 P('NC.DONE').a(('ACCEPT',)).goto('NC.DONE')
 return 'NC.START'

def build():
 spec=importlib.util.spec_from_file_location('nativecarrierbase',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
 start=install(E);E.g.finish()
 return {'start':start,'states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
if __name__=='__main__':
 d=build();pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')));print('native carrier states',len(d['states']))

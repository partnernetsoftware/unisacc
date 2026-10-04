#!/usr/bin/env python3
"""T2 pilot: assemble production manifest slices and check semantic contracts.

Recipe for another stage: select production rows (never synthesize their rules),
provide the same flags/facts, assert state+ordered actions independently, then
mutate one manifest operand privately and require the same assertion to fail.
Subgraphs need not be closed pipelines: no runtime or whole-graph hash oracle.
"""
import argparse
from pathlib import Path
import shutil
import sys
import tempfile
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT/'exec'))
sys.path.insert(0, str(ROOT/'exec/build'))
import assemble
import graph


def build(stage, mutation=False):
    with tempfile.TemporaryDirectory(prefix='manifest-entry-') as td:
        directory = Path(td)/stage
        directory.mkdir()
        for source_file in (ROOT/'exec'/stage).glob('*.tsv'):
            shutil.copyfile(source_file,directory/source_file.name)
        source = directory/('body-manifest.tsv' if stage=='pp' else 'gen-manifest.tsv')
        lines = source.read_text().splitlines()
        if stage=='pp':
            rows=[l for l in lines if l.startswith('rows\tdirective-scan\t')]
            assert len(rows)==1, 'pp manifest row missing or duplicated'
            if mutation:
                assert rows[0].count('TAKEB=TAKEB')==1
                rows[0]=rows[0].replace('TAKEB=TAKEB','TAKEB=SEENB',1)
            machine=graph.G()
        else:
            rows=[l for l in lines if l.startswith('call\toutput\t') or l.startswith('template\tgen\tdispatch\t')]
            assert len(rows)==2, 'lex manifest rows missing or duplicated'
            if mutation:
                import json
                output=directory/'output-manifest.tsv'
                output_lines=output.read_text().splitlines()
                indices=[i for i,line in enumerate(output_lines) if line.startswith('let\t') and '!typed&!positions&!locations' in line]
                assert len(indices)==1
                index=indices[0];cells=output_lines[index].split('\t');options=json.loads(cells[8])
                assert options['mapseq']['scan.start'][0]['acts']==[['MARK','S']]
                options['mapseq']['scan.start'][0]['acts']=[['LDI','S',0]]
                cells[8]=json.dumps(options);output_lines[index]='\t'.join(cells)
                output.write_text('\n'.join(output_lines)+'\n')
            machine=graph.Delta()
        manifest=directory/'entry-test-manifest.tsv'
        manifest.write_text('\n'.join(rows)+'\n')
        executor=SimpleNamespace(g=machine,P=None)
        assemble.run(manifest,executor,None,dict.fromkeys(('typed','positions','locations','sourcefacts'),False),{})
        return machine


def edge(machine,state,key):
    _,rows=machine.st[state]
    target,index=rows[key]
    return target,list(machine.seqs[index])


def check_pp(machine):
    facts=assemble.load_facts('pp-layout')
    assert facts['TAKEB']!=facts['SEENB'], 'mutation must change a domain offset'
    assert edge(machine,'LIVEL',0)==('LIVE2',[('ALUI','add','a','K',facts['TAKEB']),('LDX','t','a',0),('RLD','t')]), 'pp live-depth uses TAKEB'
    for key in (1,2):
        assert edge(machine,'LIVEL',key)==('P3WS',[]), 'pp already resolved depth'
    assert edge(machine,'D_ERROR',0)==('P3BLANK',[('JUMP','LS')]), 'pp inactive #error is skipped'
    for key in range(1,257):
        assert edge(machine,'D_ERROR',key)==('ERR.len',[('ALU','sub','t','LE','NS'),('CMPI','t',283)]), 'pp active #error reaches length guard'
    assert edge(machine,'ERR.len',2)==('DEAD',[('REJECT','not covered: #error text longer than 283 bytes')]), 'pp error text bound rejects'
    assert edge(machine,'DIAG.w2',1)==('DEAD',[('REJECT','not covered: a preprocessing diagnostic inside an included header')]), 'pp included diagnostic boundary rejects'
    return 6


def check_lex(machine):
    facts=assemble.load_facts('lex-gen')
    for key in facts['classes']['ws']:
        assert edge(machine,'DISPATCH',key)==('DISPATCH',[('ADV',)]), 'lex whitespace advances'
    for key in facts['classes']['A']:
        target,actions=edge(machine,'DISPATCH',key)
        assert target.startswith('ID') and actions==[('MARK','S'),('ADV',)], 'lex identifier class begins identifier'
    for key in facts['classes']['d']:
        target,actions=edge(machine,'DISPATCH',key)
        assert target==('NZ' if key==ord('0') else 'DEC') and actions==[('MARK','S'),('ADV',)], 'lex decimal class selects zero/decimal path'
    for key in facts['classes']['other']:
        assert edge(machine,'DISPATCH',key)==('HALT',[('REJECT','unexpected character')]), 'lex invalid bytes reject'
    assert edge(machine,'DISPATCH',256)==('CNT0',[('OUT',c) for c in b'eof\n']+[('INC','NT')]), 'lex EOF token spelling'
    return 5



def change_rule(directory, specification):
    """Mutate a source row, never the assembled graph or shared checkout."""
    filename, section, state, observation, column, old, new = specification
    path=directory/filename;lines=path.read_text().splitlines();hits=0
    for index,line in enumerate(lines):
        cells=line.split('\t')
        unsectioned=section==''
        if unsectioned: cells=['']+cells
        if len(cells)!=5 or cells[:3]!=[section,state,observation]: continue
        hits+=1
        if column=='target':
            assert cells[3]==old
            cells[3]=new
        else:
            assert cells[4].count(old)==1,(state,old)
            cells[4]=cells[4].replace(old,new,1)
        lines[index]='\t'.join(cells[1:] if unsectioned else cells)
    assert hits==1, specification
    path.write_text('\n'.join(lines)+'\n')


def extended_build(stage, mutation=None):
    import procs
    folder='enc' if stage=='enc/arm' else stage
    with tempfile.TemporaryDirectory(prefix='manifest-entry-') as td:
        directory=Path(td)/folder;directory.mkdir()
        for source in (ROOT/'exec'/folder).glob('*.tsv'):
            shutil.copyfile(source,directory/source.name)
        source_name={'parse2':'truth-manifest.tsv','enc':'gen-manifest.tsv',
                     'enc/arm':'arm-manifest.tsv','lower':'data-manifest.tsv'}[stage]
        lines=(directory/source_name).read_text().splitlines()
        if stage=='parse2':
            rows=[line for line in lines if line.startswith('rows\tscalar\toutput-')]
            assert len(rows)==5
            if mutation:
                name,old,new=mutation;hits=0
                for index,line in enumerate(rows):
                    if 'entry=@str:'+name+'\t' in line:
                        assert line.count(old)==1;rows[index]=line.replace(old,new,1);hits+=1
                assert hits==1
            env={};flags={}
        else:
            prefix={'enc':'rows\tx86-procs\tprocs\t',
                    'enc/arm':'rows\tarmcontract\tscan\t',
                    'lower':'rows\tdata\t-\t!win\t'}[stage]
            rows=[line for line in lines if line.startswith(prefix)]
            assert len(rows)==1
            if mutation: change_rule(directory,mutation)
            env={'target':'lnx/x86_64','os':'lnx','done':'ACCEPTDATA','code_start':'H.code'} if stage=='lower' else {}
            flags={'win':False} if stage=='lower' else {}
        machine=graph.G();out=lambda text:[('OUT',c) for c in text.encode()]
        executor=SimpleNamespace(g=machine,O=out,rej=lambda reason:[('REJECT',reason)])
        executor.P=procs.make_P(machine,out,executor.rej)
        manifest=directory/'entry-test-manifest.tsv';manifest.write_text('\n'.join(rows)+'\n')
        assemble.run(manifest,executor,executor.P,flags,env)
        return machine


def expect(machine,state,target,actions,key=0):
    assert edge(machine,state,key)==(target,actions),state


def output_text(machine,state,text,tail):
    expect(machine,state,'RET',[('OUT',c) for c in text.encode()]+tail)


def extended_contracts(stage):
    # Each contract has its own valid source mutation; expected values are not
    # read from the production action rows being checked.
    if stage=='parse2':
        f=assemble.load_facts('k2-truth');contracts=[]
        for state,operation,truth in [('FT.double','TRUTH64_OP',True),('FT.float','TRUTH32_OP',True),
                                      ('FN.double','NOT64_OP',False),('FN.float','NOT32_OP',False),('FN.integer','NOTINT_OP',False)]:
            text='  imm r1, 0\n  '+f[operation]+' r0, r0, r1\n'
            if truth: text+='  imm r1, 1\n  xor64 r0, r0, r1\n'
            tail=[('LDI','vb',4)] if truth else [('LDI','vt',0),('LDI','vb',4)]
            contracts.append((state,lambda g,st=state,t=text,a=tail:output_text(g,st,t,a),
                              (state,'imm r1\\x2c 0','imm r1\\x2c 1')))
        return contracts
    if stage=='enc':
        def rex(g): expect(g,'REX','RET',[('ALUI','shl','rx_t','rx_w',3),('ALUI','or','rx_t','rx_t',64),('ALUI','sar','rx_u','rx_r',3),('ALUI','shl','rx_u','rx_u',2),('ALU','or','rx_t','rx_t','rx_u'),('ALUI','sar','rx_u','rx_b',3),('ALU','or','rx_t','rx_t','rx_u'),('OUTW','rx_t')])
        def modrm(g): expect(g,'MODRM','RET',[('ALUI','shl','rx_t','mr_m',6),('ALUI','and','rx_u','mr_r',7),('ALUI','shl','rx_u','rx_u',3),('ALU','or','rx_t','rx_t','rx_u'),('ALUI','and','rx_u','mr_b',7),('ALU','or','rx_t','rx_t','rx_u'),('OUTW','rx_t')])
        def alu(g):
            target,actions=edge(g,'ALU',0)
            assert target=='REX' and actions[:3]==[('LDI','rx_w',1),('COPYW','rx_r','al_s'),('COPYW','rx_b','al_d')], 'ALU'
            assert len(actions)==4 and actions[3][0]=='PUSH' and actions[3][1] in g.st,'ALU continuation'
        return [
            ('REX',rex,('x86-procs-result.tsv','procs','REX','*','actions','"rx_w",3','"rx_w",4')),
            ('MODRM',modrm,('x86-procs-result.tsv','procs','MODRM','*','actions','"mr_m",6','"mr_m",5')),
            ('ALU',alu,('x86-procs-result.tsv','procs','ALU','*','actions','"rx_w",1','"rx_w",0')),
            ('LB.o',lambda g:expect(g,'LB.o','LB.l',[('OUTW','lb_v'),('A64I','shr','lb_v','lb_v',8),('ALUI','sub','lb_n','lb_n',1)]),('x86-procs-result.tsv','procs','LB.o','*','actions','"lb_v",8','"lb_v",7')),
            ('ME.66',lambda g:expect(g,'ME.66','ME.rex',[('LDI','ob',102),('OUTW','ob')]),('x86-procs-result.tsv','procs','ME.66','*','target','ME.rex','RET'))]
    if stage=='enc/arm':
        f=assemble.load_facts('enc-arm')
        def reg(g):
            target,actions=edge(g,'REG.value',0)
            assert target in g.st and g.st[target][0]=='r' and actions==[('LDX','v','t',f['REG']),('CMPI','v',0)],'REG.value'
        def overflow(g):
            _,actions=edge(g,'NUM.digit',0)
            assert actions==[('LDI','limit',1844674407370955161),('C64U','v','limit')],'NUM.digit'
        return [
            ('header',lambda g:expect(g,'LINE','HDR.key',[('MARK','hs'),('ADV',)],64),('armcontract-byte.tsv','scan','LINE','64','actions','"ADV"','"COPY"')),
            ('eof',lambda g:expect(g,'LINE','FINISH',[],256),('armcontract-byte.tsv','scan','LINE','256','target','FINISH','FAIL')),
            ('bad-number',lambda g:expect(g,'NUM.first','FAIL',[],ord('x')),('armcontract-byte.tsv','scan','NUM.first','*','target','FAIL','ENC')),
            ('REG.value',reg,('armcontract-result.tsv','scan','REG.value','*','actions','"constant","REG"','"constant","OP"')),
            ('NUM.digit',overflow,('armcontract-result.tsv','scan','NUM.digit','*','actions','1844674407370955161','0'))]
    f=assemble.load_facts('lower-data')
    def limit(g):
        target,actions=edge(g,'START',0)
        assert target=='LINE' and actions[:4]==[('LDI','dn',0),('LDI','ns',0),('LDI','rn',0),('LDI','data_limit',f['posix_limit'])],'START limit'
    def definition(g):
        _,actions=edge(g,'D.ne',0)
        assert actions==[('INTERN','ni','nstart','nend'),('LDX','t','ni',f['DEFINED']),('CMPI','t',0)],'D.ne'
    return [
        ('string-kind',lambda g:expect(g,'D.str','D.ws',[('LDI','is_bss',0)]),('data-result.tsv','D.str','*','actions','"is_bss",0','"is_bss",1')),
        ('bss-kind',lambda g:expect(g,'D.bss','D.ws',[('LDI','is_bss',1)]),('data-result.tsv','D.bss','*','actions','"is_bss",1','"is_bss",0')),
        ('copy-line',lambda g:expect(g,'COPYLINE','CL',[('JUMP','line')]),('data-result.tsv','COPYLINE','*','target','CL','SKIP')),
        ('limit',limit,('data-result.tsv','START','*','actions','"constant","data_limit"','"constant","extra"')),
        ('definition',definition,('data-result.tsv','D.ne','*','actions','"constant","DEFINED"','"constant","RAW"')),
        ('reject',lambda g:expect(g,'FAIL','DEAD',[('REJECT','not covered: tape data directive')]),('data-result.tsv','FAIL','*','target','DEAD','RET'))]


def check_extended(stage):
    started=time.monotonic();contracts=extended_contracts(stage)
    original=extended_build(stage)
    for name,check,mutation in contracts:
        check(original)
        if stage=='lower':
            filename,state,observation,column,old,new=mutation
            mutation=(filename,'',state,observation,column,old,new)
        mutant=extended_build(stage,mutation)
        try: check(mutant)
        except AssertionError: pass
        else: raise AssertionError((stage,name,'semantic mutation survived'))
    elapsed=time.monotonic()-started
    assert elapsed<10,(stage,elapsed)
    print(f'manifest entries {stage}: {len(contracts)} contracts, {len(contracts)} semantic mutations caught, {elapsed:.3f}s')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--stage',choices=('pp','lex','parse2','enc','enc/arm','lower'),required=True)
    stage=parser.parse_args().stage
    if stage not in ('pp','lex'): return check_extended(stage)
    started=time.monotonic()
    check=check_pp if stage=='pp' else check_lex
    count=check(build(stage))
    # Assembly must succeed for the mutant; only the semantic assertion may
    # kill it. A syntax error would not demonstrate an independent oracle.
    mutant=build(stage,mutation=True)
    try: check(mutant)
    except AssertionError as error:
        expected='pp live-depth uses TAKEB' if stage=='pp' else 'lex identifier class begins identifier'
        assert str(error)==expected, (stage,str(error))
    else: raise AssertionError('manifest mutation survived')
    elapsed=time.monotonic()-started
    assert elapsed<10, ('entry test exceeded 10 seconds',elapsed)
    print(f'manifest entries {stage}: {count} contracts, semantic mutation caught, {elapsed:.3f}s')


if __name__=='__main__': main()

#!/usr/bin/env python3
"""Experimental conservative original-span reference; disabled by default.
Uses positive prologue evidence; unknown/ambiguous/reachable callr => exact input.
"""
import copy, hashlib, re
from pathlib import Path
from .tape import SHAPE, REGS, _strip_comment, _regroup
ROOT=Path(__file__).resolve().parent.parent
NAME=re.compile(r'[A-Za-z_][A-Za-z0-9_.$]*\Z')


def prune_text_with_receipt(raw):
    if not isinstance(raw,bytes):raise ValueError('bytes required')
    if len(raw)>2*1024*1024 or any(c in raw for c in (b'\r',b'\v',b'\f')):
        digest=hashlib.sha256(raw).hexdigest()
        return raw,dict(input_sha256=digest,output_sha256=digest,input_bytes=len(raw),
            output_bytes=len(raw),removed_bytes=0,fallback_reasons=['experimental capacity or non-LF input'])
    original=[line+b'\n' for line in raw.split(b'\n')[:-1]]+([raw.split(b'\n')[-1]] if not raw.endswith(b'\n') else []);rows=[];labels={};data_names=set();reasons=[];pc=0
    for i,line in enumerate(original):
        text=_strip_comment(line.decode('latin-1')).strip();row={'line':i,'text':text,'kind':'other','pc':pc}
        if not text:pass
        elif re.match(r'\.bss[ \t]',text):
            parts=text.split()
            if len(parts)!=3 or not NAME.fullmatch(parts[1]) or not parts[2].isdigit() or int(parts[2])>32*1024*1024:
                reasons.append('unrecognized .bss')
            else:row.update(kind='data',name=parts[1]);data_names.add(parts[1])
        elif re.match(r'\.str[ \t]',text):
            m=re.fullmatch(r'\.str[ \t]+([A-Za-z_][A-Za-z0-9_.$]*)[ \t]+"(?:\\.|[^"\\])*"',text)
            if not m:reasons.append('unrecognized .str')
            else:row.update(kind='data',name=m[1]);data_names.add(m[1])
        elif text.endswith(':') and NAME.fullmatch(text[:-1]):
            name=text[:-1];row.update(kind='label',name=name)
            if name in labels:reasons.append('duplicate label '+name)
            else:labels[name]=i
        else:
            parts=text.replace('[',' ').replace(']',' ').replace(',',' ').split();op=parts[0]
            if op not in SHAPE:reasons.append('unknown opcode/directive '+op)
            else:
                args=_regroup(op,parts[1:])
                valid=len(args)==len(SHAPE[op])
                if valid:
                    for kind,arg in zip(SHAPE[op],args):
                        if kind=='r':valid &= arg in REGS
                        elif kind=='i':
                            valid &= bool(re.fullmatch(r'[+-]?(?:0[xX][0-9a-fA-F]+|[0-9]+)',arg))
                            try:
                                value=int(arg,0)
                                valid &= -(1<<63)<=value<(1<<63)
                            except ValueError:valid=False
                        else:valid &= bool(NAME.fullmatch(arg)) or bool(re.fullmatch(r'[+-]?(?:0[xX][0-9a-fA-F]+|[0-9]+)',arg))
                        if kind!='r' and re.fullmatch(r'[+-]?(?:0[xX][0-9a-fA-F]+|[0-9]+)',arg):
                            try:valid &= -(1<<63)<=int(arg,0)<(1<<63)
                            except ValueError:valid=False
                if valid and op in ('.ld','.st'):valid &= int(args[3],0) in (1,2,4,8)
                if valid and op=='.hostaddr':valid &= 0<=int(args[1],0)<4
                if op in ('.hostaddr','.hostcall'):reasons.append('host ABI requires target validation')
                if not valid:reasons.append('unrecognized operands '+op)
                else:
                    args=[str(int(arg,0)) if kind=='i' else arg for kind,arg in zip(SHAPE[op],args)]
                    row.update(kind='instruction',op=op,args=args);pc+=1
        rows.append(row)
    referenced=set(labels)|data_names
    for row in rows:
        if row['kind']=='instruction':
            referenced.update(arg for kind,arg in zip(SHAPE[row['op']],row['args']) if kind in ('L','s') and NAME.fullmatch(arg))
    if len(rows)>32768 or pc>=32768 or len(referenced)>8192:
        digest=hashlib.sha256(raw).hexdigest()
        return raw,dict(input_sha256=digest,output_sha256=digest,input_bytes=len(raw),
            output_bytes=len(raw),removed_bytes=0,fallback_reasons=['experimental row/name capacity'])
    if data_names.intersection(labels):reasons.append('code/data symbol ambiguity')
    by_pc={}
    for row in rows:
        if row['kind']=='label':by_pc.setdefault(row['pc'],[]).append(row)
    prologues=set()
    for name,i in labels.items():
        following=[]
        for row in rows[i+1:]:
            if row['kind']=='label':continue
            if row['kind']=='instruction':following.append(row)
            elif row['kind']=='data':break
            if len(following)==4:break
        if len(following)==4:
            shape=[(r['op'],r['args']) for r in following]
            if shape[:3]==[('.frame',['8']),('store64',['r7','0','r6']),('mov',['r6','r7'])] and shape[3][0]=='.frame' and int(shape[3][1][0],0)>=0:
                prologues.add(name)
    # Alias names at the same original instruction pc map to one owner.
    anchors=set(prologues)|({name for name in ['_start','main','__init','__main_ret'] if name in labels})
    for row in rows:
        if row['kind']=='instruction' and row['op']=='call':
            name=row['args'][0]
            if name not in labels:reasons.append('missing call target '+name)
            else:anchors.add(name)
    start_lines={min(r['line'] for r in by_pc[rows[labels[n]]['pc']]) for n in anchors}
    start_lines.add(0);starts=sorted(start_lines);owner_of=[0]*len(rows);units=[]
    for owner,start in enumerate(starts):
        end=starts[owner+1] if owner+1<len(starts) else len(rows)
        unit={'start':start,'end':end,'names':[],'functions':[],'edges':set(),'callr':False}
        for i in range(start,end):
            owner_of[i]=owner
            if rows[i]['kind']=='label':
                name=rows[i]['name'];unit['names'].append(name)
                if name in prologues:unit['functions'].append(name)
        units.append(unit)
    name_owner={n:owner_of[i] for n,i in labels.items()};roots=set()
    if rows:
        root_name='_start' if '_start' in labels else None
        first_instruction=next((r['line'] for r in rows if r['kind']=='instruction'),None)
        if root_name:roots.add(name_owner[root_name])
        elif first_instruction is not None:roots.add(owner_of[first_instruction])
    for name in ['main','__init','__main_ret']:
        if name in name_owner:roots.add(name_owner[name])
    for row in rows:
        if row['kind']!='instruction':continue
        owner=owner_of[row['line']];op,args=row['op'],row['args']
        if op in ['call','jump','jumpz']:
            target=args[-1]
            if target not in name_owner:reasons.append('missing code target '+target)
            else:units[owner]['edges'].add(name_owner[target])
        elif op=='.lea':
            target=args[1]
            if target in name_owner:roots.add(name_owner[target])
            elif target not in data_names:
                try:int(target,0)
                except ValueError:reasons.append('unresolved address '+target)
        if op in ('callr','callm'):units[owner]['callr']=True
    for owner,unit in enumerate(units[:-1]):
        code=[r for r in rows[unit['start']:unit['end']] if r['kind']=='instruction']
        if not code or code[-1]['op'] not in ['ret','jump','.exit']:unit['edges'].add(owner+1)
    alive=set(roots);queue=list(sorted(roots));at=0
    while at<len(queue):
        owner=queue[at];at+=1
        if units[owner]['callr']:reasons.append('reachable callr: unknown pointer provenance');break
        for target in sorted(units[owner]['edges']):
            if target not in alive:alive.add(target);queue.append(target)
    if reasons:alive=set(range(len(units)))
    keep=[i for i,r in enumerate(rows) if r['kind']=='data' or owner_of[i] in alive]
    out=b''.join(original[i] for i in keep)
    offsets=[];pos=0
    for line in original:offsets.append(pos);pos+=len(line)
    spans=[(offsets[i],offsets[i]+len(original[i])) for i in keep]
    functions=sorted(prologues);removed=sorted(n for n in functions if name_owner[n] not in alive)
    data_indices=[i for i,r in enumerate(rows) if r['kind']=='data']
    assert all(i in keep for i in data_indices)
    assert out==b''.join(raw[a:b] for a,b in spans)
    if reasons:assert out==raw
    receipt={'prototype_only':True,'input_sha256':hashlib.sha256(raw).hexdigest(),'output_sha256':hashlib.sha256(out).hexdigest(),
             'input_bytes':len(raw),'output_bytes':len(out),'removed_bytes':len(raw)-len(out),'functions_before':len(functions),
             'functions_after':len(functions)-len(removed),'removed_functions':removed,'retained_functions':sorted(set(functions)-set(removed)),
             'units_before':len(units),'units_after':len(alive),
             'retained_nonprologue_entries':[units[o]['names'][0] for o in sorted(alive) if units[o]['names'] and not units[o]['functions']],
             'tape_schema_sha256':hashlib.sha256((ROOT/'unisa/tape.py').read_bytes()).hexdigest(),
             'fallback_reasons':sorted(set(reasons)),'roots':[units[o]['names'][0] if units[o]['names'] else '<prefix>' for o in sorted(roots)],
             'data_lines_preserved':len(data_indices),'kept_original_line_indices':keep,'original_byte_spans':spans,
             'data_sha256':hashlib.sha256(b''.join(original[i] for i in data_indices)).hexdigest()}
    return out,receipt


def prune(tape):
    """Analyze text spans, then retain original instruction/data objects and PCs.

    Serialization is only the analysis input; data, symbols and relocations are
    copied from the original Tape rather than reparsed or repacked.
    """
    raw=tape.to_text().encode('latin-1')
    if len(raw)>2*1024*1024:
        return tape
    output, receipt=prune_text_with_receipt(raw)
    if output==raw:
        return tape
    kept=set(receipt['kept_original_line_indices']); pcs=[]; labels=set(); pc=0
    for line_index,line in enumerate(raw.splitlines()):
        if line.startswith(b'  '):
            if line_index in kept:pcs.append(pc)
            pc+=1
        elif line.endswith(b':') and line_index in kept:
            labels.add(line[:-1].decode('latin-1'))
    assert pc==len(tape.code)
    mapping=[0]*(pc+1); retained=set(pcs); newpc=0
    for oldpc in range(pc):
        mapping[oldpc]=newpc
        if oldpc in retained:newpc+=1
    mapping[pc]=newpc
    result=copy.copy(tape)
    result.code=[tape.code[i] for i in pcs]
    result.labels={name:mapping[oldpc] for name,oldpc in tape.labels.items() if name in labels}
    result.data=tape.data;   # shared, read-only from here on (a copy cost 600 MB on the compiler's own tape)
    result.syms=dict(tape.syms);result.relocs=list(tape.relocs)
    return result


def prune_text(raw):
    """Return original-span bytes; unsupported or over-capacity input stays whole."""
    return prune_text_with_receipt(raw)[0]

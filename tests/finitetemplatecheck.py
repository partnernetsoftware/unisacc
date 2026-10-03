#!/usr/bin/env python3
"""finite_rules templates: call chains across loops ({fresh@row}/{prev}, dotted `over`)
and rename rewriting PUSH arguments.  Synthetic graph; no stage involved."""
import importlib.util,pathlib,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path[:0]=[str(ROOT/'exec')]
import finite_rules as F
spec=importlib.util.spec_from_file_location('fr_lexgen',ROOT/'exec/parse/gen.py');M=importlib.util.module_from_spec(spec);spec.loader.exec_module(M)
n=[0]
def fresh(k):n[0]+=1;return f'T.{k}{n[0]}'
rows=['# section\tblock\teach\tover\tkind\ta\tb\tc\td',
 'c\tb\tgrp\t-\trule\tS{grp.g}\t*\tW\t[["PUSH","{fresh@row:R:r}"]]',
 'c\tb\tgrp\tel:grp.els\trule\t{prev:R}\t*\tW\t[["LDI","x",{el}],["PUSH","{fresh@row:R:r}"]]',
 'c\tb\tgrp\t=\trule\t{prev:R}\t*\tW\t[["PUSH","{fresh@row:R:r}"]]',
 'c\tb\tgrp\t-\trule\t{prev:R}\t*\tEND\t[]',
 'n\tn\t-\tQ\trule\tN{Q.k}\t*\tW\t[["PUSH","{fresh@row:R:r}"]]',
 'n\tn\t-\t.e:Q.e\trule\t{prev:R}\t*\tW\t[["LDI","x",{e}],["PUSH","{fresh@row:R:r}"]]',
 'n\tn\t-\t=\trule\t{prev:R}\t*\tEND{Q.k}\t[]',
 'r\tr\t-\t-\trename\tW\tW.original\t\t',
 'e\te\t-\t-\tfill-edge\tE\t1\tIGNORED\t[]',
 'e\te\t-\t-\tfill-edge\tE\t2\tC\t[]',
 'e\te\t-\t-\tinsert-edge\tE\t3\tD\t[]',
 'e\te\t-\t-\tdrop-edge\tE\t0\t\t',
 'e\te\t-\t-\tset-mode\tE\tr\tb\t',
 'u\tu\t-\t-\tinsert-edge\tE\t1\tBAD\t[]',
 'm\tm\t-\t-\tcopy-state\tE\tE.copy\t\t',
 'm\tm\t-\t-\tmove-state\tE\tE.moved\t\t',
 'p\tp\t-\t-\tclone-push\tR\tR.copy\tW\tCONT',
 'q\tq\t-\t-\tcopy\tE.dest\tE.copy\t\t']
with tempfile.TemporaryDirectory() as d:
 (pathlib.Path(d)/'t-template.tsv').write_text('\n'.join(rows)+'\n')
 facts=dict(grp=[dict(g=1,els=[7,8]),dict(g=2,els=[])],Q=[dict(k=1,e=[5]),dict(k=2,e=[6,9])])
 out,_,_=F.expand_template(pathlib.Path(d)/'t-template.tsv',facts,fresh,'c')
 got=[(l.split('\t')[0],l.split('\t')[3]) for l in out]
 assert got==[('S1','[["PUSH","T.r1"]]'),('T.r1','[["LDI","x",7],["PUSH","T.r2"]]'),('T.r2','[["PUSH","T.r3"]]'),
  ('T.r3','[["LDI","x",8],["PUSH","T.r4"]]'),('T.r4','[["PUSH","T.r5"]]'),('T.r5','[]'),('S2','[["PUSH","T.r6"]]'),('T.r6','[]')],got
 out,_,_=F.expand_template(pathlib.Path(d)/'t-template.tsv',facts,fresh,'n')
 assert [l.split('\t')[0]+'>'+l.split('\t')[2] for l in out]==['N1>W','T.r7>W','T.r8>END1','N2>W','T.r9>W','T.r10>W','T.r11>END2'],out
 g=type(M.g)();g.on('W',range(257),'X',[],'r');g.on('A',range(257),'W',[('PUSH','W'),('LDI','x',1)],'r');g.labels.add('W')
 F.install_template(g,d,'t',{},fresh,section='r')
 assert 'W' not in g.st and 'W.original' in g.st and 'W.original' in g.labels
 t,s=g.st['A'][1][0];assert t=='W.original' and list(g.seqs[s])==[('PUSH','W.original'),('LDI','x',1)],(t,g.seqs[s])
 h=type(M.g)();h.on('E',[0],'A',[],'r');h.on('E',[1],'B',[],'r')
 F.install_template(h,d,'t',{},fresh,section='e')
 assert h.st['E'][0]=='b' and {k:v[0] for k,v in h.st['E'][1].items()}=={1:'B',2:'C',3:'D'}
 try:F.install_template(h,d,'t',{},fresh,section='u')
 except ValueError as exc:assert 'duplicate edge E/1' in str(exc)
 else:raise AssertionError('duplicate insert accepted')
 h.on('SRC',[0],'E',[],'b')
 F.install_template(h,d,'t',{},fresh,section='m')
 assert 'E' not in h.st and h.st['E.copy'] is h.st['E.moved']
 assert h.st['SRC'][1][0][0]=='E'  # move is intentionally not a target rewrite
 F.install_template(h,d,'t',{},fresh,section='q')
 assert h.st['E.dest'] is h.st['E.copy']
 z=type(M.g)();z.on('R',[0,1],'W',[('MARK','tpos'),('PUSH','OLD')],'r')
 F.install_template(z,d,'t',{},fresh,section='p')
 assert 'CONT' in z.labels and z.st['R.copy'][0]=='r'
 assert z.st['R.copy'][1] is not z.st['R'][1]
 assert all(list(z.seqs[seq])==[('MARK','tpos'),('PUSH','CONT')]
            for _,seq in z.st['R.copy'][1].values())
print('finite template check ok')

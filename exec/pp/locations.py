"""Optional preprocessing diagnostic envelope, emitted by ordinary actions.
UNIPP1 NUL; five u32-le words (text length, forced lines, automatic lines,
splice count, include count); splice offsets; includes as (line, line count,
name length, name bytes); then exactly text-length preprocessed bytes.
"""
IRNAME = 1 << 40

def install(g, spl, irln, irnl):
    def out(s): return [('OUT', b) for b in s]
    def word(reg):
        a=[]
        for shift in (0,8,16,24):
            a += [('ALUI','sar','loc_byte',reg,shift),('OUTW','loc_byte')]
        return a
    g.els('ACC','LOC.splices', [('LDI','loc_zero',0),('OLEN','loc_len'),('OCUT','loc_text','loc_zero')]
        +out(b'UNIPP1\0')+sum((word(r) for r in ('loc_len','CLI_PRELINES','AI_LINES','NSPL','NIREG')),[])
        +[('LDI','loc_i',0),('CMP','loc_i','NSPL')])
    g.r('LOC.splices',{0:('LOC.splice',[]),(1,2):('LOC.includes',[('LDI','loc_i',0),('CMP','loc_i','NIREG')])})
    g.els('LOC.splice','LOC.splices',[('LDX','loc_v','loc_i',spl)]+word('loc_v')+
        [('ALUI','add','loc_i','loc_i',1),('CMP','loc_i','NSPL')])
    g.r('LOC.includes',{0:('LOC.include',[]),(1,2):('LOC.text',[('INPUSH','loc_text')])})
    g.els('LOC.include','LOC.name',[('LDX','loc_v','loc_i',irln)]+word('loc_v')+
        [('LDX','loc_v','loc_i',irnl)]+word('loc_v')+
        [('LDX','loc_name','loc_i',IRNAME),('BLEN','loc_v','loc_name')]+word('loc_v')+
        [('INPUSH','loc_name')])
    g.on('LOC.name',[256],'LOC.includes',[('INPOP',),('ALUI','add','loc_i','loc_i',1),('CMP','loc_i','NIREG')])
    g.els('LOC.name','LOC.name',[('COPYT',),('ADV',)])
    g.on('LOC.text',[256],'LOC.end',[('INPOP',),('ACCEPT',)])
    g.els('LOC.text','LOC.text',[('COPYT',),('ADV',)])
    g.els('LOC.end','LOC.end',[('REJECT','unreachable')])

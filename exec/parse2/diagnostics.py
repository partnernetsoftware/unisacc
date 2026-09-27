"""Reference diagnostic positioning/rendering compiled to ordinary actions.
DIAG.report takes diag_pos (preprocessed byte offset), diag_message (blob),
and diag_warning (0=error, 1=warning). It returns diag_reported=0 for header
warnings, otherwise 1, and restores the input frame and stdout selection.
All scratch registers and decimal digits are private to this routine.
"""
from tokenlocations import SPLICES,INCLUDE_LINE,INCLUDE_LINES,INCLUDE_NAME
DIGITS=39<<40


def install(E,P):
    g=E.g
    P('DIAG.report').a(('LDI','diag_reported',0),('LDI','dp_zero',0),
        ('COPYW','dp_p','diag_pos')).branch({0:'DP.clamp0'},'DP.upper',[('C64','dp_p','dp_zero')])
    P('DP.clamp0').a(('LDI','dp_p',0)).goto('DP.upper')
    P('DP.upper').branch({2:'DP.clampend'},'DP.scan0',[('C64','dp_p','diag_textlen')])
    P('DP.clampend').a(('COPYW','dp_p','diag_textlen')).goto('DP.scan0')
    P('DP.scan0').a(('INPUSH','diag_source'),('LDI','dp_i',0),('LDI','dp_start',0),
        ('LDI','dp_line',1),('A64','sub','dp_line','dp_line','diag_forced'),
        ('LDI','dp_col',1)).goto('DP.scan')
    P('DP.scan').branch({0:'DP.char'},'DP.endline',[('CMP','dp_i','dp_p')])
    g.on('DP.char',[10],'DP.advance',[('A64I','add','dp_line','dp_line',1),('LDI','dp_col',1),('ALUI','add','dp_start','dp_i',1)])
    g.els('DP.char','DP.advance',[('A64I','add','dp_col','dp_col',1)])
    P('DP.advance').a(('ADV',),('ALUI','add','dp_i','dp_i',1)).goto('DP.scan')
    g.on('DP.endline',[10,256],'DP.regions0',[('MARK','dp_end'),('INPOP',)])
    g.els('DP.endline','DP.endline',[('ADV',)])
    P('DP.regions0').a(('LDI','dp_inside',-1),('COPYW','dp_file','diag_filename'),
        ('ALUI','sub','dp_i','diag_ninclude',1)).goto('DP.regions')
    P('DP.regions').branch({0:'DP.outside'},'DP.region',[('CMPI','dp_i',0)])
    P('DP.region').a(('ALU','add','dp_slot','diag_mapbase','dp_i'),('LDX','dp_ln','dp_slot',INCLUDE_LINE),('LDX','dp_nl','dp_slot',INCLUDE_LINES),
        ('A64','add','dp_last','dp_ln','dp_nl')).branch({2:'DP.after'},'DP.within',[('C64','dp_line','dp_last')])
    P('DP.after').a(('A64','sub','dp_line','dp_line','dp_nl')).goto('DP.previous')
    P('DP.within').branch({(1,2):'DP.inside'},'DP.previous',[('C64','dp_line','dp_ln')])
    P('DP.inside').a(('COPYW','dp_inside','dp_i'),('LDX','dp_file','dp_slot',INCLUDE_NAME),
        ('A64','sub','dp_line','dp_line','dp_ln'),('A64I','add','dp_line','dp_line',1)).goto('DP.suppress')
    P('DP.previous').a(('ALUI','sub','dp_i','dp_i',1)).goto('DP.regions')
    P('DP.outside').a(('A64','sub','dp_line','dp_line','diag_auto'),('LDI','dp_i',0)).goto('DP.splices')
    P('DP.splices').branch({0:'DP.splice'},'DP.emit',[('CMP','dp_i','diag_nsplice')])
    P('DP.splice').a(('ALU','add','dp_slot','diag_mapbase','dp_i'),('LDX','dp_v','dp_slot',SPLICES)).branch({0:'DP.joined'},'DP.nextsplice',[('CMP','dp_v','dp_p')])
    P('DP.joined').a(('A64I','add','dp_line','dp_line',1)).goto('DP.nextsplice')
    P('DP.nextsplice').a(('ALUI','add','dp_i','dp_i',1)).goto('DP.splices')
    P('DP.suppress').branch({1:'RET'},'DP.emit',[('CMPI','diag_warning',1)])
    P('DP.emit').a(('LDI','diag_reported',1),('OSEL',1),('INPUSH','dp_file'),
        ('BLEN','dp_len','dp_file'),('SPAN2','dp_zero','dp_len'),('INPOP',)).o(':').a(
        ('COPYW','dp_num','dp_line')).call('DP.number').o(':').a(
        ('COPYW','dp_num','dp_col')).call('DP.number').branch({1:'DP.warning'},'DP.error',[('CMPI','diag_warning',1)])
    P('DP.warning').o(': warning: ').goto('DP.message')
    P('DP.error').o(': error: ').goto('DP.message')
    P('DP.message').a(('INPUSH','diag_message'),('BLEN','dp_len','diag_message'),
        ('SPAN2','dp_zero','dp_len'),('INPOP',)).o('\n  ').a(('INPUSH','diag_source'),
        ('SPAN2','dp_start','dp_end')).o('\n  ').a(('JUMP','dp_start'),('COPYW','dp_i','dp_start')).goto('DP.caret')
    P('DP.caret').branch({0:'DP.caretbyte'},'DP.done',[('CMP','dp_i','dp_p')])
    g.on('DP.caretbyte',[9],'DP.caretnext',[('OUT',9)])
    g.els('DP.caretbyte','DP.caretnext',[('OUT',32)])
    P('DP.caretnext').a(('ADV',),('ALUI','add','dp_i','dp_i',1)).goto('DP.caret')
    P('DP.done').o('^\n').a(('INPOP',),('OSEL',0)).ret()
    # en2 prints zero, positive decimal, and nothing for a negative line.
    P('DP.number').branch({0:'RET',1:'DP.numzero'},'DP.numstart',[('C64','dp_num','dp_zero')])
    P('DP.numzero').o('0').ret()
    P('DP.numstart').a(('LDI','dp_nd',0)).goto('DP.numdigits')
    P('DP.numdigits').a(('A64I','urem','dp_digit','dp_num',10),
        ('STX','dp_nd',DIGITS,'dp_digit'),('ALUI','add','dp_nd','dp_nd',1),
        ('A64I','udiv','dp_num','dp_num',10)).branch({1:'DP.numout'},'DP.numdigits',[('C64','dp_num','dp_zero')])
    P('DP.numout').a(('ALUI','sub','dp_nd','dp_nd',1),('LDX','dp_digit','dp_nd',DIGITS),
        ('ALUI','add','dp_digit','dp_digit',48),('OUTW','dp_digit')).branch({1:'RET'},'DP.numout',[('CMPI','dp_nd',0)])

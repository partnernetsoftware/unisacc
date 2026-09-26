"""Construct ordinary actions that read an optional little-endian u64 resource.
No executor primitive is added; absence and malformed length are distinct.
"""
def u64(E, label, key, result, present, fail):
    P=E.P
    p=P(label).a(('SBCLR',),[('SBOUT',c) for c in key],('SBFIND','mi_blob'),('COPYW',present,'mi_blob'),('LDI',result,0))
    p.branch({1:'RET'},label+'.length',[('CMPI','mi_blob',0)])
    P(label+'.length').a(('BLEN','mi_len','mi_blob')).branch({1:label+'.read'},fail,[('CMPI','mi_len',8)])
    p=P(label+'.read').a(('INPUSH','mi_blob'))
    for shift in range(0,64,8):
        p.a(('BYTE','mi_byte'),('A64I','shl','mi_byte','mi_byte',shift),('A64','or',result,result,'mi_byte'),('ADV',))
    p.a(('INPOP',)).ret()

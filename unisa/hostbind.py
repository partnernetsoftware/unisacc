"""LB-1 binding reference oracle. Host values never enter hostsymbols.produce.
Legacy loader capabilities only: no signature or actual function-type evidence.
"""
import struct

def bind(contract, target, kind, available, addresses, mask=15):
    def fail(reason):
        raise ValueError("unisacc: bind: " + reason)
    if (len(contract)<12 or contract[:8]!=b"UNISYM1\n" or contract[8] not in range(1,5)
            or contract[9] or contract[10]>4 or contract[11]>1
            or len(contract)!=12+8*contract[10]):
        fail("invalid-contract framing")
    previous=-1
    slots=[]
    for off in range(12,len(contract),8):
        identity,slot,symbol_kind,abi,reserved=struct.unpack_from("<IBBBB",contract,off)
        if slot>3 or slot<=previous or identity!=slot+1 or symbol_kind!=1 or abi or reserved:
            fail("invalid-contract record")
        previous=slot
        slots.append(slot)
    if target!=contract[8] or kind not in (1,2) or available & ~15 or mask & ~15 or len(addresses)!=4:
        fail("invalid-provider backend")
    for slot in slots:
        if not (available & mask & (1<<slot)) or (kind==2 and not addresses[slot]):
            fail("missing-provider symbol %d slot %d" % (slot+1,slot))
    return (target,kind,sum(1<<s for s in slots),tuple(addresses))

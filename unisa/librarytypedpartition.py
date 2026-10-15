"""Observation reference: opt-in V3 fixed-typed ownership, no provider state.
Modes 0/1 preserve old partition results. Mode bit0=data; bit1=fixed V3.
This stage validates envelope/opaque signature framing, not ABI qualification.
"""
from .librarycontract import produce as validate

def produce(frame: bytes) -> bytes:
    def fail(reason):
        raise ValueError("library partition: " + reason)
    if not isinstance(frame, bytes) or len(frame)<16 or frame[:8]!=b"USLPAR1\n" or frame[10] not in (0,1,2) or frame[11] not in (0,1,2,3):
        fail("malformed declaration stream")
    count=int.from_bytes(frame[12:16],"little")
    if count>4096:fail("malformed declaration stream")
    audit,mode=frame[10:12];at=16;owners=[];consumers=[]
    for i in range(count):
        if len(frame)-at<36:fail("malformed declaration stream")
        z=int.from_bytes(frame[at:at+4],"little")
        kind,callkind,sigv=frame[at+8:at+11]
        owner=1 if kind==0 and callkind==0 else 4 if kind==1 and mode&1 else 8 if kind==0 and callkind==1 and sigv==3 and mode&2 else 2
        consumers.append(owner);owners.append(1 if owner==8 else owner);at+=4+z
        if at>len(frame):fail("malformed declaration stream")
    try:validate(b"USLDEC1\n"+frame[8:10]+b"\0\0"+frame[12:at])
    except ValueError:fail("malformed declaration stream")
    claims=frame[at:]
    if audit:
        if len(claims)!=count:fail("malformed declaration stream")
        for claim,owner in zip(claims,consumers if audit==2 else owners):
            if claim==0:fail("missing-owner")
            if (audit==2 and claim in (3,5,6,7,9,10,11,12,13,14,15)) or (audit==1 and (claim==3 or mode&1 and claim in (5,6,7))):fail("duplicate-owner")
            if claim not in ((1,2,4,8) if audit==2 else (1,2,4)):fail("malformed declaration stream")
            if claim!=owner:fail("wrong-owner")
    elif claims:fail("malformed declaration stream")
    return b"USLSEL1\n"+count.to_bytes(4,"little")+b"".join((256+i).to_bytes(4,"little")+bytes((owner,)) for i,owner in enumerate(owners))

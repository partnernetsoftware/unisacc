"""Observation-only callback ownership overlay over the existing reference.

Mode0..7 delegates unchanged. In the explicitly installed callback model,
bit3 enables callkind3/V3, logical consumer32 on physical function owner1.
The projection below reuses the existing framing/audit implementation; opaque
nested signature bytes are not ABI evidence and provider state is never read.
"""
from .librarycallsitepartition import produce as previous


def produce(frame: bytes, *, librarycallback=False) -> bytes:
    if not librarycallback:
        return previous(frame)
    def fail(reason="malformed declaration stream"):
        raise ValueError("library partition: " + reason)
    if not isinstance(frame, bytes) or len(frame)<16 or frame[:8]!=b"USLPAR1\n" or frame[10] not in (0,1,2) or frame[11]>15:
        fail()
    if frame[11]<8:
        return previous(frame)
    mode=frame[11];audit=frame[10];count=int.from_bytes(frame[12:16],"little")
    if count>4096:fail()
    at=16;records=[]
    for _ in range(count):
        if len(frame)-at<36:fail()
        z=int.from_bytes(frame[at:at+4],"little")
        if z<32 or z>len(frame)-at-4:fail()
        records.append(at);at+=4+z
    # Validate all original core bytes first, reusing the predecessor's contract.
    raw=bytearray(frame[:at]);raw[10]=0;raw[11]=mode&7
    owners=previous(bytes(raw))
    claims=bytearray(frame[at:])
    if (audit and len(claims)!=count) or (not audit and claims):fail()
    for i,pos in enumerate(records):
        callback=frame[pos+8:pos+11]==bytes((0,3,3))
        if callback:raw[pos+9]=0  # existing physical function owner, opaque sig unchanged
        if not audit:continue
        claim=claims[i]
        if claim>=32:
            if audit!=2 or claim>=64:fail()
            if claim!=32:fail("duplicate-owner")
            # The predecessor verifies a registered but wrong consumer itself.
            claims[i]=1 if callback else (4 if owners[16+5*i]==1 else 1)
        elif callback and audit==2:
            # Do not let the projected ordinary owner accept a crossed claim.
            if claim in (1,2,4,8) or claim==16 and mode&4:claims[i]=2
    raw[10]=audit
    return previous(bytes(raw)+claims)


def _oracle(cases, output, *, librarycallback=False):
    import hashlib
    from pathlib import Path
    raw=Path(cases).read_bytes();rows=["#sha256\t"+hashlib.sha256(raw).hexdigest()]
    for line in raw.decode().splitlines():
        if line.startswith("#"):continue
        name,data,*_=line.split("\t")
        try:result=produce(bytes.fromhex(data),librarycallback=librarycallback);rc=0;reason=""
        except ValueError as error:result=b"";rc=1;reason=str(error)
        rows.append("\t".join((name,str(rc),result.hex(),reason)))
    Path(output).write_text("\n".join(rows)+"\n")


if __name__=="__main__":
    import sys
    args=sys.argv[1:];enabled=bool(args and args[0]=="--librarycallback")
    if enabled:args=args[1:]
    if len(args)!=3 or args[0]!="--oracle":
        raise SystemExit("usage: python3 -m unisa.librarycallbackpartition --oracle CASES LEDGER")
    _oracle(args[1],args[2],librarycallback=enabled)

"""2b declaration ownership observation reference; no provider inputs."""
from .librarycontract import produce as validate

def produce(frame: bytes) -> bytes:
    def fail(reason):
        raise ValueError("library partition: " + reason)
    if not isinstance(frame, bytes) or len(frame) < 16 or frame[:8] != b"USLPAR1\n" or frame[10] not in (0, 1) or frame[11] not in (0, 1):
        fail("malformed declaration stream")
    count = int.from_bytes(frame[12:16], "little")
    audit = frame[10]
    data_mode = frame[11]
    at, owners = 16, []
    for index in range(count):
        if at + 36 > len(frame):
            fail("malformed declaration stream")
        size = int.from_bytes(frame[at:at+4], "little")
        owners.append(1 if frame[at+8] == 0 and frame[at+9] == 0 else (4 if frame[at+8] == 1 and data_mode else 2))
        at += 4 + size
        if at > len(frame):
            fail("malformed declaration stream")
    try:
        validate(b"USLDEC1\n" + frame[8:10] + b"\0\0" + frame[12:at])
    except ValueError:
        fail("malformed declaration stream")
    claims = frame[at:]
    if audit:
        if len(claims) != count:
            fail("malformed declaration stream")
        for claim, owner in zip(claims, owners):
            if claim == 0: fail("missing-owner")
            if claim == 3 or (data_mode and claim in (5, 6, 7)): fail("duplicate-owner")
            if claim not in (1, 2, 4): fail("malformed declaration stream")
            if claim != owner: fail("wrong-owner")
    elif claims:
        fail("malformed declaration stream")
    return b"USLSEL1\n" + count.to_bytes(4, "little") + b"".join((256+i).to_bytes(4,"little") + bytes([owner]) for i,owner in enumerate(owners))

"""Reader-only fixed-two-int witness oracle; observation, not native ABI proof.

Explicit source facts (used/defined/prototype-compatible) come from frozen E1
fixtures. This does not parse C or certify the entire typed signature domain.
The opt-in overlay replaces only the dispatcher literal with stable-ID libraryfn.
PLAN remains zero; binding and lower consumption are not proved here.
Optional actual E1 callsite tokens enable arity checking; without them this
remains a declaration/frame projection, not callsite eligibility.
"""
import struct
from .librarycontract import produce as validate

def call_arguments(tokens, name):
    """One E1 observer shared by fixed arity and variadic frames; direct calls only."""
    start = tokens.find(b"UNITOK1\0", 0, 32)
    if start < 0 or start + 12 > len(tokens):
        raise ValueError("reference callsite requires complete UNITOK1")
    at = start + 12 + struct.unpack_from("<I", tokens, start + 8)[0]
    words = []
    while at < len(tokens) and tokens[at] == 64:
        if at + 6 > len(tokens) or tokens[at + 5] != 10:
            raise ValueError("reference callsite token framing")
        end = tokens.find(b"\n", at + 6)
        if end < 0:
            raise ValueError("reference callsite token truncated")
        words.append(tokens[at + 6:end].decode("ascii"))
        at = end + 1
        if words[-1] == "eof": break
    if not words or words[-1] != "eof":
        raise ValueError("reference callsite missing eof")
    braces = 0
    for i, token in enumerate(words):
        if token == "{": braces += 1
        elif token == "}": braces -= 1
        if braces <= 0 or token != "id=" + name or words[i + 1:i + 2] != ["("]:
            continue
        depth, args, arg = 1, [], []
        for token in words[i + 2:]:
            if token == ")" and depth == 1:
                if arg: args.append(tuple(arg))
                elif args: raise ValueError("reference callsite empty argument")
                yield tuple(args)
                break
            if token == "," and depth == 1:
                if not arg: raise ValueError("reference callsite empty argument")
                args.append(tuple(arg)); arg = []
                continue
            arg.append(token)
            if token in ("(", "[", "{"): depth += 1
            elif token in (")", "]", "}"): depth -= 1
        else: raise ValueError("reference callsite unclosed arguments")

def call_arities(tokens, name):
    for args in call_arguments(tokens, name): yield len(args)

def variadic_frames(frame, selection, source_state, tokens):
    """Narrow integer-literal witness of address-free callsite bytes, not an ABI oracle.

    The product derives descriptors in LV/LX. Here the existing reader reference
    observes frozen E1 direct literal calls and canonical scalar declarations.
    General expressions, floats/aggregates and multiple declared symbols are
    outside this reference witness and fail closed.
    """
    def fail(reason="type"):
        raise ValueError("library typed " + reason + " mismatch symbol " + name)
    length, ident, kind, callkind, version, flags, nn, sn, extent, fixed, var, reserved = struct.unpack_from("<IIBBBBIIQIB3s", frame, 16)
    name=frame[52:52+nn].decode("ascii"); sig=frame[52+nn:]
    if (ident,kind,callkind,version,flags,extent,var)!=(256,0,2,3,0,0,1) or tokens is None: fail()
    if sig[:8]!=b"USLSIG3\n" or struct.unpack_from("<Q",sig,8)[0]!=1: fail()
    z=struct.unpack_from("<Q",sig,16)[0]
    if sig[24:24+z]!=name.encode() or sig[24+z:28+z]!=b"\0\1\1\1" or struct.unpack_from("<Q",sig,28+z)[0]!=fixed: fail()
    at=36+z; result=sig[at:at+78]; at+=78
    if struct.unpack_from("<Q",sig,at)[0]!=fixed: fail()
    at+=8; prefix=[sig[at+78*i:at+78*(i+1)] for i in range(fixed)]
    if len(sig)!=at+78*fixed+1 or sig[-1:]!=b"\0": fail()
    # This witness supports signed int32/long scalar results and fixed parameters.
    def scalar(desc):
        if len(desc)!=78: fail()
        fields=struct.unpack_from("<7Q",desc); facts=struct.unpack_from("<BBQBBBBQ",desc,56)
        if fields[0] or fields[2] or fields[3]!=1 or fields[4] not in (4,8) or fields[5] or fields[6]!=fields[4] or facts!=(0,0,fields[4],0,3,1,0,0): fail()
        return fields[4]
    scalar(result)
    for d in prefix: scalar(d)
    if source_state=="source-mismatch":raise ValueError("not covered: library import binding or signature")
    if source_state in ("unused","defined"):return b"USLCSF1\n"+struct.pack("<Q",0)
    if source_state!="used":fail()
    def promoted(arg):
        word="".join(arg)
        if word.startswith("-"): word=word[1:]
        if not word.startswith("num="):fail()
        val=word[4:]; suffix=val[-1:].lower(); digits=val[:-1] if suffix=="l" else val
        if not digits or not digits.isdigit():fail()
        width=8 if suffix=="l" else 4
        if int(digits) >= 1 << (width*8-1):fail()
        return struct.pack("<7QBBQBBBBQ",0,width,0,1,width,0,width,0,0,width,0,3,1,0,0)
    records=[]
    for ordinal,args in enumerate(call_arguments(tokens,name),1):
        if len(args)<fixed:fail("arity")
        # Fixed inputs are converted by the product into the declared prefix.
        for arg in args[:fixed]:promoted(arg)
        concrete=(b"USLSIG3\n"+struct.pack("<QQ",1,len(name))+name.encode()+b"\0\1\0\1"+struct.pack("<Q",len(args))+result+struct.pack("<Q",len(args))+b"".join(prefix)+b"".join(promoted(arg) for arg in args[fixed:])+b"\0")
        records.append(struct.pack("<5Q",32+len(concrete),ordinal,ident,fixed,len(concrete))+concrete)
    return b"USLCSF1\n"+struct.pack("<Q",len(records))+b"".join(records)

def stage_capabilities(names=()):
    """Generic capability-set adapter; authoritative names are the model manifest flags.

    This is reference/tool construction IO, never a product/runtime resource
    fallback. Semantic consumers retain ownership of their own capability.
    """
    from pathlib import Path
    path=Path(__file__).resolve().parents[1]/"exec/parse2/librarycallsite-manifest.tsv"
    lines=[line.split()[2:] for line in path.read_text().splitlines() if line.startswith("#! flags ")]
    if len(lines)!=1 or len(lines[0])!=len(set(lines[0])):
        raise ValueError("stage capabilities: ambiguous manifest flags")
    allowed=frozenset(lines[0]); entries={n:frozenset((n,)) for n in allowed}
    for row in path.read_text().splitlines():
        if row.startswith("# capability-alias\t"):
            _, alias, values=row.split("\t")
            targets=frozenset(values.split())
            if alias in entries or not targets<=allowed:
                raise ValueError("stage capabilities: invalid alias " + alias)
            entries[alias]=targets
    unknown=frozenset(names)-entries.keys()
    if unknown:raise ValueError("stage capabilities: unknown capability " + sorted(unknown)[0])
    return frozenset().union(*(entries[name] for name in names))

def produce(frame, selection, mode, source_state, tokens=None, *, capabilities=("typed-mode",)):
    def fail(): raise ValueError("not covered: library typed contract import")
    # Legacy mode is an alias row in the same table, resolved by the same adapter.
    caps=stage_capabilities(capabilities)
    if "librarytyped" not in caps: fail()
    validate(b"USLDEC1\n" + frame[8:])
    if frame[:8] != b"USLCON1\n" or frame[12:16] != b"\1\0\0\0" or mode != b"\1"+bytes(7): fail()
    length, ident, kind, callkind, version, flags, nn, sn, extent, count, var, reserved = struct.unpack_from("<IIBBBBIIQIB3s", frame, 16)
    if selection[:12] != b"USLSEL1\n"+struct.pack("<I",1) or len(selection)!=17 or int.from_bytes(selection[12:16],"little")!=ident: fail()
    owner=selection[16]
    if owner==2 and kind==0 and (callkind in (2,3) or callkind==1 and version!=3): return b""
    if {"librarytyped","librarycallsite"} <= caps and owner==1 and callkind==2:
        return variadic_frames(frame, selection, source_state, tokens)
    if owner!=1 or (ident,kind,callkind,version,flags,extent,count,var) != (256,0,1,3,0,0,2,0): fail()
    name=frame[52:52+nn].decode("ascii"); sig=frame[52+nn:]
    if sig[:8]!=b"USLSIG3\n" or struct.unpack_from("<Q",sig,8)[0]!=1: fail()
    z=struct.unpack_from("<Q",sig,16)[0]
    if sig[24:24+z]!=name.encode() or sig[24+z:28+z]!=b"\0\1\0\0" or struct.unpack_from("<Q",sig,28+z)[0]!=2: fail()
    # Three scalar descriptors: result followed by two fixed parameters.
    at=36+z
    for j in range(3):
        if j:
            if j==1:
                if struct.unpack_from("<Q",sig,at)[0]!=2: fail()
                at+=8
        fields=struct.unpack_from("<7Q",sig,at); facts=struct.unpack_from("<BBQBBBBQ",sig,at+56)
        if fields!=(0,0,0,1,4,0,4) or facts!=(0,0,4,0,3,2,0,0): fail()
        at+=78
    if sig[at:]!=b"\0": fail()
    if source_state=="source-mismatch": raise ValueError("not covered: library import binding or signature")
    if source_state in ("unused","defined"):return b""
    if source_state!="used":fail()
    if tokens is not None and any(n != count for n in call_arities(tokens, name)):
        raise ValueError("library typed arity mismatch symbol " + name)
    text=name+":\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 72\n"
    text+="  store64 [r7+48], r0\n  store64 [r7+56], r1\n  imm r1, 0\n  store64 [r7+0], r1\n"
    text+="  imm r1, 48\n  add64 r1, r7, r1\n  store64 [r7+8], r1\n  imm r1, 64\n  add64 r1, r7, r1\n  store64 [r7+16], r1\n"
    text+="  imm r1, 2\n  store64 [r7+24], r1\n  imm r1, 0\n  store64 [r7+32], r1\n  imm r1, 0\n  store64 [r7+40], r1\n"
    text+="  .libraryfn r1, 256\n  mov r0, r7\n  .librarycall r1, r0\n  load64 r1, [r7+16]\n  load64 r0, [r1+0]\n"
    text+="  .frame -72\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n"
    return text.encode()

if __name__=="__main__":
    import sys
    from pathlib import Path
    try: out=produce(*(Path(x).read_bytes() for x in sys.argv[1:4]),sys.argv[4],Path(sys.argv[5]).read_bytes() if len(sys.argv)>5 else None,capabilities=stage_capabilities(tuple(x.removeprefix("--") for x in sys.argv[6:]) or ("typed-mode",)))
    except (ValueError,struct.error,UnicodeError) as e:print(e,file=sys.stderr);sys.exit(1)
    sys.stdout.buffer.write(out)

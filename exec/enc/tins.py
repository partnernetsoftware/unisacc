"""TIns text (research/e5-tins-interface.md): dump a real TargetProgram, parse it back.

One instruction per line: `op a, b, ... key=value ...`; `name:` lines give labels
(several may share an index; a label past the last instruction comes last).
setreg's (kind, v) is `kind v`; spinit's None second operand is left out (the
stated normalisation); any other None, or a type this text has no form for, is
refused, not dropped.  Meta: the keys lower.py sets (META below); a bool is
`true`/`false`, a None meta value is `none` -- only in meta, where no label can
be meant.  A bool operand (argsave's third) is `true`/`false` too.  Nothing here
computes machine code, layout, offsets or short branches."""
META = ("reloc", "form", "gate", "carry", "role", "catop", "sysno", "ret", "retconv", "winapi", "winimp",
        "hstd", "scr0", "scr1", "written")


def _arg(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, int):
        return str(v)
    if isinstance(v, str) and v and " " not in v and "," not in v and "=" not in v and not v.startswith("'"):
        # a string that would read back as something else (an int, a bool, none) is quoted: 'VALUE
        return v if isinstance(_val(v), str) and v != "none" else "'" + v
    if isinstance(v, tuple) and len(v) == 2 and v[0] in ("imm", "reg", "mem", "addr"):
        return "%s %s" % (v[0], _arg(v[1]))
    raise ValueError("no text form for %r" % (v,))


def dump(tp, full=False):
    at = {}
    for name, pc in tp.labels.items():
        at.setdefault(pc, []).append(name)
    out = []
    if full:
        # Lowering data only: no layout addresses, instruction offsets or bytes.
        out = ["@target " + tp.target, "@data " + (tp.data.hex() or "-")]
        for attr in ("src_os", "data_len", "bss", "relocs"):
            if hasattr(tp, attr):
                value = getattr(tp, attr)
                if attr == "relocs":
                    value = ",".join(str(v) for v in value) or "-"
                out.append("@%s %s" % (attr, value))
        for name, addr in tp.syms.items():
            if not isinstance(name, str) or not name or any(c.isspace() for c in name):
                raise ValueError("invalid data symbol name")
            if type(addr) is not int:
                raise ValueError("data symbol address is not an integer")
            out.append("@sym %s %d" % (name, addr))
    for pc, ins in enumerate(tp.code + [None]):
        for name in at.get(pc, []):
            out.append(name + ":")
        if ins is None:
            break
        args = list(ins.args)
        if ins.op == "spinit" and len(args) == 2 and args[1] is None:
            args = args[:1]
        ln = ins.op + (" " + ", ".join(_arg(a) for a in args) if args else "")
        for k, v in ins.meta.items():
            if k not in META:
                raise ValueError("meta %r has no text form" % k)
            ln += " %s=%s" % (k, "none" if v is None else _arg(v))
        out.append(ln)
    return "\n".join(out) + "\n"


def parse(text, target="lnx/x86_64"):
    from unisa.lower import TargetProgram
    tp = TargetProgram(target, b"", {})
    header = set()
    started = False
    for ln in text.split("\n"):
        ln = ln.strip()
        if not ln:
            continue
        if ln.startswith("@"):
            if started:
                raise ValueError("header after instructions or labels")
            words = ln.split()
            tag = words[0]
            if tag in ("@target", "@data"):
                if tag in header or len(words) != 2:
                    raise ValueError("duplicate or malformed " + tag)
                header.add(tag)
                if tag == "@target":
                    if words[1] not in {o + "/" + a for o in ("lnx", "osx", "win") for a in ("x86_64", "arm64")}:
                        raise ValueError("unknown target")
                    tp.target = words[1]
                    tp.os, tp.arch = tp.target.split("/")
                else:
                    value = words[1]
                    if value != "-" and (len(value) % 2 or any(c not in "0123456789abcdef" for c in value)):
                        raise ValueError("invalid data bytes")
                    tp.data = b"" if value == "-" else bytes.fromhex(value)
            elif tag in ("@src_os", "@data_len", "@bss", "@relocs"):
                if tag in header or len(words) != 2:
                    raise ValueError("duplicate or malformed " + tag)
                header.add(tag)
                value = words[1]
                if tag == "@src_os":
                    if value not in ("lnx", "osx", "win"):
                        raise ValueError("unknown source OS")
                elif tag == "@relocs":
                    value = [] if value == "-" else [int(v, 10) for v in value.split(",")]
                    if any(v < 0 for v in value):
                        raise ValueError("negative relocation offset")
                else:
                    value = int(value, 10)
                    if value < 0:
                        raise ValueError("negative size")
                setattr(tp, tag[1:], value)
            elif tag == "@sym" and len(words) == 3:
                if words[1] in tp.syms:
                    raise ValueError("duplicate data symbol")
                tp.syms[words[1]] = int(words[2], 10)
            else:
                raise ValueError("unknown or malformed header")
            continue
        started = True
        if ln.endswith(":") and " " not in ln:
            if ln[:-1] in tp.labels:
                raise ValueError("label defined twice: " + ln)
            tp.labels[ln[:-1]] = len(tp.code)
            continue
        words = ln.split(" ")
        meta = {}
        while "=" in words[-1]:
            k, _, v = words.pop().partition("=")
            if k in meta or k not in META:
                raise ValueError("meta: " + k)
            meta[k] = None if v == "none" else _val(v)
        op, rest = words[0], " ".join(words[1:])
        args = [_val(a.strip()) for a in rest.split(",")] if rest.strip() else []
        args = [tuple([a.split(" ")[0], _val(a.split(" ")[1])]) if isinstance(a, str) and " " in a else a for a in args]
        if op == "spinit" and len(args) == 1:
            args.append(None)
        tp.emit(op, *args, **meta)
    if header or tp.syms:
        if not {"@target", "@data"}.issubset(header):
            raise ValueError("full header requires target and data")
    return tp


def _val(s):
    if s.startswith("'"):
        return s[1:]
    if s in ("true", "false"):
        return s == "true"
    try:
        return int(s, 0)
    except ValueError:
        return s

"""SHA-256 transitions for Mach-O page signatures; no hash runtime primitive.
Input: sh_blob. Appends 32 digest bytes, restores input, returns via RET.
Round/initial constants are declarations read from src/back_image.c. The
compression and padding algorithm is explicitly compiled into ordinary actions.
"""
import re
from pathlib import Path
W, K = 2 << 40, 3 << 40  # distinct from the image DATA byte region


def constants():
    s=(Path(__file__).resolve().parents[2]/'src/back_image.c').read_text()
    blocks=re.findall(r'long SHA_K\[64\]\s*=\s*\{([^}]+)\}',s)
    assert len(blocks)==1
    k=[int(x.strip()) for x in blocks[0].split(',') if x.strip()]
    init=re.findall(r'int sha_init\(void\)\s*\{(.*?)return 0;',s,re.S)
    assert len(init)==1
    pairs=re.findall(r'sha_h\[(\d+)\]\s*=\s*(\d+);',init[0])
    assert len(k)==64 and [int(i) for i,_ in pairs]==list(range(8))
    h=[int(v) for _,v in pairs]
    assert all(0<=x<2**32 for x in k+h)
    return k,h


def install(E):
    from finite_rules import install as install_rules
    ks, hs = constants()
    initial = []
    for i, value in enumerate(ks):
        initial.extend((("LDI", "sh_x", i), ("LDI", "sh_y", value), ("STX", "sh_x", K, "sh_y")))
    initial.extend(("LDI", "sh_h" + str(i), value) for i, value in enumerate(hs))
    bindings = dict(W=W, K=K)
    for line in Path(__file__).with_name("sha256-names.tsv").read_text().splitlines():
        if line and not line.startswith("#"):
            key, prefix, kind = line.split("\t")
            bindings[key] = E.P(prefix + ".sha_" + key).fresh(kind)
    install_rules(E.g, Path(__file__).parent, "sha256", bindings=bindings,
                  sequences={"constants": initial}, section="main")

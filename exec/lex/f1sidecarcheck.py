"""Check the optional F1 lexer sidecar without changing public token bytes."""
import json
import pathlib
import struct
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "pp"))
import sim


class Files:
    def __init__(self, sourcefacts=False):
        self.sourcefacts = sourcefacts

    def get(self, key):
        if key == b"\0cli/f1":
            return b"\1"
        if self.sourcefacts and key == b"\0library/sourcefacts":
            return struct.pack("<Q", 1)
        return None


with tempfile.TemporaryDirectory(prefix="e1-f1-") as td:
    delta_path = pathlib.Path(td) / "lex.json"
    subprocess.run([sys.executable, str(HERE.parent / "build" / "gen.py"),
                    "lex", str(delta_path), "--sourcefacts"], check=True,
                   stdout=subprocess.DEVNULL, timeout=25)
    delta = json.loads(delta_path.read_text())
    delta["states"]["HALT"] = ["b", {}]
    delta["seqs"] = [[["ALUI", "add", a[1], a[1], 1] if a[0] == "INC" else a
                      for a in seq] for seq in delta["seqs"]]
    for source, kinds in (
        (b"int x;", []),
        (b"__attribute__((constructor)) void boot(void){}", [1]),
        (b"__attribute__((destructor)) void bye(void){}", [2]),
        (b"__attribute__((constructor)) void a(void){} "
         b"__attribute__((destructor)) void b(void){}", [1, 2]),
    ):
        result, public, _ = sim.run(delta, source, "f1.c", maxsteps=200000)
        assert result == "accept", (source, result)
        result, framed, _ = sim.run(delta, source, "f1.c", files=Files(), maxsteps=200000)
        assert result == "accept" and framed[:8] == b"USLATTR1", (source, result)
        count, token_len = struct.unpack_from("<II", framed, 8)
        assert count == len(kinds) and token_len == len(public)
        records = [struct.unpack_from("<IB", framed, 16 + 5 * i) for i in range(count)]
        assert [kind for _, kind in records] == kinds, (source, records)
        assert framed[16 + 5 * count:] == public, source
    source = b"__attribute__((constructor)) void boot(void){}"
    facts = b"USLFACT1\n" + bytes((2, 1, 1)) + struct.pack("<Q", len(source)) + source
    result, framed, _ = sim.run(delta, facts, "f1.c", files=Files(True), maxsteps=200000)
    assert result == "accept" and framed[:8] == b"USLATTR1"
    count, token_len = struct.unpack_from("<II", framed, 8)
    assert count == 1 and framed[16 + 5 * count:][:9] == b"USLFACT1\n"
    assert token_len == len(framed) - 16 - 5 * count
    print("F1 E1 sidecar: 5 cases, public tokens unchanged")

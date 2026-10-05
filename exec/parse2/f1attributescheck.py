"""F1 attribute marks: E1 sidecar and E3 tape against the C reference."""
import json
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "exec" / "pp"))
sys.path.insert(0, str(ROOT / "exec"))
import sim
import assemble


class Files:
    def __init__(self, values):
        self.values = values
        self.disk = sim.Files(str(ROOT / "include"))

    def get(self, key):
        return self.values[key] if key in self.values else self.disk.get(key)


def model(stage, path, *flags):
    subprocess.run([sys.executable, str(ROOT / "exec" / "build" / "gen.py"),
                    stage, str(path), *flags], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL, timeout=40)
    delta = json.loads(path.read_text())
    delta["states"]["HALT"] = ["b", {}]
    if stage == "lex":
        delta["seqs"] = [[["ALUI", "add", a[1], a[1], 1] if a[0] == "INC" else a
                          for a in seq] for seq in delta["seqs"]]
    return delta


with tempfile.TemporaryDirectory(prefix="f1-attributes-") as td:
    tmp = pathlib.Path(td)
    ref = tmp / "ua"
    subprocess.run([str(ROOT / "tests" / "build_ref.sh"), str(tmp / "ua.c"), str(ref)],
                   cwd=ROOT, check=True, stdout=subprocess.DEVNULL, timeout=40)
    e2 = model("pp", tmp / "e2.json", "--locations", "--shared-predefines")
    e1 = model("lex", tmp / "e1.json", "--locations")
    e3 = model("parse2", tmp / "e3.json", "--errors")
    resources = {b"\0predefines/" + k.encode(): v.encode()
                 for k, v in assemble.load_facts("pp-gen")["predefres"].items()}
    resources[b"\0cli/target"] = b"osx/arm64"
    plain = tmp / "small.c"
    plain.write_text("int x;\n__attribute__((constructor)) void boot(void){x=1;}\n"
                     "__attribute__((destructor)) void bye(void){x=2;}\n"
                     "int main(void){return x;}\n")
    for source, expected_marks in ((plain, 2), (ROOT / "tests/c/f1_ctor_dtor.c", 3)):
        verdict, pp, _ = sim.run(e2, source.read_bytes(), str(source),
                                 files=Files(resources), maxsteps=5000000)
        assert verdict == "accept", (source, verdict, pp)
        verdict, framed, _ = sim.run(e1, pp, str(source),
                                     files=Files({b"\0cli/f1": b"\1"}), maxsteps=5000000)
        assert verdict == "accept" and framed[:8] == b"USLATTR1", source
        count, token_len = struct.unpack_from("<II", framed, 8)
        assert count == expected_marks, (source, count)
        marks = framed[16:16 + 5 * count]
        tokens = framed[16 + 5 * count:]
        assert token_len == len(tokens)
        attrs = struct.pack("<I", count) + b"".join(
            struct.pack("<I", 0) + marks[5 * i:5 * i + 5] for i in range(count))
        verdict, tape, _ = sim.run(e3, tokens, str(source),
                                   files=Files({b"\0cli/attributes": attrs}),
                                   maxsteps=10000000)
        assert verdict == "accept", (source, verdict, tape)
        reference = subprocess.check_output([str(ref), "-S", str(source)], timeout=30)
        assert tape == reference, (source, len(tape), len(reference))
        print("F1 attributes:", source.name, count, "marks, tape byte equal", flush=True)

"""Compare PE host forwarding through the lower and encoder tables with C reference."""
import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]))
from exec.pp.sim import run


SOURCES = {
    "addr": "long __hostaddr0(void); int main(void){return __hostaddr0()!=0;}\n",
    "callonly": ("long __hostcall(long,long*); "
                 "int main(void){long a[10]={0}; return __hostcall(0,a)==0;}\n"),
    "call": ("long __hostaddr0(void); long __hostcall(long,long*); "
             "int main(void){long a[10]={0}; return __hostcall(__hostaddr0(),a)==0;}\n"),
}


def command(*args):
    p = subprocess.run(args, capture_output=True, timeout=55, check=True)
    return p.stdout


def main():
    if len(sys.argv) != 7:
        raise SystemExit("usage: peforward.py REF LOWER_JSON ENC_JSON RUNTIME ENC_TBL ARCH")
    ref, lower_path, enc_path, runtime, enc_tbl, arch = sys.argv[1:]
    import json
    lower = json.loads(pathlib.Path(lower_path).read_text())
    enc = json.loads(pathlib.Path(enc_path).read_text())
    with tempfile.TemporaryDirectory(prefix="unisa-peforward-") as td:
        tmp = pathlib.Path(td)
        for name, source in SOURCES.items():
            src = tmp / (name + ".c")
            tape = tmp / (name + ".tape")
            image = tmp / (name + ".exe")
            src.write_text(source)
            command(ref, "-b", "win/" + arch, "-S", str(src), "-o", str(tape))
            command(ref, "-b", "win/" + arch, str(src), "-o", str(image))
            status, tins, _ = run(lower, tape.read_bytes(), "input", maxsteps=3000000)
            assert status == "accept", (arch, name, "lower", status)
            status, got, _ = run(enc, tins, "input", maxsteps=3000000)
            assert status == "accept", (arch, name, "enc", status)
            tins_path = tmp / (name + ".tins")
            tins_path.write_bytes(tins)
            assert command(runtime, enc_tbl, str(tins_path)) == got, (arch, name, "C table")
            want = image.read_bytes()
            assert got == want, (arch, name, len(got), len(want),
                                 next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), None))
            print("PE forward", arch, name, len(got), "bytes equal", flush=True)


if __name__ == "__main__":
    main()

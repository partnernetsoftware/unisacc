"""Chain the stages of exec/pipeline/stages.tsv on one C file.

    python3 exec/pipeline/run.py [--ref-feed] [--gen] [-I DIR] FILE.c

Refuses to connect adjacent stages whose formats differ.  Each stage (and each
reference / checker call) is bounded by 58 s.  Stops at the first reject or
not-covered, naming the stage.  Every stage's output is compared with the
reference's stream of the same format.  An input format ending in `?` is a
declared not-yet-connected edge: without --ref-feed the chain stops there;
with it, that stage is fed the reference's stream (reported `ref-fed`).
Exit 0 when every stage run was accepted and equal to the reference.
"""
import hashlib
import models
import os
import shlex
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
T = 58
# every artefact (reference binaries, deltas) lives in the per-checkout dir of
# exec/stamp.sh and is rebuilt by its `fresh` when its sources change
X = subprocess.run(["sh", "exec/stamp.sh", "dir"], cwd=ROOT, capture_output=True,
                   text=True, check=True).stdout.strip()
os.makedirs(os.path.join(X, "pipe"), exist_ok=True)
for k, v in (("E2REF", "ua_ref"), ("E1REF", "ua_ref"), ("E3REF", "ua_ref"), ("E3DUMP", "ua_tdump")):
    os.environ[k] = os.path.join(X, v)


def manifest():
    rows = []
    for l in open(os.path.join(HERE, "stages.tsv")):
        if l.strip() and not l.startswith("#"):
            f = l.rstrip("\n").split("\t")
            f = [c.replace("{x}", X) for c in f]
            rows.append(dict(zip(("name", "gen", "exec", "in", "out", "ref", "delta", "refin"), f)))
    return rows


def sh(cmd):
    try:
        p = subprocess.run(shlex.split(cmd), capture_output=True, timeout=T, cwd=ROOT)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return "timeout", b"", b""


def fresh(out, cmd, inputs):
    """exec/stamp.sh fresh: rebuild out unless its stamp matches cmd + inputs."""
    p = subprocess.run(["sh", "-c", '. exec/stamp.sh; fresh "$@"', "fresh", out] + cmd + ["--"] + inputs,
                       cwd=ROOT, capture_output=True, timeout=T)
    return p.returncode, p.stderr


def refs():
    b = [sys.executable, os.path.join(ROOT, "tests", "bound.py"), "58"]
    src = subprocess.run(["sh", "-c", '. exec/stamp.sh; echo $REFSRC'], cwd=ROOT,
                         capture_output=True, text=True).stdout.split()
    r, e = fresh(os.path.join(X, "ua_ref"), b + ["./tests/build_ref.sh", os.path.join(X, "ua_ref.c"),
                                                  os.path.join(X, "ua_ref")], src)
    if r == 0:
        r, e = fresh(os.path.join(X, "ua_tdump"), b + ["exec/parse/mkdump.sh", os.path.join(X, "ua_ref.c"),
                                                        os.path.join(X, "ua_tdump")],
                     [os.path.join(X, "ua_ref.c"), "exec/parse/mkdump.sh"])
    return r, e


def gen_inputs(cmd):
    """Use the shared content closure, including K2 manifests and facts.

    fresh already hashes the command. The digest file covers added/removed
    inputs too; selecting directories from the entrypoint misses stage data.
    """
    digest = models.closure(hashlib.sha256(sys.version.encode())).hexdigest()
    path = os.path.join(X, "pipe", "generator-closure-" + digest + ".sha256")
    with tempfile.NamedTemporaryFile(mode="w", dir=os.path.dirname(path),
                                     delete=False) as f:
        f.write(digest + "\n")
        temporary = f.name
    try:
        os.replace(temporary, path)
    finally:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
    return [path, os.path.join(X, "ua_ref.stamp")]


def check(fmt, path):
    r, _, e = sh("python3 %s %s" % (os.path.join(HERE, "check_%s.py" % fmt.replace(".", "_")), path))
    return None if r == 0 else e.decode("latin-1").strip() or str(r)


def first_diff(a, b):
    la, lb = a.split(b"\n"), b.split(b"\n")
    for i, (x, y) in enumerate(zip(la, lb)):
        if x != y:
            return "line %d: %r vs ref %r" % (i + 1, x[:60], y[:60])
    return "length %d vs ref %d lines" % (len(la), len(lb))


def split_f1(frame):
    """E1's F1 frame carries marks outside the ordinary typed-token stream."""
    if len(frame) < 16 or frame[:8] != b"USLATTR1":
        raise ValueError("E1 did not return an F1 attribute frame")
    count, token_len = struct.unpack_from("<II", frame, 8)
    end = 16 + 5 * count
    if count > 256 or end + token_len != len(frame):
        raise ValueError("malformed E1 attribute frame")
    attrs = bytearray(struct.pack("<I", count))
    for i in range(count):
        attrs.extend(struct.pack("<I", 0))  # one source, unit zero
        attrs.extend(frame[16 + 5 * i:21 + 5 * i])
    return frame[end:], bytes(attrs)


def main():
    a = sys.argv[1:]
    feed = "--ref-feed" in a
    gen = "--gen" in a
    include_dirs = []
    files = []
    i = 0
    while i < len(a):
        arg = a[i]
        if arg in ("--ref-feed", "--gen"):
            pass
        elif arg == "-I":
            i += 1
            if i >= len(a):
                print("missing -I directory")
                return 2
            include_dirs.append(os.path.abspath(a[i]))
        elif arg.startswith("-I") and len(arg) > 2:
            include_dirs.append(os.path.abspath(arg[2:]))
        elif arg.startswith("-"):
            print("unknown option: " + arg)
            return 2
        else:
            files.append(arg)
        i += 1
    if len(files) != 1:
        print("expected one C source file")
        return 2
    src = os.path.abspath(files[0])
    incflags = "".join(" -I " + shlex.quote(d) for d in include_dirs)
    rows = manifest()
    r, e = refs()
    if r != 0:
        print("could not build the reference binaries: %s" % e[-300:])
        return 1
    # connection check: the whole manifest, before running anything
    prev = "src.c"
    for s in rows:
        want = s["in"].rstrip("?")
        if s["in"].endswith("?"):
            print("edge %-3s <- %s: NOT CONNECTED (declared: upstream writes %s, stage needs %s)" % (s["name"], prev, prev, want))
        elif want != prev:
            print("REFUSED: %s needs %s, upstream writes %s" % (s["name"], want, prev))
            return 2
        prev = s["out"]
    tmp = tempfile.mkdtemp(prefix="pipe_")
    # E2 resolves quoted includes relative to the source path.  Feeding a copy
    # under tmp would silently change that search root and __FILE__ locations.
    cur = src
    bad = check("src.c", cur)
    if bad:
        print("input is not src.c: %s" % bad)
        return 1
    ok = True
    attrs_path = None
    for k, s in enumerate(rows, 1):
        n, fmt = s["name"], s["in"].rstrip("?")
        if s["in"].endswith("?"):
            if not feed:
                print("%s: stop -- input edge not connected (use --ref-feed)" % n)
                return 1
            r, o, e = sh(s["refin"].format(src=shlex.quote(src)) + incflags)
            cur = os.path.join(tmp, "%d.ref.%s" % (k, fmt))
            open(cur, "wb").write(o)
            print("%s: ref-fed %s from the reference (%s)" % (n, fmt, "exit %s" % r))
        bad = check(fmt, cur)
        if bad:
            print("%s: input fails %s check: %s" % (n, fmt, bad))
            return 1
        if gen and os.path.exists(s["delta"] + ".stamp"):
            os.unlink(s["delta"] + ".stamp")
        r, e = fresh(s["delta"], shlex.split(s["gen"].format(delta=s["delta"])), gen_inputs(s["gen"]))
        if True:
            if r != 0:
                print("%s: gen failed (%s) %s" % (n, r, e[-200:]))
                return 1
        cmd = s["exec"].format(delta=s["delta"], **{"in": shlex.quote(cur)})
        if n == "E2":
            cmd += " --source " + shlex.quote(src) + incflags
        if n == "E3" and attrs_path is not None:
            cmd += " --attributes " + shlex.quote(attrs_path)
        r, o, e = sh(cmd)
        if n == "E1" and r == 0:
            try:
                o, attrs = split_f1(o)
            except ValueError as ex:
                print("E1: not covered -- %s" % ex)
                return 1
            attrs_path = os.path.join(tmp, "e1.attributes")
            open(attrs_path, "wb").write(attrs)
        # The token dumper's command path does not accept -I.  Feed it the
        # already checked E2 stream; this is also the exact text E1 consumed.
        ref_input = cur if n == "E1" else src
        rr, ro, _ = sh(s["ref"].format(src=shlex.quote(ref_input)) + (" -nostdinc" if n == "E1" else incflags))
        et = e.decode("latin-1").strip().split("\n")[0][:100]
        if et.startswith("REJECT "):
            et = et[7:]
        if r == 1 and not et.startswith("not covered"):
            verdict = "reject-agree" if rr != 0 else "REJECT (ref accepts)"
            print("%s: %s -- %s" % (n, verdict, et))
            return 0 if rr != 0 else 1
        if r != 0:
            print("%s: not covered (exit %s) -- %s" % (n, r, et))
            return 1
        out = os.path.join(tmp, "%d.%s" % (k, s["out"]))
        open(out, "wb").write(o)
        bad = check(s["out"], out)
        eq = rr == 0 and o == ro
        print("%s: accept %s %d B, %s%s" % (n, s["out"], len(o),
              "equal to reference" if eq else "DIFF " + first_diff(o, ro),
              "" if not bad else "; format check FAILED: " + bad))
        ok = ok and eq and not bad
        cur = out
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

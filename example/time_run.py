# Time unisacc.com -run against small programs.
# python3 example/time_run.py
import os, statistics, subprocess, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UA = os.path.join(ROOT, "unisacc.com")
EX = os.path.dirname(os.path.abspath(__file__))
UNITS = os.path.join(EX, "units")
os.makedirs(UNITS, exist_ok=True)

def write(path, text):
    with open(path, "w") as f:
        f.write(text)

def unit_paths(n):
    paths = []
    for i in range(n):
        p = os.path.join(UNITS, "u%d.c" % i)
        if i == 0:
            body = "int f0(void) { return 0; }\nint main(void) { return f0(); }\n"
        else:
            body = "int f%d(void) { return %d; }\n" % (i, i)
        write(p, body)
        paths.append(p)
    return paths

def many_fn(n):
    p = os.path.join(EX, "many_fn.c")
    lines = ["int f%d(void) { return %d; }\n" % (i, i) for i in range(n)]
    lines.append("int main(void) { return f0(); }\n")
    write(p, "".join(lines))
    return p

def run(args, out=None):
    t = time.perf_counter()
    p = subprocess.run(args, cwd=ROOT, stdout=subprocess.DEVNULL if out is None else open(out, "wb"),
                       stderr=subprocess.PIPE)
    dt = time.perf_counter() - t
    err = p.stderr.decode("utf-8", "replace")
    return p.returncode, dt, err

def median(cmd, repeats=2, out=None):
    xs = []
    err = ""
    rc = 0
    for _ in range(repeats):
        rc, dt, err = run(cmd, out)
        xs.append(dt)
    return rc, statistics.median(xs), err

def asks(err):
    for line in err.splitlines():
        if line.startswith("asked pp"):
            return line
    return ""

def main():
    sh = ["/bin/sh", UA]
    rows = []
    def add(name, rc, dt, extra=""):
        rows.append((name, rc, dt, extra))
        print("%-28s rc=%s %7.3fs %s" % (name, rc, dt, extra), flush=True)

    rc, dt, err = median(sh + ["--version"])
    add("version", rc, dt)

    sources = {
        "empty": os.path.join(EX, "empty.c"),
        "include": os.path.join(EX, "include_only.c"),
        "printf": os.path.join(EX, "printf_hi.c"),
        "headers": os.path.join(EX, "headers.c"),
    }
    for name, src in sources.items():
        pre = "/tmp/ua-ex-%s.i" % name
        rc, dt, err = median(sh + ["-E", "-o", pre, src])
        nbytes = os.path.getsize(pre) if os.path.exists(pre) else -1
        add("E " + name, rc, dt, "bytes %d" % nbytes)
        tape = "/tmp/ua-ex-%s.tape" % name
        rc, dt, err = median(sh + ["-c", "-o", tape, src])
        add("c " + name, rc, dt, err.strip())
        rc, dt, err = median(sh + ["-run", src])
        add("run " + name, rc, dt)

    for n in (1, 4, 8, 16):
        paths = unit_paths(n)
        rc, dt, err = median(sh + ["-run"] + paths)
        add("run units %d" % n, rc, dt)
        rc, dt, err = median(sh + ["-c", "-o", "/tmp/ua-ex-u.tape"] + paths)
        add("c units %d" % n, rc, dt, err.strip())

    src = many_fn(16)
    rc, dt, err = median(sh + ["-run", src])
    add("run many_fn 16", rc, dt)
    rc, dt, err = median(sh + ["-c", "-o", "/tmp/ua-ex-many.tape", src])
    add("c many_fn 16", rc, dt, err.strip())

    clang = "/usr/bin/clang"
    if os.path.exists(clang):
        for name, src in sources.items():
            rc, dt, err = median([clang, "-O0", "-c", "-o", "/tmp/ua-ex-clang.o", src])
            add("clang -O0 -c " + name, rc, dt)

    outp = os.path.join(EX, "times.tsv")
    with open(outp, "w") as f:
        f.write("name\trc\tseconds\textra\n")
        for name, rc, dt, extra in rows:
            f.write("%s\t%s\t%.3f\t%s\n" % (name, rc, dt, extra))
    print("wrote", outp)

if __name__ == "__main__":
    main()

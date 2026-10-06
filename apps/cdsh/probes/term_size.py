# Pty witness for term size. Started by check.c. No C compiler involved.
import os, pty, struct, fcntl, termios, subprocess, time, select, sys, shlex
U = sys.argv[1]

def run(rows, cols, timeout=30):
    mfd, sfd = pty.openpty()
    fcntl.ioctl(sfd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    cmd = "exec %s term.c term_cli.c size" % shlex.quote(U)
    p = subprocess.Popen(["/bin/sh", "-c", cmd], stdout=sfd, stderr=sfd, stdin=sfd, close_fds=True)
    os.close(sfd)
    out, deadline = b"", time.time() + timeout
    while time.time() < deadline:
        r, _, _ = select.select([mfd], [], [], 0.2)
        if r:
            try: c = os.read(mfd, 4096)
            except OSError: break
            if not c: break
            out += c
        if p.poll() is not None:
            while True:
                r, _, _ = select.select([mfd], [], [], 0.1)
                if not r: break
                try: c = os.read(mfd, 4096)
                except OSError: break
                if not c: break
                out += c
            break
    try: p.wait(timeout=5)
    except subprocess.TimeoutExpired: p.kill()
    os.close(mfd)
    return out.decode(errors="replace").strip()

u = run(30, 100)
us = run(2, 5)
print("uni=%s|uni-small=%s" % (u, us))

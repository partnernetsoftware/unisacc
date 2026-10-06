import os, pty, struct, fcntl, termios, subprocess, time, select, sys, shlex
D, U, HERE, PORT = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
SRC = ["tui.c","render.c","term.c","chat.c","clock.c","tools.c","file.c",
       "shell.c","edit.c","gate.c","json.c","session.c","agent.c","net.c"]
EP = "http://127.0.0.1:%d/v1/chat/completions" % PORT
TEXT = "build the feature, then report"
ROOT = os.path.dirname(HERE)
fail = 0
# unisacc compiles in-process; measured ~24s for this list on an idle machine.
FIRST_FRAME_S = 180

def run():
    work = os.path.join(D, "work")
    tr   = os.path.join(D, "tr.jsonl")
    os.makedirs(work, exist_ok=True)
    for name in ("思维树.md", "记忆宫殿.md"):
        try: os.remove(os.path.join(work, name))
        except OSError: pass
    try: os.remove(tr)
    except OSError: pass

    env = dict(os.environ)
    env["CDSH_ENDPOINT"]   = EP
    env["CDSH_MODEL"]      = "stub"
    env["CDSH_CWD"]        = work
    env["CDSH_TRANSCRIPT"] = tr
    # Drop TMUX so the in-TUI tmux enumeration connects via the socket instead
    # of the pty we hand the TUI. A pty whose master is this harness does not
    # answer tmux's terminal probes and would deadlock; the real user terminal
    # does, so this only affects the test. The window list comes from the same
    # server either way.
    env.pop("TMUX", None)

    mfd, sfd = pty.openpty()
    fcntl.ioctl(sfd, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 100, 0, 0))
    cmd = "cd %s && exec %s %s agent" % (
        shlex.quote(ROOT), shlex.quote(U), " ".join(shlex.quote(s) for s in SRC))
    p = subprocess.Popen(["/bin/sh", "-c", cmd], stdout=sfd, stderr=sfd,
                         stdin=sfd, close_fds=True, cwd=ROOT, env=env)
    os.close(sfd)
    # Wait for the first frame instead of sleeping a guess at the compile.
    out = b""
    up = False
    end = time.time() + FIRST_FRAME_S
    while time.time() < end:
        r, _, _ = select.select([mfd], [], [], 0.25)
        if r:
            try: c = os.read(mfd, 8192)
            except OSError: break
            if not c: break
            out += c; up = True
            break
        if p.poll() is not None:
            break
    if not up:
        try: p.wait(timeout=3); rc = p.returncode
        except subprocess.TimeoutExpired: p.kill(); rc = "TIMEOUT"
        os.close(mfd)
        return rc, out.decode(errors="replace"), tr, False
    try: os.write(mfd, TEXT.encode() + b"\r")
    except OSError: pass
    deadline = time.time() + 15.0
    while time.time() < deadline:
        r, _, _ = select.select([mfd], [], [], 0.25)
        if not r:
            if out: break
            continue
        try: c = os.read(mfd, 8192)
        except OSError: break
        if not c: break
        out += c
    try: os.write(mfd, b"\x03")
    except OSError: pass
    try: p.wait(timeout=3); rc = p.returncode
    except subprocess.TimeoutExpired: p.kill(); rc = "TIMEOUT"
    os.close(mfd)
    return rc, out.decode(errors="replace"), tr, True

def check(rc, screen, tr, up):
    global fail
    if not up:
        print("  FAIL  the program never drew a frame (rc=%s): %s"
              % (rc, " ".join(screen.split())[:200]))
        fail += 1
        return
    ok = True
    if rc not in (0,):
        print("  FAIL  tui exited non-zero (rc=%s)" % rc); ok = False
    for needle in ("hello-from-agent", "完成了", "you> " + TEXT):
        if needle not in screen:
            print("  FAIL  screen missing %r" % needle); ok = False
    if not os.path.exists(tr):
        print("  FAIL  no transcript"); ok = False
    else:
        t = open(tr).read()
        for needle in ("hello-from-agent", '"role":"decision","go":"stop"', "完成了"):
            if needle not in t:
                print("  FAIL  transcript missing %r" % needle); ok = False
    if ok:
        print("  ok    unisacc: agent ran in the TUI (exec -> answer -> go:stop)")
    else:
        fail += 1

rc, sc, tr, up = run()
check(rc, sc, tr, up)

sys.exit(1 if fail else 0)

import os, pty, struct, fcntl, termios, subprocess, time, select, json, sys, shlex
U = sys.argv[1]
ROOT = sys.argv[2]
SRCS = sys.argv[3]
# Per-run transcript under the caller's temp dir: a fixed /tmp path made two
# concurrent runs (check.sh runs this probe, and a person may run it too) write
# the same file, so one would delete the other's evidence mid-assertion.
TR  = os.path.join(sys.argv[4], "tui-e2e.jsonl")
fail = 0

# How long to wait for the FIRST FRAME before typing. unisacc compiles the
# sources in-process — measured 24s for this source list on an idle machine, and
# more under load — so the fixed 0.4s sleep this probe used to have raced the
# compile: the keystrokes went into a pty whose program had not started, and the
# probe then reported an empty screen as if the TUI had lost the input.
FIRST_FRAME_S = 120

def run(txt, per_char):
    lines = txt if isinstance(txt, (list, tuple)) else [txt]
    with open(TR, "w") as f:
        f.write('{"role":"model","p":0.830000}\n')
    mfd, sfd = pty.openpty()
    fcntl.ioctl(sfd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 70, 0, 0))
    # cdsh.sh is unisacc.com cdsh.c plus the file table.
    cdsh = os.path.join(ROOT, "cdsh.sh")
    cmd = "exec %s run %s" % (shlex.quote(cdsh), shlex.quote(TR))
    p = subprocess.Popen(["/bin/sh", "-c", cmd], stdout=sfd, stderr=sfd, stdin=sfd, close_fds=True)
    os.close(sfd)
    # Wait for the frame, do not sleep a guess at it. Reading the first bytes is
    # the only signal that says "the TUI is up and a keystroke will be seen".
    # A build error is also output, so a dead program is reported as a dead
    # program rather than as a TUI that lost the keystrokes.
    out = b""
    up = False
    end = time.time() + FIRST_FRAME_S
    while time.time() < end:
        r, _, _ = select.select([mfd], [], [], 0.25)
        if r:
            try: c = os.read(mfd, 8192)
            except OSError: break
            if not c: break
            out += c
            up = True
            break                  # one read is enough: the first frame is drawn
        if p.poll() is not None:
            break                  # died without drawing anything
    if not up:
        try: p.wait(timeout=3); rc = p.returncode
        except subprocess.TimeoutExpired: p.kill(); rc = "TIMEOUT"
        os.close(mfd)
        return rc, out.decode(errors="replace"), False
    for line in lines:
        try:
            if per_char:
                for ch in line:
                    os.write(mfd, ch.encode()); time.sleep(0.03)
            else:
                os.write(mfd, line.encode())
            time.sleep(0.2)
            os.write(mfd, b"\r")
        except OSError:
            break                  # the program exited mid-typing; judged below
        # The first frame is already in `out`. Wait until THIS line produces
        # new bytes, then until the diff goes quiet. A quiet check against the
        # old frame would return before the tool line was drawn.
        time.sleep(0.4)
        got = False
        deadline = time.time() + 4.0
        while time.time() < deadline:
            r, _, _ = select.select([mfd], [], [], 0.25)
            if not r:
                if got: break
                continue
            try: c = os.read(mfd, 8192)
            except OSError: break
            if not c: break
            out += c
            got = True
    try: os.write(mfd, b"\x03")
    except OSError: pass
    try: p.wait(timeout=3); rc = p.returncode
    except subprocess.TimeoutExpired: p.kill(); rc = "TIMEOUT"
    os.close(mfd)
    return rc, out.decode(errors="replace"), True

def user_lines():
    if not os.path.exists(TR): return []
    return [l.strip() for l in open(TR) if '"user"' in l]

# The transcript needs a probability or the gate has nothing to judge; the
# `once` path is not enough, so write one the same way the CLI does.
TEXT = "hello from tui"

for label, per_char in (("bulk (whole line in one write)", False), ("per-character", True)):
    rc, screen, up = run(TEXT, per_char)
    if not up:
        # Report the program's own words. "no verdict on screen" against a
        # program that never started is a misleading FAIL: it blames the TUI
        # for the build.
        print("  FAIL  %s: the program never drew a frame (rc=%s): %s"
              % (label, rc, " ".join(screen.split())[:200]))
        fail += 1
        continue
    ul = user_lines()
    # The record now carries a monotonic "ts" (clock.c), so match by content
    # rather than exact string — the timestamp is variable by design.
    last = ul[-1] if ul else ""
    if last and '"role":"user"' in last and ('"text":"%s"' % TEXT) in last and '"ts":' in last:
        print("  ok    %s: the full line reached the transcript (with ts)" % label)
    else:
        print("  FAIL  %s: transcript got %s" % (label, last if last else "<nothing>"))
        fail += 1
    if "CONTINUE" in screen or "STOP" in screen:
        print("  ok    %s: a verdict was rendered" % label)
    else:
        print("  FAIL  %s: no verdict on screen" % label); fail += 1
    if "unknown command" in screen:
        print("  ok    %s: cli dispatch showed on the tui" % label)
    else:
        print("  FAIL  %s: cli dispatch missing on screen" % label); fail += 1

# One TUI process, three CLI lines. The frame must show the shell line and
# the file the later read printed. This is the CLI entry driving the TUI.
HAND = "/tmp/cdsh-e2e-hand.txt"
rc, screen, up = run([
    "sh echo cdsh-hand",
    "write %s from-cli" % HAND,
    "read %s" % HAND,
], False)
if not up:
    print("  FAIL  cli hand: the program never drew a frame (rc=%s): %s"
          % (rc, " ".join(screen.split())[:200]))
    fail += 1
else:
    if "cdsh-hand" in screen and "from-cli" in screen:
        print("  ok    cli hand: echo and read landed on the tui")
    else:
        print("  FAIL  cli hand: screen missing tool output: %s"
              % " ".join(screen.split())[:240])
        fail += 1
if os.path.exists(HAND): os.remove(HAND)

# Every line must still be valid JSON, with the user text in it.
bad = 0
if not os.path.exists(TR):
    print("  FAIL  no transcript was written at all"); fail += 1
else:
    for i, l in enumerate(open(TR)):
        l = l.strip()
        if not l: continue
        try: json.loads(l)
        except Exception as e:
            bad += 1; print("  FAIL  line %d is not JSON: %s" % (i + 1, e))
    if bad == 0:
        print("  ok    every transcript line is valid JSON")
    else:
        fail += 1

if os.path.exists(TR): os.remove(TR)
sys.exit(1 if fail else 0)

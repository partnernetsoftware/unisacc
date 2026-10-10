#!/usr/bin/env python3
"""reload_state.py — regression probe for the v1 state validator.

WHY: the C library owns validity; this probe owns only the fixture
matrix. It runs the real CLI (via /bin/sh unisacc.com) on each case
and asserts the exit code and the PASS/FAIL line it prints. Most
cases are built with json.dumps; the duplicate-key cases are spliced
as raw text so they reach the validator exactly as written.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.dirname(HERE)              # .../apps/csih
ROOT = os.path.dirname(os.path.dirname(APP))   # .../unisacc.com
BIN = None                                       # set by build_bin()

def run_check(case_name, raw_text, expect_code, expect_ok, why_substr):
    """Run the CLI on raw_text; return (ok, message)."""
    tmpdir = tempfile.mkdtemp(prefix="csih-state-")
    try:
        path = os.path.join(tmpdir, "state.json")
        with open(path, "wb") as f:
            f.write(raw_text.encode("utf-8"))
        cmd = [BIN, "check", path]
        try:
            p = subprocess.run(cmd, cwd=APP, capture_output=True,
                               timeout=4)
        except subprocess.TimeoutExpired:
            return False, "%s: timeout" % case_name
        out = (p.stdout + p.stderr).decode("utf-8", "replace").strip()
        if p.returncode != expect_code:
            return False, "%s: exit %d, want %d | %s" % (
                case_name, p.returncode, expect_code, out)
        if expect_ok:
            if not out.startswith("PASS: ok"):
                return False, "%s: not PASS: ok | %s" % (case_name, out)
        else:
            if not out.startswith("FAIL:"):
                return False, "%s: not FAIL | %s" % (case_name, out)
            if why_substr and why_substr not in out:
                return False, "%s: want '%s' | %s" % (case_name, why_substr, out)
        return True, "%s: exit %d %s" % (case_name, p.returncode, out)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)

def base(**over):
    """A minimal valid state; override fields via kwargs."""
    d = {
        "version": 1,
        "session_id": "sess-abc",
        "handoff_id": "h1",
        "candidate_hash": "a" * 64,
        "goal": "do the thing",
        "input": "hello world",
        "cwd": "/tmp",
        "role": "agent",
        "peer": "",
        "journal": {"path": "/tmp/journal.jsonl", "offset": 0},
        "pending_queue": [],
    }
    d.update(over)
    return json.dumps(d, ensure_ascii=False)


def cases():
    cs = []
    # --- valid ---
    cs.append(("valid_ascii", base(), 0, True, None))
    cs.append(("valid_unicode_ws_goal_input",
               base(goal="目标 中文\n第二行", input="输入\t含空格 中文"),
               0, True, None))
    # 8 queue items pass
    cs.append(("queue8_pass",
               base(pending_queue=["a%d" % i for i in range(8)]),
               0, True, None))
    # 9 queue items reject
    cs.append(("queue9_reject",
               base(pending_queue=["a%d" % i for i in range(9)]),
               1, False, "pending_queue"))
    # goal 4095 pass, 4096 reject
    cs.append(("goal4095_pass", base(goal="g" * 4095), 0, True, None))
    cs.append(("goal4096_reject", base(goal="g" * 4096), 1, False, "4095"))
    # offset edges
    cs.append(("offset_max_pass",
               base(journal={"path": "/tmp/j.jsonl",
                             "offset": 9007199254740991}),
               0, True, None))
    cs.append(("offset_2p53_reject",
               base(journal={"path": "/tmp/j.jsonl",
                             "offset": 9007199254740992}),
               1, False, "offset"))
    cs.append(("offset_neg_reject",
               base(journal={"path": "/tmp/j.jsonl", "offset": -1}),
               1, False, "offset"))
    cs.append(("offset_frac_reject",
               base(journal={"path": "/tmp/j.jsonl", "offset": 1.5}),
               1, False, "offset"))
    # relative journal path reject
    cs.append(("journal_relpath_reject",
               base(journal={"path": "tmp/j.jsonl", "offset": 0}),
               1, False, "absolute"))
    # non-ascii identity reject
    cs.append(("sessionid_unicode_reject", base(session_id="会话"),
               1, False, "session_id"))
    # version 2 reject
    cs.append(("version2_reject", base(version=2), 1, False, "version"))
    # offset 9007199254740993 reject
    cs.append(("offset_2p53p1_reject",
               base(journal={"path": "/tmp/j.jsonl",
                             "offset": 9007199254740993}),
               1, False, "offset"))
    # hash 63 reject
    cs.append(("hash63_reject", base(candidate_hash="a" * 63),
               1, False, "candidate_hash"))
    # cwd relative reject
    cs.append(("cwd_rel_reject", base(cwd="rel/path"),
               1, False, "cwd"))
    return cs

def cases2():
    """Cases needing raw string surgery (dup keys) or hand-built JSON."""
    cs = []
    # unknown / missing required field
    d = json.loads(base())
    d["bogus"] = 1
    cs.append(("unknown_field_reject", json.dumps(d, ensure_ascii=False),
               1, False, "unknown"))
    d = json.loads(base())
    del d["input"]
    cs.append(("missing_input_reject", json.dumps(d, ensure_ascii=False),
               1, False, "missing"))
    # duplicate journal: raw append so no dict collapse
    raw = base().rstrip()
    assert raw.endswith("}")
    dup = raw[:-1] + ',"journal":{"path":"/tmp/j2.jsonl","offset":0}}'
    cs.append(("dup_journal_reject", dup, 1, False, "duplicate"))
    # duplicate pending_queue: raw append
    dup = raw[:-1] + ',"pending_queue":[]}'
    cs.append(("dup_queue_reject", dup, 1, False, "duplicate"))
    return cs


def build_bin():
    """Build once with -o: run mode (no -o) drops .cx definitions (unisacc known defect)."""
    global BIN
    tmpdir = tempfile.mkdtemp(prefix="csih-state-bin-")
    BIN = os.path.join(tmpdir, "reload_state_check")
    p = subprocess.run(["/bin/sh", os.path.join(ROOT, "unisacc.com"),
                        "-include", os.path.join(APP, "json.h"), "-o", BIN,
                        "json.cx", "reload_state.c", "reload_state_cli.c"],
                       cwd=APP, capture_output=True, timeout=120)
    if p.returncode != 0:
        print("FAIL build: " + p.stderr.decode("utf-8", "replace")[-400:])
        sys.exit(1)
    return tmpdir

def main():
    bindir = build_bin()
    try:
        return run_all()
    finally:
        shutil.rmtree(bindir, ignore_errors=True)

def run_all():
    all_cases = cases() + cases2()
    passed = 0
    failed = 0
    for name, raw, code, ok, why in all_cases:
        good, msg = run_check(name, raw, code, ok, why)
        print(("PASS " if good else "FAIL ") + msg)
        if good:
            passed += 1
        else:
            failed += 1
    print("TOTAL: %d passed, %d failed, %d total" % (
        passed, failed, passed + failed))
    return 0 if failed == 0 and (passed + failed) > 0 else 1


if __name__ == "__main__":
    sys.exit(main())


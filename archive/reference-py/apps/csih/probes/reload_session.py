#!/usr/bin/env python3
"""Strict undeployed v2 automatic-execution state through the real session CLI."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

APP = Path(__file__).resolve().parents[1]
ROOT = APP.parents[1]
SOURCES = ["json.c", "reload_state.c", "reload_session_decode.c",
           "reload_session_encode.c", "reload_session_cli.c"]
WATCH = [APP / name for name in SOURCES] + [APP / "reload_session.h", ROOT / "unisacc.com"]


def hashes():
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in WATCH}


def main():
    before = hashes()
    state = dict(version=2, session_id="sess", handoff_id="h1",
                 candidate_hash="a" * 64, goal="目标", loop_on=True, loop_left=8,
                 cwd="/tmp", role="agent", peer="", input="输入",
                 journal=dict(path="/tmp/session.jsonl", offset=0),
                 pending_queue=["待办"], history=["历史"], history_pos=1,
                 history_browsing=False, history_draft="草稿")
    cases = []
    for value in (True, False):
        cases.append((str(value), json.dumps(dict(state, loop_on=value), ensure_ascii=False), value, None))
    missing = dict(state)
    del missing["loop_on"]
    cases.append(("missing", json.dumps(missing), None, "missing: loop_on"))
    for value in (0, 1):
        cases.append(("number%d" % value, json.dumps(dict(state, loop_on=value)), None, "loop_on: not bool"))
    raw = json.dumps(state)
    cases.append(("duplicate", raw[:-1] + ',"loop_on":false}', None, "loop_on: duplicate"))
    cases.append(("unknown", json.dumps(dict(state, bogus=True)), None, "unknown key: bogus"))
    cases.append(("true_zero", json.dumps(dict(state, loop_left=0)), True, None))
    missing = dict(state)
    del missing["loop_left"]
    cases.append(("missing_left", json.dumps(missing), None, "missing: loop_left"))
    cases.append(("fraction_left", json.dumps(dict(state, loop_left=1.5)), None, "loop_left: not integer"))
    for value in (9, 1e300):
        cases.append(("range_left%s" % value, json.dumps(dict(state, loop_left=value)), None, "loop_left: not number in 0..8"))
    try:
        with tempfile.TemporaryDirectory(prefix="csih-session-loop-") as tmp:
            fixture = Path(tmp) / "state.json"
            for name, raw, value, reason in cases:
                fixture.write_text(raw)
                result = subprocess.run(["/bin/sh", str(ROOT / "unisacc.com"), *SOURCES,
                                         "roundtrip", str(fixture)], cwd=APP,
                                        capture_output=True, timeout=4)
                out = result.stdout.decode("utf-8")
                assert not result.stderr, (name, result.stderr)
                if reason:
                    assert result.returncode == 1, (name, result.returncode, out)
                    assert out.strip() == "FAIL decode_v2 input: " + reason, (name, out)
                else:
                    assert result.returncode == 0 and out.startswith("PASS\n"), (name, result.returncode, out)
                    restored = json.loads(out.split("\n", 1)[1])
                    assert restored == json.loads(raw), (name, restored)
                    assert restored["loop_on"] is value, (name, restored)
                print("PASS " + name, flush=True)
    finally:
        assert hashes() == before, "source/compiler changed during probe"
    print("TOTAL: 12 passed; source/compiler hashes unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

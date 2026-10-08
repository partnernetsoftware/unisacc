#!/usr/bin/env python3
import hashlib, json, os, pathlib, shutil, signal, subprocess, sys, tempfile, threading
from http.server import BaseHTTPRequestHandler, HTTPServer

CWD = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # apps/csih
BIN = "/Users/wjc/repos/unisacc/unisacc.com"
SRC = ["agent.c", "agent_cli.c", "file.c", "edit.c", "shell.c", "json.c",
       "session.c", "net.c", "plugin.c", "agent"]

def make_handler(responses, counter):
    class H(BaseHTTPRequestHandler):
        def log_message(self, *a): pass
        def do_POST(self):
            n = int(self.headers.get("Content-Length", 0))
            self.rfile.read(n)
            i = counter[0]; counter[0] += 1
            body = responses[min(i, len(responses) - 1)]
            data = json.dumps({"choices": [{"message": {"content": body}}]}).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers(); self.wfile.write(data)
    return H

def run_case(name, responses, prompt, expect_reqs, expect_rc, pre=None, extra_env=None):
    counter = [0]
    httpd = HTTPServer(("127.0.0.1", 0), make_handler(responses, counter))
    port = httpd.server_address[1]
    t = threading.Thread(target=httpd.serve_forever, daemon=True); t.start()
    cwd = tempfile.mkdtemp(prefix="csih-probe-")
    trace = os.path.join(cwd, "trace.jsonl")
    env = dict(os.environ)
    home = os.path.join(cwd, "home"); os.mkdir(home, 0o700)
    env["HOME"] = home
    env["DEEPSEEK_API_KEY"] = "LOCAL_STUB_ONLY"
    env.pop("OPENAI_API_KEY", None)
    for k in ("http_proxy", "https_proxy", "HTTP_PROXY", "HTTPS_PROXY", "all_proxy", "ALL_PROXY"):
        env.pop(k, None)
    env["NO_PROXY"] = "127.0.0.1,localhost"
    env["no_proxy"] = "127.0.0.1,localhost"
    env["CSIH_ENDPOINT"] = "http://127.0.0.1:%d/v1/chat/completions" % port
    env["CSIH_MODEL"] = "stub"
    env["CSIH_CWD"] = cwd
    env["CSIH_TRANSCRIPT"] = trace
    env.pop("CSIH_ROLE", None)
    env.pop("CSIH_PEER", None)
    if name.startswith("watch"):
        env["CSIH_ROLE"] = "watch"
        env["CSIH_PEER"] = "0:csih-x"
    if extra_env: env.update(extra_env)
    if pre: pre(cwd)
    argv = ["/bin/sh", BIN] + SRC + [prompt]
    out = b""
    try:
        p = subprocess.Popen(argv, cwd=CWD, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, start_new_session=True)
        out, _ = p.communicate(timeout=9)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL); out, _ = p.communicate(); rc = -1
    finally:
        httpd.shutdown(); httpd.server_close(); t.join(timeout=2)
    reqs = counter[0]
    s = out.decode(errors="replace")
    ok = (rc == expect_rc) and (reqs == expect_reqs)
    reasons = []
    if rc != expect_rc: reasons.append("rc %d != %d" % (rc, expect_rc))
    if reqs != expect_reqs: reasons.append("reqs %d != %d" % (reqs, expect_reqs))
    if name == "red":
        if "unfinished" not in s: reasons.append("red missing 'unfinished'")
        if "slice row" not in s: reasons.append("red missing 'slice row'")
    if name.startswith("watch") and name != "watch_no_tools":
        if "unfinished" not in s: reasons.append("%s missing 'unfinished'" % name)
        if "peer mail not delivered" not in s:
            reasons.append("%s missing 'peer mail not delivered'" % name)
    if name in ("normal", "watch_no_tools"):
        if "agent FAILED" in s: reasons.append("%s x 'agent FAILED'" % name)
    if name in ("outcome_failed", "outcome_failed_last", "watch_failed") and "FAILED_ARTIFACT" not in s: reasons.append("failed body missing")
    if name in ("outcome_partial", "watch_partial") and "PARTIAL_ARTIFACT" not in s: reasons.append("partial body missing")
    if name == "outcome_legacy" and ("LEGACY_ARTIFACT" not in s or "unverified" not in s): reasons.append("legacy outcome missing")
    expected_cause = {
        "parsefail": "too many unparseable steps",
        "stop_no_answer": "go=stop without answer",
        "direct_stop_no_answer": "go=stop without answer",
        "action_budget": "MAX_ACTIONS",
        "round_budget": "MAX_ROUNDS after continue",
        "prior_answer_budget": "MAX_ROUNDS after continue",
        "invalid_judge": "invalid round-end judgment",
    }.get(name)
    if expected_cause and ("unfinished" not in s or expected_cause not in s):
        reasons.append("missing unfinished cause " + expected_cause)
    records = []
    try:
        with open(trace) as file: records = [json.loads(line) for line in file]
    except FileNotFoundError: pass
    if name in ("invalid_judge", "invalid_judge_recovery"):
        if not any(r.get("role") == "assistant" and r.get("text") == "INVALID_JUDGMENT" for r in records):
            reasons.append("invalid judgment actual content missing")
        if not any(r.get("name") == "error" and "invalid round-end judgment" in r.get("text", "") for r in records):
            reasons.append("invalid judgment error record missing")
        stops = [r for r in records if r.get("role") == "decision" and r.get("go") == "stop"]
        if len(stops) != (1 if name == "invalid_judge_recovery" else 0):
            reasons.append("invalid token synthesized a stop")
    ok = ok and not reasons
    print("%-14s reqs=%d rc=%d expect=(%s,%d) %s" %
          (name, reqs, rc, expect_reqs, expect_rc, "OK" if ok else "FAIL"))
    if reasons:
        print("   reasons:", "; ".join(reasons))
    if not ok and out:
        print(s[:1500])
    shutil.rmtree(cwd)
    return dict(name=name, ok=ok, rc=rc, requests=reqs, expected_requests=expect_reqs, expected_rc=expect_rc, stdout=s, reasons=reasons, records=records)

EXEC = '{"act":"exec","cmd":"true","why":"x"}'
ANSWER = '{"act":"answer","outcome":"completed","text":"done"}'
STOP = '{"go":"stop"}'

def main():
    def mk_red(cwd):
        os.makedirs(os.path.join(cwd, "apps", "csih"))
    failed = json.dumps(dict(act="answer", outcome="failed", text="FAILED_ARTIFACT"))
    partial = json.dumps(dict(act="answer", outcome="partial", text="PARTIAL_ARTIFACT"))
    legacy = json.dumps(dict(act="answer", text="LEGACY_ARTIFACT"))
    rows = [
        ("outcome_failed", [failed, STOP], "do it", 1, 1, None, None),
        ("outcome_partial", [partial, STOP], "do it", 1, 1, None, None),
        ("outcome_legacy", [legacy, STOP], "do it", 1, 1, None, None),
        ("outcome_failed_last", ['{"go":"continue"}'] * 7 + [failed], "do it", 8, 1, None, None),
        ("outcome_last_continue", ['{"go":"continue"}'] * 7 + [ANSWER, '{"go":"continue"}'], "do it", 9, 1, None, None),
        ("outcome_invalid", ['{"act":"answer","outcome":1,"text":"bad"}'], "do it", 3, 1, None, None),
        ("outcome_duplicate", ['{"act":"answer","outcome":"failed","outcome":"completed","text":"bad"}'], "do it", 3, 1, None, None),
        ("outcome_nul", ['{"act":"answer","outcome":"completed\\u0000failed","text":"bad"}'], "do it", 3, 1, None, None),
        ("watch_failed", [failed], "do watch", 1, 1, None, None),
        ("watch_partial", [partial], "do watch", 1, 1, None, None),
        ("normal", [EXEC, ANSWER, STOP], "do it", 3, 0, None, None),
        ("parsefail", ["UNPARSEABLE"], "do it", 3, 1, None, None),
        ("stop_no_answer", [STOP], "do it", 3, 1, None, None),
        ("direct_stop_no_answer", ['{"go":"continue"}', '{"go":"continue"}', STOP], "do it", 3, 1, None, None),
        ("last_round_answer", ['{"go":"continue"}'] * 7 + [ANSWER, STOP], "do it", 9, 0, None, None),
        ("action_budget", [EXEC], "do it", 16, 1, None, None),
        ("round_budget", ['{"go":"continue"}'], "do it", 8, 1, None, None),
        ("prior_answer_budget", [ANSWER] + ['{"go":"continue"}'] * 8, "do it", 9, 1, None, None),
        ("invalid_judge", [ANSWER, "INVALID_JUDGMENT"], "do it", 4, 1, None, None),
        ("invalid_judge_recovery", [ANSWER, "INVALID_JUDGMENT", STOP], "do it", 3, 0, None, None),
        ("no_tools_prose", ["直接答复", STOP], "不用工具，直接回答测试", 1, 1, None, None),
        ("red", ['{"act":"file","op":"write","path":"apps/csih/probe.c","text":"x"}', STOP],
         "write probe", 2, 1, mk_red, None),
        ("watch_answer", [ANSWER, ANSWER], "do watch", 2, 1, None, None),
        ("watch_stop", [STOP], "do watch", 1, 1, None, None),
        ("watch_no_tools", [ANSWER, STOP], "不用" + "工具，直接回答测试", 2, 0, None, None),
    ]
    # All commands below are harmless fakes; never invoke the real envelope.
    fake = {
        "watch_false_comment": "false # envelope 0:csih-x",
        "watch_echo_receipt": "echo 'envelope → 0:csih-x.0: 80 chars' # envelope 0:csih-x",
        "watch_false_true": "false; true # envelope 0:csih-x",
        "watch_wrong_peer": "echo 'envelope → 0:csih-x2.0: 80 chars' # envelope 0:csih-x2",
        "watch_exit1": "echo 'envelope → 0:csih-x.0: 80 chars'; exit 1 # envelope 0:csih-x",
        "watch_timeout": "sleep 2 # envelope 0:csih-x",
    }
    for name, cmd in fake.items():
        action = json.dumps(dict(act="exec", cmd=cmd, why="regression"), ensure_ascii=False)
        rows.append((name, [action, ANSWER, STOP], "do watch", 3, 1, None,
                     {"CSIH_EXEC_TIMEOUT_SEC": "1"} if name == "watch_timeout" else None))
    args = sys.argv[1:]
    receipt = "/tmp/csih-agent-loop-review.json"
    if "--receipt" in args:
        i = args.index("--receipt"); receipt = args[i+1]; args = args[:i] + args[i+2:]
    selected = set(args)
    if selected:
        assert selected <= {row[0] for row in rows}, "unknown case"
        rows = [row for row in rows if row[0] in selected]
    assert rows
    def hashes():
        files = sorted(p for p in pathlib.Path(CWD).rglob("*") if p.suffix in (".c", ".h", ".inc"))
        files += [pathlib.Path(BIN), pathlib.Path(__file__)]
        return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    before = hashes()
    r = [run_case(*row) for row in rows]
    after = hashes()
    passed = all(row["ok"] for row in r) and before == after
    pathlib.Path(receipt).write_text(json.dumps(dict(passed=passed, before=before, after=after, cases=r), ensure_ascii=False, indent=2))
    print("TOTAL", "PASS" if passed else "FAIL", len(rows))
    sys.exit(0 if passed else 1)

if __name__ == "__main__":
    main()

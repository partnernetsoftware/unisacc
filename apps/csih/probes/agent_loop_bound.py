#!/usr/bin/env python3
import json, os, shutil, subprocess, sys, tempfile, threading
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

def run_case(name, responses, prompt, expect_reqs, expect_rc, pre=None):
    counter = [0]
    httpd = HTTPServer(("127.0.0.1", 0), make_handler(responses, counter))
    port = httpd.server_address[1]
    t = threading.Thread(target=httpd.serve_forever, daemon=True); t.start()
    cwd = tempfile.mkdtemp(prefix="csih-probe-")
    trace = os.path.join(cwd, "trace.jsonl")
    env = dict(os.environ)
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
    if pre: pre(cwd)
    argv = ["/bin/sh", BIN] + SRC + [prompt]
    out = b""
    try:
        p = subprocess.run(argv, cwd=CWD, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=9)
        rc = p.returncode; out = p.stdout
    except subprocess.TimeoutExpired:
        rc = -1
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
    if name in ("watch_answer", "watch_stop"):
        if "unfinished" not in s: reasons.append("%s missing 'unfinished'" % name)
        if "peer mail not delivered" not in s:
            reasons.append("%s missing 'peer mail not delivered'" % name)
    if name in ("normal", "watch_no_tools"):
        if "agent FAILED" in s: reasons.append("%s x 'agent FAILED'" % name)
    ok = ok and not reasons
    print("%-14s reqs=%d rc=%d expect=(%s,%d) %s" %
          (name, reqs, rc, expect_reqs, expect_rc, "OK" if ok else "FAIL"))
    if reasons:
        print("   reasons:", "; ".join(reasons))
    if not ok and out:
        print(s[:1500])
    return ok

EXEC = '{"act":"exec","cmd":"true","why":"x"}'
ANSWER = '{"act":"answer","text":"done"}'
STOP = '{"go":"stop"}'

def main():
    def mk_red(cwd):
        os.makedirs(os.path.join(cwd, "apps", "csih"))
    r = []
    r.append(run_case("normal", [EXEC, ANSWER, STOP], "do it", 3, 0))
    r.append(run_case("red", [
        '{"act":"file","op":"write","path":"apps/csih/probe.c","text":"x"}',
        STOP], "write probe", 2, 1, mk_red))
    r.append(run_case("watch_answer", [ANSWER, ANSWER], "do watch", 2, 1))
    r.append(run_case("watch_stop", [STOP], "do watch", 1, 1))
    r.append(run_case("watch_no_tools", [ANSWER, STOP],
                      "不用" + "工具，直接回答测试", 2, 0))
    print("TOTAL", "PASS" if all(r) else "FAIL")
    sys.exit(0 if all(r) else 1)

if __name__ == "__main__":
    main()

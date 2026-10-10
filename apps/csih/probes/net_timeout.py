#!/usr/bin/env python3
"""Private one-second threshold exercises production timeout control, not a 60s wait."""
import hashlib, json, os, pathlib, shutil, signal, subprocess, sys, tempfile, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer
APP = pathlib.Path(__file__).resolve().parents[1]
ROOT = APP.parents[1]
TUI = "tui.c render.c term.c chat.c clock.c tools.c cols.cx home.cx file.c shell.c edit.c gate.c json.cx session.c agent.c plugin.c net.c reload_state.c reload_session_decode.c reload_session_encode.c reload_io.c reload_load.c reload_consume.c journal_checkpoint.c reload_owner.c csih_message.c csih_message_io.c context_index.c".split()
INCLUDES = ["-include", str(APP / "csih_cols.h"), "-include", str(APP / "csih_home.h"), "-include", str(APP / "json.h")]
CLI = "agent.c agent_cli.c cols.cx home.cx file.c edit.c shell.c json.cx session.c net.c plugin.c".split()

def hashes():
    paths = sorted(p for p in APP.rglob("*") if p.suffix in (".c", ".h", ".inc", ".cx"))
    paths += [ROOT / "unisacc.com", pathlib.Path(__file__)]
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}

def run(argv, cwd, env):
    start = time.monotonic()
    p = subprocess.Popen(argv, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        out, err = p.communicate(timeout=14)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL); out, err = p.communicate()
        raise AssertionError(("process exceeded14s", argv, out, err))
    return dict(argv=argv, rc=p.returncode, stdout=out.decode(errors="replace"), stderr=err.decode(errors="replace"), seconds=time.monotonic()-start)

def main():
    result = dict(before=hashes(), requests=[]); server = None; thread = None
    try:
        assert APP.joinpath("net.c").read_text().count("#define NET_TOTAL_SEC     60") == 1
        with tempfile.TemporaryDirectory(prefix="csih-net-timeout-") as tmp:
            private = pathlib.Path(tmp); app = private / "app"; app.mkdir()
            for rel in [pathlib.Path(p).relative_to(APP) for p in result["before"] if pathlib.Path(p).suffix in (".c", ".h", ".inc", ".cx")]:
                src = APP / rel; assert not src.is_symlink()
                dst = app / rel; dst.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(src, dst)
            compiler = private / "unisacc.com"; shutil.copy2(ROOT / "unisacc.com", compiler)
            home = private / "home"; home.mkdir(mode=0o700)
            (home / "env.jsonl").write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
            env = dict(os.environ, HOME=str(home), DEEPSEEK_API_KEY="LOCAL_STUB_ONLY", CSIH_ROLE="", CSIH_PEER="", CSIH_CWD=str(private), CSIH_TRANSCRIPT=str(private / "journal.jsonl"), CSIH_MODEL="local-stub", NO_PROXY="127.0.0.1,localhost", no_proxy="127.0.0.1,localhost")
            for key in ("OPENAI_API_KEY", "http_proxy", "https_proxy", "HTTP_PROXY", "HTTPS_PROXY", "all_proxy", "ALL_PROXY", "CDSH_ROLE", "CDSH_PEER", "CDSH_ENDPOINT", "CDSH_MODEL", "CDSH_CWD"):
                env.pop(key, None)
            # Complete selftest uses an unchanged private production snapshot (60s).
            result["tui_build"] = run(["/bin/sh", str(compiler), *INCLUDES, "-o", str(private / "tui"), *TUI], app, env)
            assert result["tui_build"]["rc"] == 0, result["tui_build"]
            result["tui_selftest"] = run([str(private / "tui"), "selftest"], app, env)
            gate = result["tui_selftest"]
            assert gate["rc"] == 0 and gate["stdout"].splitlines()[-1] == "selftest ok" and "FAIL" not in gate["stdout"] and not gate["stderr"], gate
            net = app / "net.c"; text = net.read_text(); assert text.count("#define NET_TOTAL_SEC     60") == 1
            net.write_text(text.replace("#define NET_TOTAL_SEC     60", "#define NET_TOTAL_SEC     1"))
            result["private_threshold"] = 1
            result["agent_build"] = run(["/bin/sh", str(compiler), *INCLUDES, "-o", str(private / "agent"), *CLI], app, env)
            assert result["agent_build"]["rc"] == 0, result["agent_build"]
            class Handler(BaseHTTPRequestHandler):
                def log_message(self, *args): pass
                def do_POST(self):
                    result["requests"].append(json.loads(self.rfile.read(int(self.headers["Content-Length"]))))
                    time.sleep(3)
                    body = b'{"choices":[{"message":{"content":"unused"}}]}'
                    try:
                        self.send_response(200); self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body)
                    except (BrokenPipeError, ConnectionResetError): pass
            server = HTTPServer(("127.0.0.1", 0), Handler)
            thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
            env["CSIH_ENDPOINT"] = "http://127.0.0.1:%d/v1/chat/completions" % server.server_address[1]
            result["agent_run"] = run([str(private / "agent"), "agent", "TIMEOUT_PROBE"], app, env)
            test = result["agent_run"]
            assert test["rc"] == 1 and len(result["requests"]) == 1, test
            # Default60s diagnostic is retained in private1s test; err=-6 proves timeout branch.
            assert "agent FAILED: model call timed out (60s) (err=-6)" in test["stdout"], test
            assert test["seconds"] < 5, test
            result["passed"] = True
    except Exception as error:
        result.update(passed=False, error=repr(error))
    finally:
        if server: server.shutdown(); server.server_close()
        if thread: thread.join(timeout=4); assert not thread.is_alive()
        result["after"] = hashes()
        if result["before"] != result["after"]: result.update(passed=False, freeze_error="production source/compiler changed")
        pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "/tmp/csih-net-timeout-review.json").write_text(json.dumps(result, ensure_ascii=False, indent=2))
    print("PASS private timeout branch (err=-6), one localhost request; production60s TUI selftest" if result.get("passed") else "FAIL " + result.get("error", result.get("freeze_error", "unknown")), flush=True)
    return 0 if result.get("passed") else 1
if __name__ == "__main__": raise SystemExit(main())

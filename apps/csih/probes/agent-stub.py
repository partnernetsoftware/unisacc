#!/usr/bin/env python3
"""
agent-stub.py — a scripted stand-in for the DeepSeek chat/completions API.

It returns the SAME JSON shape DeepSeek does:
    {"choices":[{"message":{"role":"assistant","content":"..."}}], ...}

so cdsh's agent.c can be exercised end-to-end with NO DNS, NO TLS, and NO API
key. The stub is stateless: it decides what to emit by inspecting the request's
messages, so two independent agent runs (gcc + unisacc) against one stub are
both correct.

Decision logic:
  - if the last user message asks the round-end question ("Decide only:
    continue")  -> emit {"go":"stop"}
  - elif there are no tool messages yet            -> emit an exec action
  - else                                          -> emit an answer

Run:  python3 agent-stub.py [port]      (default 8137)
"""

import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer


def is_tool_result(m):
    role = m.get("role")
    content = m.get("content") or ""
    # csih keeps role=tool on disk, but the wire form is a user turn
    # prefixed with [tool] so DeepSeek does not see a bare role=tool.
    if role == "tool":
        return True
    return role == "user" and content.startswith("[tool]\n")


def decide(messages):
    tool_count = sum(1 for m in messages if is_tool_result(m))
    last_user = None
    for m in reversed(messages):
        if m.get("role") == "user" and not is_tool_result(m):
            last_user = m.get("content", "")
            break
    if last_user and "Decide only: continue" in last_user:
        return '{"go":"stop"}'
    if tool_count == 0:
        return '{"act":"exec","cmd":"echo hello-from-agent"}'
    return '{"act":"answer","text":"完成了"}'


class Handler(BaseHTTPRequestHandler):
    def _send(self, obj):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        n = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(n) if n else b""
        try:
            req = json.loads(raw.decode("utf-8"))
            messages = req.get("messages", [])
        except Exception:
            messages = []
        content = decide(messages)
        self._send({"id": "stub", "choices": [
            {"message": {"role": "assistant", "content": content}}]})

    def log_message(self, *a):
        pass


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8137
    srv = HTTPServer(("127.0.0.1", port), Handler)
    srv.serve_forever()


if __name__ == "__main__":
    main()

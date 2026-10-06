#!/usr/bin/env python3
"""
deepseek-proxy.py — optional plaintext hop. Not required for TLS.

csih can POST https://api.deepseek.com directly: net.c spawns https_post.c,
which dlopens libcurl (unisacc 0.0.24). This proxy remains for a caller that
still speaks only http://127.0.0.1 and wants the key injected on the way out.

    csih (plaintext)  ──>  127.0.0.1:PROXY_PORT  ──>  https://api.deepseek.com

Usage:
    DEEPSEEK_API_KEY=sk-... python3 deepseek-proxy.py [port]   (default 8080)

Only stdlib is used, so it runs anywhere Python 3 does. It forwards ONLY the
chat/completions path; anything else returns 404.
"""

import json
import os
import sys
import urllib.request
from http.server import BaseHTTPRequestHandler, HTTPServer

UPSTREAM = "https://api.deepseek.com/v1/chat/completions"


class Handler(BaseHTTPRequestHandler):
    def _reply(self, status, obj):
        body = json.dumps(obj).encode("utf-8") if isinstance(obj, dict) else obj
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        key = os.environ.get("DEEPSEEK_API_KEY")
        if not key:
            self._reply(500, {"error": "DEEPSEEK_API_KEY not set"})
            return
        if self.path != "/v1/chat/completions" and self.path != "/chat/completions":
            self._reply(404, {"error": "only /v1/chat/completions is proxied"})
            return

        n = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(n) if n else b""
        req = urllib.request.Request(
            UPSTREAM, data=raw, headers={
                "Content-Type": "application/json",
                "Authorization": "Bearer " + key,
            }, method="POST")
        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                data = resp.read()
            self.send_response(resp.status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        except urllib.error.HTTPError as e:
            data = e.read()
            self.send_response(e.code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        except Exception as e:  # noqa: BLE001
            self._reply(502, {"error": "upstream error: %s" % e})

    def log_message(self, *a):
        pass


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    srv = HTTPServer(("127.0.0.1", port), Handler)
    print("deepseek-proxy listening on http://127.0.0.1:%d -> %s" % (port, UPSTREAM))
    srv.serve_forever()


if __name__ == "__main__":
    main()

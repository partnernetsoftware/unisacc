import http.server, sys, json
PORT = int(sys.argv[1])
class H(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        try: ln = int(self.headers.get("Content-Length", "0"))
        except ValueError: ln = 0
        self.rfile.read(ln)
        body = json.dumps({"p": 0.83, "text": "continue"}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, fmt, *args):
        return
http.server.HTTPServer(("127.0.0.1", PORT), H).serve_forever()

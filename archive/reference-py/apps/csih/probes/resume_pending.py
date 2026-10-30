#!/usr/bin/env python3
"""Actual localhost model/tool resume with a pending prompt and unsent draft."""
import hashlib, json, os, pathlib, subprocess, sys, tempfile, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer
import owned_session as ui

APP=ui.APP; ROOT=ui.ROOT
WATCH=ui.WATCH+[pathlib.Path(__file__)]
def hashes():return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in WATCH}

def main():
    before=hashes(); result={"before":before}; requests=[]; session=None
    responses=[dict(act="exec",cmd="pwd",why="verify snapshot cwd"),dict(act="answer",text="RESUME_DONE"),dict(go="stop")]
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            requests.append(json.loads(self.rfile.read(int(self.headers["Content-Length"]))))
            index=len(requests)-1
            if index==0:time.sleep(.5)
            body=json.dumps(dict(choices=[dict(message=dict(content=json.dumps(responses[min(index,2)])))])) .encode()
            self.send_response(200);self.send_header("Content-Type","application/json");self.send_header("Content-Length",str(len(body)));self.end_headers();self.wfile.write(body)
    server=HTTPServer(("127.0.0.1",0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="csih-resume-pending-") as tmp:
            folder=pathlib.Path(tmp).resolve();cwd=folder/"restored-cwd";cwd.mkdir(mode=0o700)
            wrong=folder/"wrong-cwd";wrong.mkdir(mode=0o700)
            home=folder/"private-home";home.mkdir(mode=0o700)
            journal=folder/"journal.jsonl";journal.write_bytes(b"");journal.chmod(0o600)
            other=folder/"unrelated.jsonl";other.write_bytes(b"UNCHANGED")
            state=folder/"state-v2.json";hash="b"*64;draft="未发送中文稿";suffix="-EDIT"
            snapshot=dict(version=2,session_id="sess",handoff_id="pending",candidate_hash=hash,
                          goal="RESTORED_GOAL",loop_on=True,loop_left=0,cwd=str(cwd),role="write",peer="0:restored-peer",
                          journal=dict(path=str(journal),offset=0),input=draft,pending_queue=["QUEUED_RESUME"],
                          history=["HISTORIC"],history_pos=1,history_browsing=False,history_draft="历史草稿")
            state.write_text(json.dumps(snapshot,ensure_ascii=False));state.chmod(0o600)
            ui.BINARY=folder/"candidate"
            build=subprocess.run(["/bin/sh",str(ROOT/"unisacc.com"),"-o",str(ui.BINARY),*ui.SRC],cwd=APP,capture_output=True,timeout=15)
            assert build.returncode==0,(build.stdout,build.stderr)
            env=dict(os.environ,CSIH_CWD=str(wrong),CSIH_ROLE="watch",CSIH_PEER="wrong-peer",CSIH_TRANSCRIPT=str(other),
                     CSIH_ENDPOINT="http://127.0.0.1:%d/v1/chat/completions"%server.server_address[1],CSIH_MODEL="local-stub")
            for key in ("http_proxy","https_proxy","HTTP_PROXY","HTTPS_PROXY","all_proxy","ALL_PROXY","OPENAI_API_KEY","DEEPSEEK_API_KEY"):env.pop(key,None)
            env.update(NO_PROXY="127.0.0.1,localhost",no_proxy="127.0.0.1,localhost",HOME=str(home),DEEPSEEK_API_KEY="LOCAL_STUB_ONLY")
            session=ui.Session(["resume-agent",str(folder),str(state),"sess","pending",hash],env)
            while len(requests)<1:session.pump(.1)
            session.send(suffix.encode())
            while len(requests)<3:session.pump(.1)
            session.until("RESUME_DONE");session.pump(.5)
            output=session.out.decode(errors="replace")
            records=[json.loads(line) for line in journal.read_text().splitlines()]
            result.update(requests=requests,output=output,journal=records)
            assert draft+suffix in output,"pending execution overwrote unsent draft or later editing"
            assert "csih · 写" in output,"owned title did not match restored role"
            assert len(requests)==3,"budget reset caused extra requests"
            users=[record for record in records if record.get("role")=="user"]
            assert len(users)==1 and users[0]["text"]=="QUEUED_RESUME",users
            system="\n".join(message.get("content","") for message in requests[0]["messages"] if message.get("role")=="system")
            assert "写手。同伴 0:restored-peer" in system,system
            tool=[record.get("text","") for record in records if record.get("role")=="tool"]
            assert any(text == "cwd=" + str(cwd) + "\nexit=0\n" + str(cwd) + "\n" for text in tool),tool
            assert other.read_bytes()==b"UNCHANGED" and not state.is_file()
            session.send(b"\x7f"*30);session.send(b"/export-state after\r")
            saved=json.loads(state.read_text())
            for key in ("goal","history","history_draft","cwd","role","peer"):assert saved[key]==snapshot[key],(key,saved[key])
            assert saved["pending_queue"]==[] and saved["loop_left"]==0 and saved["loop_on"] is False
            assert saved["input"]=="" # control command stripped; draft proof was live frame above
            session.send(b"/exit\r");session.finish();assert len(requests)==3;result["passed"]=True
            print("PASS draft and live edits retained; exactly3 local requests; actual pwd/role/peer/journal restored; budget naturally disarmed",flush=True)
    except Exception as error:
        result.update(passed=False,error=repr(error));print("FAIL",repr(error),flush=True)
    finally:
        if session:session.close()
        server.shutdown();server.server_close();thread.join(timeout=2)
        result["after"]=hashes();assert result["before"]==result["after"]
        pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "/tmp/csih-resume-pending-review.json").write_text(json.dumps(result,ensure_ascii=False,indent=2))
    return 0 if result.get("passed") else 1
if __name__=="__main__":raise SystemExit(main())

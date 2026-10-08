#!/usr/bin/env python3
"""Private PTY owned export/resume; no provider calls, mail, or existing windows."""
import fcntl, hashlib, json, os, pathlib, pty, select, signal, struct, subprocess, tempfile, termios, time
APP = pathlib.Path(__file__).resolve().parents[1]
ROOT = APP.parents[1]
SRC = "csih.c render.c term.c chat.c clock.c tools.c file.c shell.c edit.c gate.c json.c session.c agent.c plugin.c net.c".split()
WATCH = list(APP.glob("*.c")) + list(APP.glob("*.h")) + list(APP.glob("*.inc")) + [ROOT / "unisacc.com"]

def hashes(): return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in WATCH}

class Session:
    def __init__(self, args, env):
        self.master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 35, 120, 0, 0))
        self.p = subprocess.Popen([str(BINARY), *args], cwd=APP,
                                  env=env, stdin=slave, stdout=slave, stderr=slave, start_new_session=True)
        os.close(slave); self.out = b""; self.start = time.monotonic()
    def pump(self, seconds=.2):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            assert time.monotonic() - self.start < 15, "process exceeded15 seconds"
            if select.select([self.master], [], [], .03)[0]:
                try: data = os.read(self.master, 65536)
                except OSError: break
                if not data: break
                self.out += data
    def until(self, needle):
        while needle.encode() not in self.out:
            self.pump(.1)
            assert self.p.poll() is None, self.out.decode(errors="replace")
    def send(self, data): os.write(self.master, data); self.pump(.25)
    def finish(self, expected=0):
        while self.p.poll() is None: self.pump(.1)
        self.pump(.05); assert self.p.returncode == expected, self.out.decode(errors="replace")
    def close(self):
        if self.p.poll() is None:
            os.killpg(self.p.pid, signal.SIGKILL); self.p.wait(timeout=2)
        os.close(self.master)

def main():
    before = hashes(); sessions=[]
    try:
        with tempfile.TemporaryDirectory(prefix="csih-owned-") as tmp:
            global BINARY
            folder=pathlib.Path(tmp); cwd=folder / "cwd"; cwd.mkdir(mode=0o700)
            BINARY=folder / "owned-tui"
            build=subprocess.run(["/bin/sh",str(ROOT / "unisacc.com"),"-o",str(BINARY),*SRC],
                                 cwd=APP,capture_output=True,timeout=15)
            assert build.returncode==0,(build.stdout,build.stderr)
            assert BINARY.is_file(),"compiler did not produce executable"
            selftest=subprocess.run([str(BINARY),"selftest"],cwd=APP,capture_output=True,timeout=15)
            assert selftest.returncode==0 and b"selftest ok" in selftest.stdout and not selftest.stderr,(selftest.stdout,selftest.stderr)
            print("PASS compiled real unisacc candidate and full selftest",flush=True)
            env=dict(os.environ, CSIH_CWD=str(cwd), CSIH_ROLE="write", CSIH_PEER="0:owned-test",
                     CSIH_ENDPOINT="http://127.0.0.1:1/v1/chat/completions", CSIH_MODEL="private-no-network")
            env.pop("OPENAI_API_KEY", None); env.pop("DEEPSEEK_API_KEY", None)
            hash="a"*64; state=folder / "state-v2.json"; journal=folder / "journal.jsonl"
            owner=Session(["agent-owned",tmp,"sess",hash],env); sessions.append(owner); owner.until("owned session")
            old=journal.read_bytes(); inode=journal.stat().st_ino
            other=Session(["agent-owned",tmp,"sess",hash],env); sessions.append(other); other.finish(1)
            assert b"ownership lock unavailable" in other.out and journal.read_bytes()==old and journal.stat().st_ino==inode
            print("PASS second owner refused without journal write",flush=True)
            owner.send(b"/goal OWNEDGOAL\r"); owner.send("中文草稿".encode()); owner.send(b"\x1b[A")
            owner.send(b"\x7f"*30); owner.send(b"/export-state h1\r")
            assert state.is_file(),owner.out.decode(errors="replace")
            saved=json.loads(state.read_text()); assert saved["input"]=="" and saved["history"]==["/goal OWNEDGOAL"]
            assert saved["history_draft"]=="中文草稿" and saved["goal"]=="OWNEDGOAL"
            assert saved["cwd"]==str(cwd) and saved["role"]=="write" and saved["peer"]=="0:owned-test"
            assert saved["journal"]==dict(path=str(journal),offset=len(old)) and saved["loop_left"]==0 and saved["loop_on"] is False
            owner.send(b"/exit\r"); owner.finish(); print("PASS owned controls exported without sending/history pollution",flush=True)
            raw=state.read_bytes()
            for label, mutation, session in [("identity",None,"wrong"),("offset",dict(offset=1),"sess"),("binding",dict(path=str(folder/"other.jsonl")),"sess")]:
                bad=json.loads(raw); bad["journal"].update(mutation or {}); state.write_text(json.dumps(bad)); state.chmod(0o600)
                current=state.read_bytes(); child=Session(["resume-agent",tmp,str(state),session,"h1",hash],env); sessions.append(child); child.finish(1)
                assert state.read_bytes()==current and journal.read_bytes()==old
                if label=="binding": assert b"snapshot journal not bound" in child.out
                print("PASS rejected "+label+" preserves state",flush=True)
            state.write_bytes(raw); state.chmod(0o600)
            wrong=dict(env,CSIH_CWD=tmp,CSIH_ROLE="watch",CSIH_PEER="wrong")
            resumed=Session(["resume-agent",tmp,str(state),"sess","h1",hash],wrong); sessions.append(resumed); resumed.until("owned session")
            assert not state.is_file(),"commit must consume once"
            resumed.send(b"\x1b[B"); resumed.until("中文草稿"); resumed.send(b"\x7f"*30); resumed.send(b"/export-state h2\r")
            again=json.loads(state.read_text())
            for key in ("goal","history","history_draft","pending_queue","loop_on","loop_left","cwd","role","peer","journal"):
                assert again[key]==saved[key],(key,again,saved)
            assert again["input"]=="" and again["handoff_id"]=="h2"
            resumed.send(b"/exit\r"); resumed.finish(); print("PASS resume UI/context export and committed consume",flush=True)
    finally:
        for session in sessions: session.close()
        assert hashes()==before,"source/compiler changed"
    print("TOTAL PASS; no model turn dispatched; hashes unchanged")
if __name__=="__main__":main()

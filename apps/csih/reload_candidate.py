#!/usr/bin/env python3
"""Trusted-private-directory candidate builder/verifier, not authentication or handoff."""
import hashlib, json, os, pathlib, re, shutil, signal, stat, subprocess, sys, time, uuid
SUFFIXES={".c",".h",".inc"}
SOURCES="csih.c render.c term.c chat.c clock.c tools.c file.c shell.c edit.c gate.c json.c session.c agent.c plugin.c net.c".split()
AGENT="agent.c agent_cli.c file.c edit.c shell.c json.c session.c net.c plugin.c".split()
GATES=[("tui-selftest","candidate",SOURCES,"selftest ok"),("agent-selftest","agent-selftest",AGENT,"agent: all cases pass")]
SCHEMA="csih-candidate-v1"

def require(condition,message):
    if not condition:raise ValueError(message)
def sha(data):return hashlib.sha256(data).hexdigest()
def canonical(value):return json.dumps(value,sort_keys=True,separators=(",",":"),ensure_ascii=True).encode()
def trusted(path,directory=False,private=False):
    s=path.lstat();require(s.st_uid==os.getuid() and not stat.S_ISLNK(s.st_mode),"untrusted owner/symlink: "+str(path))
    require(stat.S_ISDIR(s.st_mode) if directory else stat.S_ISREG(s.st_mode),"wrong file kind: "+str(path))
    if private:require(stat.S_IMODE(s.st_mode)==(0o700 if directory else 0o600),"wrong private permissions: "+str(path))
    return s

def manifest(app,strict=False):
    trusted(app,True,strict); rows=[]
    def visit(folder):
        for child in sorted(folder.iterdir(),key=lambda p:p.name):
            s=child.lstat();require(not stat.S_ISLNK(s.st_mode),"source symlink: "+str(child))
            require(s.st_uid==os.getuid(),"source owner mismatch")
            if stat.S_ISDIR(s.st_mode):
                if strict:require(stat.S_IMODE(s.st_mode)==0o700,"frozen directory permissions mismatch")
                visit(child)
            elif stat.S_ISREG(s.st_mode):
                if child.suffix in SUFFIXES:
                    if strict:require(stat.S_IMODE(s.st_mode)==0o600,"frozen input permissions mismatch")
                    rows.append(dict(path=child.relative_to(app).as_posix(),sha256=sha(child.read_bytes())))
                elif strict:raise ValueError("unknown frozen input: "+str(child))
            else:raise ValueError("unsupported source file kind: "+str(child))
    visit(app);rows.sort(key=lambda row:row["path"])
    require(rows,"empty source manifest"); paths={row["path"] for row in rows}
    require(set(SOURCES+AGENT)<=paths,"missing required source")
    for row in rows:
        file=app/row["path"]
        for line in file.read_text().splitlines():
            m=re.match(r"^\s*#\s*include\s+(.+)$",line)
            if not m:continue
            directive=m[1]
            if directive.startswith("<"):continue # compiler-embedded standard headers
            quoted=re.match(r'^"([^"\\]+)"',directive)
            require(quoted is not None,"unsupported nonliteral include")
            target=(file.parent/quoted[1]).resolve()
            require(target.is_relative_to(app.resolve()),"include escapes frozen app")
            require(target.relative_to(app.resolve()).as_posix() in paths,"include outside supported .c/.h/.inc inputs")
    return rows

def atomic(path,data):
    tmp=path.parent/(".receipt-"+uuid.uuid4().hex)
    fd=os.open(tmp,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
    try:
        with os.fdopen(fd,"wb") as f:f.write(data);f.flush();os.fsync(f.fileno())
        require(not path.is_symlink(),"receipt symlink")
        try:path.lstat()
        except FileNotFoundError:pass
        else:raise ValueError("refusing existing receipt")
        os.rename(tmp,path)
        dfd=os.open(path.parent,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
        try:os.fsync(dfd)
        finally:os.close(dfd)
    finally:
        try:tmp.unlink()
        except FileNotFoundError:pass

def command(argv,cwd,env):
    p=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:out,err=p.communicate(timeout=14)
    except subprocess.TimeoutExpired:
        try:os.killpg(p.pid,signal.SIGKILL)
        except ProcessLookupError:pass
        p.communicate(timeout=1)
        raise ValueError("command timeout; process group killed")
    return p.returncode,out,err

def record_command(directory,name,kind,argv,cwd,env):
    rc,out,err=command(argv,cwd,env)
    for suffix,data in (("stdout",out),("stderr",err)):
        path=directory/(name+"-"+kind+"."+suffix);path.write_bytes(data);path.chmod(0o600)
    return dict(argv=argv,cwd=str(cwd),rc=rc,stdout_sha256=sha(out),stderr_sha256=sha(err)),out,err

def expected_command(directory,binary,sources,kind):
    if kind=="build":return ["/bin/sh",str(directory/"compiler.com"),"-o",str(directory/binary),*sources]
    return [str(directory/binary),"selftest"]

def build(source,compiler,root):
    source=pathlib.Path(source).absolute();compiler=pathlib.Path(compiler).absolute();root=pathlib.Path(root).absolute()
    trusted(source,True);trusted(compiler);trusted(root,True,True)
    source=source.resolve();compiler=compiler.resolve();root=root.resolve()
    require(not root.is_relative_to(source),"private root cannot be inside source tree")
    original=manifest(source); compiler_hash=sha(compiler.read_bytes())
    directory=root/("candidate-"+uuid.uuid4().hex);directory.mkdir(mode=0o700)
    app=directory/"app";app.mkdir(mode=0o700)
    for row in original:
        target=app/row["path"];parent=app
        for part in pathlib.PurePosixPath(row["path"]).parts[:-1]:
            parent=parent/part
            try:parent.mkdir(mode=0o700)
            except FileExistsError:trusted(parent,True,True)
        target.write_bytes((source/row["path"]).read_bytes());target.chmod(0o600)
    frozen=directory/"compiler.com";frozen.write_bytes(compiler.read_bytes());frozen.chmod(0o600)
    require(manifest(source)==original and manifest(app,True)==original and sha(frozen.read_bytes())==compiler_hash and sha(compiler.read_bytes())==compiler_hash,"inputs changed during freeze")
    digest=sha(canonical(original));home=directory/"private-home";home.mkdir(mode=0o700);tmp=directory/"tmp";tmp.mkdir(mode=0o700)
    # Existing idle /reload selftest expects an openable file; dummy only.
    keyfile=home/"env.jsonl";keyfile.write_text('{"deepseek":{"api_key":"LOCAL_GATE_ONLY"}}\n');keyfile.chmod(0o600)
    env=dict(PATH=os.defpath,HOME=str(home),TMPDIR=str(tmp),LC_ALL="C",CSIH_ROLE="",CSIH_PEER="",
             CSIH_ENDPOINT="http://127.0.0.1:1/v1/chat/completions",CSIH_MODEL="candidate-gate",DEEPSEEK_API_KEY="LOCAL_GATE_ONLY",NO_PROXY="127.0.0.1,localhost",no_proxy="127.0.0.1,localhost")
    gates=[]
    for name,binary,sources,marker in GATES:
        build_record,out,err=record_command(directory,name,"build",expected_command(directory,binary,sources,"build"),app,env)
        require(build_record["rc"]==0,"candidate build failed: "+name)
        artifact=directory/binary;trusted(artifact);artifact.chmod(0o700);artifact_hash=sha(artifact.read_bytes())
        gate_record,out,err=record_command(directory,name,"run",expected_command(directory,binary,sources,"run"),app,env)
        require(gate_record["rc"]==0 and not err and b"FAIL" not in out and out.rstrip().splitlines()[-1:]==[marker.encode()],"candidate gate failed: "+name)
        require(manifest(app,True)==original and sha(frozen.read_bytes())==compiler_hash and sha(artifact.read_bytes())==artifact_hash,"frozen input/artifact changed during gate")
        gates.append(dict(name=name,binary=binary,binary_sha256=artifact_hash,build=build_record,run=gate_record))
    receipt=dict(schema=SCHEMA,source_manifest=original,source_sha256=digest,compiler_sha256=compiler_hash,
                 required_gates=[g[0] for g in GATES],gates=gates)
    atomic(directory/"receipt.json",canonical(receipt)+b"\n")
    return verify(directory)

def unique_object(pairs):
    result={}
    for key,value in pairs:
        require(key not in result,"duplicate receipt key")
        result[key]=value
    return result

def verify(directory):
    directory=pathlib.Path(directory).absolute();trusted(directory,True,True);directory=directory.resolve()
    trusted(directory.parent,True,True);trusted(directory/"receipt.json",False,True)
    allowed={"app","compiler.com","candidate","agent-selftest","receipt.json","private-home","tmp"}
    allowed.update(name+"-"+kind+"."+suffix for name,_,_,_ in GATES for kind in ("build","run") for suffix in ("stdout","stderr"))
    require({p.name for p in directory.iterdir()}<=allowed,"unknown candidate entry")
    for child in directory.iterdir():require(not child.is_symlink(),"candidate entry symlink")
    receipt=json.loads((directory/"receipt.json").read_bytes(),object_pairs_hook=unique_object)
    require(set(receipt)=={"schema","source_manifest","source_sha256","compiler_sha256","required_gates","gates"} and receipt["schema"]==SCHEMA,"receipt schema mismatch")
    rows=manifest(directory/"app",True)
    require(rows==receipt["source_manifest"] and sha(canonical(rows))==receipt["source_sha256"],"source manifest/hash mismatch")
    trusted(directory/"compiler.com",False,True);require(sha((directory/"compiler.com").read_bytes())==receipt["compiler_sha256"],"compiler hash mismatch")
    require(receipt["required_gates"]==[g[0] for g in GATES] and len(receipt["gates"])==len(GATES),"required gate set mismatch")
    for item,(name,binary,sources,marker) in zip(receipt["gates"],GATES):
        require(set(item)=={"name","binary","binary_sha256","build","run"} and item["name"]==name and item["binary"]==binary,"gate identity mismatch")
        artifact_stat=trusted(directory/binary)
        require(stat.S_IMODE(artifact_stat.st_mode)==0o700,"artifact permissions not0700")
        require(sha((directory/binary).read_bytes())==item["binary_sha256"],"artifact hash mismatch")
        for kind in ("build","run"):
            rec=item[kind];require(set(rec)=={"argv","cwd","rc","stdout_sha256","stderr_sha256"},"unknown command record")
            require(rec["argv"]==expected_command(directory,binary,sources,kind) and rec["cwd"]==str(directory/"app") and type(rec["rc"]) is int and rec["rc"]==0,"command/rc mismatch")
            data={}
            for suffix in ("stdout","stderr"):
                path=directory/(name+"-"+kind+"."+suffix);trusted(path,False,True);data[suffix]=path.read_bytes()
                require(sha(data[suffix])==rec[suffix+"_sha256"],"gate output hash mismatch")
            if kind=="run":require(not data["stderr"] and b"FAIL" not in data["stdout"] and data["stdout"].rstrip().splitlines()[-1:]==[marker.encode()],"gate success mismatch")
    return dict(candidate_dir=str(directory),hash=receipt["source_sha256"],binary=str(directory/"candidate"),receipt=str(directory/"receipt.json"))

def main():
    try:
        if len(sys.argv)==5 and sys.argv[1]=="build":result=build(*sys.argv[2:])
        elif len(sys.argv)==3 and sys.argv[1]=="verify":result=verify(sys.argv[2])
        else:raise ValueError("usage: build SOURCE_APP COMPILER PRIVATE_ROOT | verify CANDIDATE_DIR")
        print(json.dumps(result,sort_keys=True));return 0
    except (ValueError,OSError,subprocess.SubprocessError,KeyError,TypeError) as error:
        print(json.dumps(dict(error=str(error)),sort_keys=True));return 1
if __name__=="__main__":raise SystemExit(main())

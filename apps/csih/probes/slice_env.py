#!/usr/bin/env python3
"""Real private edit->agent_slice environment isolation with expected-red old control."""
import hashlib, json, os, pathlib, re, shlex, shutil, signal, subprocess, sys, tempfile, time
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1]
SOURCES='agent.c agent_cli.c file.c edit.c shell.c json.c session.c net.c plugin.c'.split()
PREFIX='CSIH_ROLE= CSIH_PEER= exec '

def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}

def run(argv,cwd,env):
    p=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    start=time.monotonic()
    try:o,e=p.communicate(timeout=14)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);o,e=p.communicate();raise AssertionError(('process timeout14',argv,o,e))
    return dict(argv=argv,cwd=str(cwd),rc=p.returncode,stdout=o.decode(errors='replace'),stderr=e.decode(errors='replace'),seconds=time.monotonic()-start)

def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'new';assert mode in ('new','old')
    result=dict(mode=mode,before=hashes());started=time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix='csih-slice-env-') as tmp:
            root=pathlib.Path(tmp).resolve();app=root/'apps/csih';app.mkdir(parents=True,mode=0o700)
            # Resample once if a concurrent authorized edit prevented a coherent copy.
            for attempt in range(2):
                before=hashes()
                if attempt:shutil.rmtree(app);app.mkdir(parents=True,mode=0o700)
                for srcpath in before:
                    src=pathlib.Path(srcpath)
                    if src.suffix in ('.c','.h','.inc'):
                        assert not src.is_symlink();dst=app/src.relative_to(APP);dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(src.read_bytes())
                compiler=root/'unisacc.com';shutil.copy2(ROOT/'unisacc.com',compiler);compiler.chmod(0o700)
                after=hashes()
                if before==after:result['before']=before;break
            else:raise AssertionError('production changed across both snapshot attempts')
            result['snapshot_attempts']=attempt+1
            assert '#define' in (app/'tui.c').read_text()
            text=(app/'agent.c').read_text();assert text.count(PREFIX)==2,'source not frozen at expected two-prefix fix'
            result['production_prefix_count']=text.count(PREFIX)
            if mode=='old':
                text=text.replace(PREFIX,'exec ');assert text.count(PREFIX)==0;(app/'agent.c').write_text(text)
            # Only private CLI gets this test entry. Production ABI/types/functions are reused.
            cli=app/'agent_cli.c';text=cli.read_text();anchor='    if (!strcmp(cmd, "selftest")) return agent_run_selftest();';assert text.count(anchor)==1
            entry=r"""    if (!strcmp(cmd, "slice-env")) {
        agent_step step;
        char cwd[1024], out[4096];
        int rc;
        if (argc != 3 || !getcwd(cwd, sizeof cwd)) return 64;
        printf("parent-before role=%s peer=%s\n", getenv("CSIH_ROLE"), getenv("CSIH_PEER"));
        step = agent_parse(argv[2]);
        rc = agent_exec(&step, cwd, out, sizeof out);
        printf("agent_exec=%d\n%s\n", rc, out);
        printf("parent-after role=%s peer=%s\n", getenv("CSIH_ROLE"), getenv("CSIH_PEER"));
        return rc == 1 ? 0 : 1;
    }
"""
            cli.write_text(text.replace(anchor,entry+anchor))
            tui=app/'tui.c';text=tui.read_text();old='/* SLICE_ENV_PRIVATE_BEFORE */';new='/* SLICE_ENV_PRIVATE_AFTER */';assert old not in text and new not in text;tui.write_text(text+'\n'+old+'\n')
            home=root/'home';home.mkdir(mode=0o700);(home/'env.jsonl').write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
            env=dict(PATH=os.defpath,HOME=str(home),UNISACC=str(compiler),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ROLE='write',CSIH_PEER='0:private-peer',CSIH_EXEC_TIMEOUT_SEC='14',CSIH_ENDPOINT='http://127.0.0.1:1/unreachable',CSIH_MODEL='slice-no-model')
            result['compiler_sha256']=hashlib.sha256(compiler.read_bytes()).hexdigest();assert result['compiler_sha256']==result['before'][str(ROOT/'unisacc.com')]
            result['private_input_sha256']={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in app.rglob('*') if p.suffix in ('.c','.h','.inc')}
            binary=root/'private-cli';result['build']=run(['/bin/sh',str(compiler),'-o',str(binary),*SOURCES],app,env);assert result['build']['rc']==0,result['build']
            result['binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
            action=json.dumps(dict(act='file',op='edit',path='tui.c',old=old,new=new))
            result['edit']=run([str(binary),'slice-env',action],app,env);edit=result['edit'];assert edit['rc']==0,edit
            assert tui.read_text().count(new)==1 and old not in tui.read_text()
            for phase in ('before','after'):assert 'parent-%s role=write peer=0:private-peer'%phase in edit['stdout'],edit
            expect=0 if mode=='new' else 1
            assert 'slice tui rc=%d'%expect in edit['stdout'] and 'slice rows rc=' not in edit['stdout'],edit
            # agent_slice returns first lines only. Independently rerun its real row
            # to retain full genuine selftest body; never synthesize child output.
            prefix=PREFIX if mode=='new' else 'exec '
            rows_cmd=prefix+shlex.quote(str(compiler))+' suite.c suite_cli.c rows tui.c'
            result['rows']=run(['/bin/sh','-c',rows_cmd],app,env);assert result['rows']['rc']==0,result['rows']
            rows=[shlex.split(line) for line in result['rows']['stdout'].splitlines() if line.strip()];assert len(rows)==1 and rows[0][0]=='tui',rows
            gate_cmd=prefix+shlex.quote(str(compiler))+' '+shlex.join(rows[0][1:])
            result['full_slice']=run(['/bin/sh','-c',gate_cmd],app,env);gate=result['full_slice'];assert gate['rc']==expect,gate
            assert not gate['stderr'],gate
            if mode=='new':assert gate['stdout'].rstrip().endswith('selftest ok') and 'FAIL' not in gate['stdout'],gate
            else:assert 'FAIL log view want 41 got goal at 2' in gate['stdout'] and 'SELFTEST FAILED' in gate['stdout'],gate
            result.update(expected_red_control=(mode=='old'),body_source='independent rerun of actual suite_cli row (agent_slice note keeps only first line)',passed=True)
            assert time.monotonic()-started<55,'selector exceeded55'
    except Exception as error:result.update(passed=False,error=repr(error))
    finally:
        result['after']=hashes()
        if result['before']!=result['after']:result.update(passed=False,freeze_error='production source/compiler/probe changed')
        result['seconds']=time.monotonic()-started
        pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/tmp/csih-slice-env-'+mode+'-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print(('PASS isolated new slice' if mode=='new' else 'PASS expected-red old pollution control') if result.get('passed') else 'FAIL '+result.get('error',result.get('freeze_error','unknown')),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Real candidate gates and precise private-copy tamper rejection."""
import contextlib, importlib.util, json, pathlib, shutil, subprocess, sys, tempfile
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1]
spec=importlib.util.spec_from_file_location("candidate_driver",APP/"reload_candidate.py")
driver=importlib.util.module_from_spec(spec);spec.loader.exec_module(driver)

@contextlib.contextmanager
def private_work():
    path=tempfile.mkdtemp(prefix="csih-candidate-probe-")
    try:yield path
    except Exception:
        print("FAILED diagnostics retained at "+path,flush=True)
        raise
    else:shutil.rmtree(path)

def main():
    driver_hash=driver.sha((APP/"reload_candidate.py").read_bytes());probe_hash=driver.sha(pathlib.Path(__file__).read_bytes())
    before=driver.manifest(APP);compiler_hash=driver.sha((ROOT/"unisacc.com").read_bytes());report=[]
    try:
        with private_work() as tmp:
            root=pathlib.Path(tmp).resolve();private=root/"candidates";private.mkdir(mode=0o700)
            source=root/"source";source.mkdir(mode=0o700)
            for row in before:
                target=source/row["path"];target.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
                target.write_bytes((APP/row["path"]).read_bytes())
            bad=root/"bad";shutil.copytree(source,bad)
            file=bad/"csih.c";original=file.read_text();file.write_text(original+"\n@not_valid_c\n")
            try:driver.build(bad,ROOT/"unisacc.com",private)
            except ValueError as error:
                assert "candidate build failed" in str(error),str(error)
                assert all(not (p/"receipt.json").is_file() for p in private.iterdir())
                report.append("bad source refused; no ready receipt")
            else:raise AssertionError("bad source accepted")
            print("PASS "+report[-1],flush=True)
            first=driver.build(source,ROOT/"unisacc.com",private);candidate=pathlib.Path(first["candidate_dir"])
            assert driver.verify(candidate)==first
            cli=subprocess.run([sys.executable,str(APP/"reload_candidate.py"),"verify",str(candidate)],capture_output=True,timeout=5)
            assert cli.returncode==0 and not cli.stderr and json.loads(cli.stdout)==first,(cli.stdout,cli.stderr)
            report.append("first real build and CLI verify, both gates green");print("PASS "+report[-1],flush=True)
            # Preserve the original directory by rename; test a byte-identical
            # private copy at the SAME absolute path so strict argv still binds.
            for relative,reason in [("app/csih.c","source manifest/hash mismatch"),("compiler.com","compiler hash mismatch"),("candidate","artifact hash mismatch"),("candidate","artifact permissions not0700")]:
                backup=candidate.with_name(candidate.name+"-backup");candidate.rename(backup)
                try:
                    shutil.copytree(backup,candidate)
                    assert driver.verify(candidate)==first
                    path=candidate/relative
                    if reason=="artifact permissions not0700":path.chmod(0o600)
                    else:path.write_bytes(path.read_bytes()+b"\nTAMPER\n")
                    try:driver.verify(candidate)
                    except ValueError as error:assert str(error)==reason,str(error)
                    else:raise AssertionError("tampered input accepted")
                    report.append("tamper refused: "+relative+" / "+reason);print("PASS "+report[-1],flush=True)
                finally:
                    shutil.rmtree(candidate);backup.rename(candidate)
                assert driver.verify(candidate)==first
            file=source/"csih.c";original=file.read_text();assert "candidate probe version2" not in original
            file.write_text(original+"\n/* candidate probe version2: comment only */\n")
            second=driver.build(source,ROOT/"unisacc.com",private)
            assert driver.verify(second["candidate_dir"])==second and first["hash"]!=second["hash"]
            report.append("second comment-only version: distinct identity, both real gates green");print("PASS "+report[-1],flush=True)
            evidence=dict(passed=True,first_hash=first["hash"],second_hash=second["hash"],checks=report,driver_hash=driver_hash,probe_hash=probe_hash,compiler_hash=compiler_hash,source_before=before,source_after=driver.manifest(APP))
            pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "/tmp/csih-candidate-review.json").write_text(json.dumps(evidence,ensure_ascii=False,indent=2))
            print(json.dumps(dict(passed=True,first_hash=first["hash"],second_hash=second["hash"],checks=report)),flush=True)
    finally:
        assert driver.manifest(APP)==before and driver.sha((ROOT/"unisacc.com").read_bytes())==compiler_hash,"production source/compiler changed"
        assert driver.sha((APP/"reload_candidate.py").read_bytes())==driver_hash and driver.sha(pathlib.Path(__file__).read_bytes())==probe_hash,"driver/probe changed during run"
    return 0
if __name__=="__main__":raise SystemExit(main())

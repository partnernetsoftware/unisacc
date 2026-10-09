#!/usr/bin/env python3
"""Host readiness negative cases; no candidate construction or suite-result reuse."""
import contextlib, importlib.util, io, json, os, pathlib, tempfile, unittest, subprocess, sys, shutil
from unittest.mock import patch
SPEC=importlib.util.spec_from_file_location('hostcheck',pathlib.Path(__file__).with_name('hostcheck.py'))
h=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(h)

class HostChecks(unittest.TestCase):
    def test_execution_host_not_output_target(self):
        self.assertTrue(h.matches('any','Linux/x86_64'))
        self.assertTrue(h.matches('native-posix','Linux/arm64'))
        self.assertFalse(h.matches('darwin-arm64','Linux/x86_64'))
        self.assertFalse(h.matches('native-posix','Other/mips'))
        self.assertTrue(h.matches('darwin-x86_64','Darwin/arm64',True))
        self.assertFalse(h.matches('darwin-x86_64','Darwin/arm64',False))
        with patch.object(h,'host',return_value='Linux/x86_64'),contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(h.require_host('darwin-arm64','osx/arm64'),77)
        self.assertEqual(out.getvalue().strip(),'UNVERIFIED: required=darwin-arm64 observed=Linux/x86_64 missing=execution-host target=osx/arm64')
    def test_gate_coverage_and_partial_unknown(self):
        for com in (False,True):
            declared=h.declarations(com)
            self.assertGreater(len(declared),500)
            self.assertEqual(declared['lib-windows-gp']['required'],'any')
            self.assertEqual(declared['lib-windows-gp-native']['required'],'darwin-x86_64')
            self.assertEqual(declared['lib-windows-gp-native']['probe'],'native-msabi-bridge')
            self.assertEqual(declared['lib-windows-gp-native']['dependencies'],'clang-arch-x86_64')
            self.assertEqual(declared['lib-windows-imports']['required'],'native-posix')
            self.assertEqual(declared['exec-native-chain']['target'],'host')
            self.assertEqual(declared['csmithdiff-40']['dependencies'],'csmith')
            self.assertTrue(any(d['required']=='UNKNOWN' for d in declared.values()))
        self.assertIn('com-csmithdiff-40',h.declarations(True))
    def test_inventory_cannot_return_success_without_probes(self):
        r=subprocess.run([sys.executable,str(h.ROOT/'tests/bound.py'),'15',sys.executable,str(h.ROOT/'tests/hostcheck.py'),'--inventory','--suite','exec-native-chain'],capture_output=True,text=True)
        self.assertEqual(r.returncode,77,(r.stdout,r.stderr))
        self.assertTrue(r.stdout.splitlines()[-1].startswith('UNVERIFIED:'))
    def test_missing_tool_and_missing_header(self):
        with patch.object(h.shutil,'which',return_value=None):
            self.assertEqual(h.Checks().probe('cc'),'MISSING')
        with tempfile.TemporaryDirectory() as td,patch.dict(os.environ,{'CSMITH_INCLUDE':td}):
            with self.assertRaises(FileNotFoundError): h.csmith_include('/usr/bin/csmith')
    def test_explicit_headers_and_actual_identity_changes(self):
        with tempfile.TemporaryDirectory() as td:
            d=pathlib.Path(td);(d/'csmith.h').write_text('/*first*/')
            with patch.dict(os.environ,{'CSMITH_INCLUDE':td}): self.assertEqual(h.csmith_include('/not-used/csmith'),d)
            first=h.header_identity(d);(d/'other.h').write_text('/*added*/');self.assertNotEqual(first,h.header_identity(d))
            other=h.header_identity(d);(d/'csmith.h').write_text('/*changed*/');self.assertNotEqual(other,h.header_identity(d))
            exe=d/'cc';exe.write_text('one');first=h.tool_identity(exe,b'v');exe.write_text('two');self.assertNotEqual(first,h.tool_identity(exe,b'v'))
    def test_compiler_real_error_is_not_host_skip(self):
        with tempfile.TemporaryDirectory() as td:
            exe=pathlib.Path(td)/'badcc';exe.write_text('#!/bin/sh\nif [ "$1" = --version ]; then echo badcc; exit 0; fi\necho compile-failed >&2\nexit 1\n');exe.chmod(0o755)
            with patch.object(h.shutil,'which',return_value=str(exe)):
                self.assertEqual(h.Checks().probe('cc'),'FAILED_PROBE')
    def test_prediction_requires_complete_matching_readiness_identity(self):
        b={'host':'Linux/x86_64','input_identity':'new'};r={'wall_seconds':11,'limit_seconds':10}
        self.assertEqual(h.prediction(r,b,'Linux/x86_64','new'),'PREDICT_TIMEOUT')
        self.assertEqual(h.prediction(r,b,'Linux/arm64','new'),'UNKNOWN')
        self.assertEqual(h.prediction(r,b,'Linux/x86_64','old'),'UNKNOWN')
        self.assertEqual(h.prediction({'cpu_seconds':17,'limit_seconds':10},b,'Linux/x86_64','new'),'UNKNOWN')
        self.assertEqual(h.prediction({},b,'Linux/x86_64','new'),'UNKNOWN')
    def test_no_time_left_is_unknown(self):
        self.assertEqual(h.Checks(seconds=0).probe('cc'),'UNKNOWN')
    @unittest.skipUnless(h.host().startswith('Linux/'),'Linux negative host cases')
    def test_host_suites_reject_before_preparation(self):
        commands=[
            [sys.executable,'exec/c/librarystackcheck.py','missing.com','arm64'],
            [sys.executable,'exec/c/librarystackcheck.py','missing.com','x86_64'],
            [sys.executable,'tests/r10stackx86check.py'],
            [sys.executable,'exec/enc/windowshostbridgecheck.py','--part','native'],
            ['env','SELF_PART=bootstrap','TARGET=osx/arm64','UA=/nonexistent','sh','exec/pipeline/selfcheck.sh'],
            ['env','SELF_PART=bootstrap','TARGET=osx/x86_64','UA=/nonexistent','sh','exec/pipeline/selfcheck.sh']]
        for args in commands:
            r=subprocess.run([sys.executable,str(h.ROOT/'tests/bound.py'),'10',*args],cwd=h.ROOT,capture_output=True,text=True)
            self.assertEqual(r.returncode,77,(args,r.stdout,r.stderr))
            self.assertTrue(r.stdout.splitlines()[-1].startswith('UNVERIFIED: required='))
    def test_native_unknown_host_does_not_build_reference(self):
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)/'uname';p.write_text('#!/bin/sh\necho unsupported\n');p.chmod(0o755)
            r=subprocess.run([sys.executable,str(h.ROOT/'tests/bound.py'),'10','env','PATH='+td+':'+os.environ['PATH'],'UA=/nonexistent','sh','exec/c/nativecheck.sh'],cwd=h.ROOT,capture_output=True,text=True)
            self.assertEqual(r.returncode,77,(r.stdout,r.stderr))
            self.assertIn('observed=unsupported/unsupported',r.stdout)
    @unittest.skipUnless(h.host().startswith('Linux/'), 'Linux separates native host from cross-COFF')
    def test_coff_failure_is_not_hidden_by_native_unverified(self):
        clang=shutil.which('clang');objcopy=shutil.which('llvm-objcopy')
        if not clang or not objcopy:self.skipTest('cross-COFF tools unavailable')
        for broken in ('bridge.bin','arm-bridge.bin'):
            with self.subTest(broken=broken),tempfile.TemporaryDirectory() as td:
                d=pathlib.Path(td);(d/'clang').symlink_to(clang)
                wrapper=d/'llvm-objcopy'
                wrapper.write_text('#!'+sys.executable+'\nimport sys,subprocess,pathlib\n'
                    +'r=subprocess.run(['+repr(objcopy)+',*sys.argv[1:]])\n'
                    +'if r.returncode:sys.exit(r.returncode)\n'
                    +'for a in sys.argv[1:]:\n'
                    +' if a.startswith("--dump-section=.text="):\n'
                    +'  p=pathlib.Path(a.split("=",2)[2])\n'
                    +'  if p.name=='+repr(broken)+':p.write_bytes(b"bad")\n')
                wrapper.chmod(0o755)
                env=dict(os.environ,LLVM_BIN=td,UA=shutil.which('true'),JOBS='1',TERM_SH_INSIDE='1',GATE_BOUND='1')
                r=subprocess.run([sys.executable,str(h.ROOT/'tests/bound.py'),'15','sh','tests/gate.sh',
                    '--suite','lib-windows-gp','--suite','lib-windows-gp-native'],cwd=h.ROOT,env=env,capture_output=True,text=True)
                self.assertEqual(r.returncode,1,(r.stdout,r.stderr))
                self.assertRegex(r.stdout,r'(?m)^lib-windows-gp\s+rc=1\s')
                self.assertRegex(r.stdout,r'(?m)^lib-windows-gp-native\s+rc=77\s')

    def test_stat_does_not_hide_permission_or_enotdir(self):
        with patch.object(pathlib.Path,'stat',side_effect=PermissionError):
            with self.assertRaises(PermissionError):h.regular('x')
        with patch.object(pathlib.Path,'stat',side_effect=NotADirectoryError):
            with self.assertRaises(NotADirectoryError):h.regular('x')

if __name__=='__main__':unittest.main()

#!/usr/bin/env python3
"""Host readiness negative cases; no candidate construction or suite-result reuse."""
import contextlib, importlib.util, io, json, os, pathlib, tempfile, unittest
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
            self.assertEqual(declared['lib-windows-imports']['required'],'native-posix')
            self.assertEqual(declared['exec-native-chain']['target'],'host')
            self.assertEqual(declared['csmithdiff-40']['dependencies'],'csmith')
            self.assertTrue(any(d['required']=='UNKNOWN' for d in declared.values()))
        self.assertIn('com-csmithdiff-40',h.declarations(True))
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
    def test_stat_does_not_hide_permission_or_enotdir(self):
        with patch.object(pathlib.Path,'stat',side_effect=PermissionError):
            with self.assertRaises(PermissionError):h.regular('x')
        with patch.object(pathlib.Path,'stat',side_effect=NotADirectoryError):
            with self.assertRaises(NotADirectoryError):h.regular('x')

if __name__=='__main__':unittest.main()

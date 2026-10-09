#!/usr/bin/env python3
"""Resource admission negative cases; never starts a generator or reproduces OOM."""
import contextlib,io,pathlib,tempfile,unittest,sys,subprocess,os,shutil
from unittest.mock import patch
import seedmemory as m
class Memory(unittest.TestCase):
 def fake(self,root,mem=100,maxval='max',used=0,parent='max'):
  (root/'mem').write_text('MemAvailable: '+str(mem)+' kB\n')
  (root/'member').write_text('0::/a/b\n');c=root/'cg';(c/'a/b').mkdir(parents=True)
  (root/'mount').write_text('1 0 0:1 / '+str(c)+' rw - cgroup2 cgroup2 rw\n')
  for p,x,u in [(c/'a/b',maxval,used),(c/'a',parent,10)]:
   (p/'memory.max').write_text(str(x));(p/'memory.current').write_text(str(u))
  return root/'mem',root/'member',root/'mount'
 def test_low_mem_and_cgroup_and_parent(self):
  for mem,x,u,parent,want in [(1,'max',0,'max',1024),(100,2000,1500,'max',500),(100,'max',0,900,890),(100,2000,3000,'max',0)]:
   with tempfile.TemporaryDirectory() as d:self.assertEqual(m.available(*self.fake(pathlib.Path(d),mem,x,u,parent)),want)
 def test_missing_membership_is_unknown_not_unlimited(self):
  with tempfile.TemporaryDirectory() as d:
   paths=self.fake(pathlib.Path(d));(pathlib.Path(d)/'cg/a/b/memory.max').unlink()
   with self.assertRaises(FileNotFoundError):m.available(*paths)
 def test_unknown_never_becomes_77(self):
  with patch.object(m,'requirement',return_value=(None,'cold Python unknown')),patch.object(m,'available',side_effect=AssertionError('must not admit unknown')):
   self.assertEqual(m.assess('gen',['e3'])[0],2)
 def test_fresh_each_start_and_real_errors_not_swallowed(self):
  with patch.object(m,'requirement',return_value=(4096,'measured')),patch.object(m,'available',side_effect=[8192,1024]):
   self.assertEqual(m.assess('gen',['e3'])[0],0);self.assertEqual(m.assess('gen',['e3'])[0],77)
  with patch.object(m,'requirement',return_value=(4096,'measured')),patch.object(m,'available',side_effect=PermissionError):
   with self.assertRaises(PermissionError):m.assess('gen',['e3'])
 def test_known_group_with_warm_cache_and_cold_unknown(self):
  with tempfile.TemporaryDirectory() as d,patch.object(m,'reference_key',return_value=m.REFERENCE_KEYS['gen']):
   self.assertIsNone(m.requirement('gen',['tokenpp','tokenlex','warnlex'],d)[0])
   c=pathlib.Path(d)/'unisacc-seedgen';c.mkdir()
   for n in ['tokenpp','tokenlex','warnlex']:(c/('py-'+m.REFERENCE_KEYS['gen']+'-'+n+'.json')).write_text('{}')
   peak=(39+120+743)*1024**2
   self.assertEqual(m.requirement('gen',['tokenpp','tokenlex','warnlex'],d)[0],peak+512*1024**2)
   with patch.object(m,'reference_key',return_value='changed'):self.assertIsNone(m.requirement('gen',['tokenpp'],d)[0])
 def test_unknown_com_and_unmeasured_parse2_flags(self):
  self.assertIsNone(m.requirement('com',[])[0]);self.assertIsNone(m.requirement('parse2',['locations'])[0])
 def test_gate_low_memory_exits_before_generator(self):
  for low_group in (False,True):
   with self.subTest(low_cgroup=low_group),tempfile.TemporaryDirectory() as d:
    root=pathlib.Path(d);paths=self.fake(root,mem=1024*1024*32 if low_group else 1,maxval=100 if low_group else 'max')
    cache=root/'unisacc-seedgen';cache.mkdir()
    for name in m.SUITES['seedgen-3'][1]:(cache/('py-'+m.REFERENCE_KEYS['gen']+'-'+name+'.json')).write_text('{}')
    tools=root/'tools';tools.mkdir();wrapper=tools/'python3'
    wrapper.write_text('#!'+sys.executable+'\nimport sys,os,pathlib\n'
      +'if len(sys.argv)>1 and sys.argv[1].endswith("/tests/seedmemory.py"):\n'
      +' sys.path.insert(0,'+repr(str(m.ROOT/'tests'))+')\n import seedmemory as m\n read=m.available\n'
      +' m.available=lambda:read(*[pathlib.Path(p) for p in '+repr([str(p) for p in paths])+'])\n'
      +' sys.argv=sys.argv[1:]\n sys.exit(m.main())\n'
      +'os.execv('+repr(sys.executable)+',['+repr(sys.executable)+',*sys.argv[1:]])\n')
    wrapper.chmod(0o755)
    env=dict(os.environ,TMPDIR=d,PATH=str(tools)+':'+os.environ['PATH'],UA=shutil.which('true'),JOBS='1',TERM_SH_INSIDE='1',GATE_BOUND='1')
    r=subprocess.run([sys.executable,str(m.ROOT/'tests/bound.py'),'10','sh','tests/gate.sh','--suite','seedgen-3'],cwd=m.ROOT,env=env,capture_output=True,text=True)
    self.assertEqual(r.returncode,4,(r.stdout,r.stderr));self.assertRegex(r.stdout,r'(?m)^seedgen-3\s+rc=77\s')
    self.assertIn('missing=memory',r.stdout)
    self.assertFalse(list(cache.glob('gen.*')),'generator must not start')

 def test_77_terminal_protocol(self):
  with patch.object(sys,'argv',['seedmemory.py','gen']),patch.object(m,'assess',return_value=(77,4096,100,'measured')),contextlib.redirect_stdout(io.StringIO()) as out:
   self.assertEqual(m.main(),77)
  self.assertEqual(out.getvalue().strip(),'UNVERIFIED: required=mem>=4096B observed=100B missing=memory')
if __name__=='__main__':unittest.main()

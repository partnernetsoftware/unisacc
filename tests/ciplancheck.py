#!/usr/bin/env python3
"""Check legacy CI coverage and dispatch without building/running a compiler."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
BASE = {'acceptance', 'vm', 'difftest', 'native', 'crossnative', 'fat',
        'artifacts', 'ccrun', 'selfhost', 'closure', 'run', 'c99', 'fuzz',
        'hostile', 'bench', 'consts', 'oracle', 'docs', 'kernel', 'tsvbuild',
        'gold_audit', 'prec_audit', 'tyinfo_audit', 'abi_audit', 'layout',
        'datashape', 'bigclosure', 'cli', 'diag', 'warn', 'opt', 'optpy',
        'difftest_o', 'scale', 'ape', 'multi', 'selfgap', 'stages',
        'nativeboot', 'acc'}
SHARDED = {'difftest', 'native', 'fat', 'ccrun', 'selfhost', 'closure',
           'stages', 'difftest_o'}


def check():
    # Fake only the dispatch runner. No test command or reference build runs.
    # The real all.sh supplies every argv; check its file partition directly.
    with tempfile.TemporaryDirectory(prefix='unisacc-ci-plan-') as name:
        tmp = Path(name)
        (tmp / 'bin').mkdir()
        runner = tmp / 'bin/python3'
        runner.write_text('#!' + sys.executable + '\n'
                          'import json,os,sys\n'
                          'with open(os.environ["CI_RECORD"],"a") as f:\n'
                          ' f.write(json.dumps(sys.argv[1:])+"\\n")\n')
        runner.chmod(0o755)
        record = tmp / 'calls.jsonl'
        env = dict(os.environ, PATH=str(tmp / 'bin') + ':' + os.environ['PATH'],
                   CI_RECORD=str(record), JOBS='128')
        proc = subprocess.run(['bash', 'tests/all.sh'], cwd=ROOT, env=env,
                              capture_output=True, text=True, timeout=15)
        assert proc.returncode == 0, proc.stdout + proc.stderr
        import json
        calls = [json.loads(line) for line in record.read_text().splitlines()]
        jobs = [args[2:] for args in calls if args[:1] == ['tests/bound.py']
                and len(args) > 2 and args[2] != 'bash']
        files = sorted(str(p.relative_to(ROOT)) for pat in ('examples/*.c', 'tests/c/*.c')
                       for p in ROOT.glob(pat))
        for suite in SHARDED - {'difftest', 'difftest_o'}:
            if suite == 'ccrun':
                chunks = [args[3:] for args in jobs if args[:3] == ['env', 'CCRUN_SHARD=1', './tests/ccrun.sh']]
            else:
                chunks = [args[1:] for args in jobs if args[0] == './tests/' + suite + '.sh' and not args[1].startswith('--prepare-')]
            assert len(chunks) == (17 if suite == 'closure' else 9 if suite == 'ccrun' else 8) and all(chunks), (suite, chunks)
            actual = [f for chunk in chunks for f in chunk]
            assert sorted(actual) == files and len(set(actual)) == len(files), suite
        for suite in ('difftest', 'difftest_o'):
            actual = [args[1] for args in jobs if args[-1] == './tests/' + suite + '.sh']
            assert sorted(actual) == ['SHARD=%d/4' % i for i in range(1, 5)], (suite, actual)
        plan = subprocess.run(['bash', 'tests/all.sh', '--list'], cwd=ROOT,
                              capture_output=True, text=True, timeout=5)
        assert plan.returncode == 0, plan.stderr
        names = plan.stdout.splitlines()
        assert names and len(names) == len(set(names)), names
        import re
        def family(name):
            for prefix in ('opt-', 'optpy-', 'ape-', 'acceptance', 'selfhost-', 'ccrun-', 'closure-'):
                if name.startswith(prefix):
                    return prefix.rstrip('-')
            return name if name in BASE else re.sub(r'\d+$', '', name)
        bases = {family(n) for n in names}
        assert BASE <= bases, BASE - bases
        if (ROOT / 'corpus/c-testsuite').is_dir():
            assert {'corpus1', 'corpus2', 'corpus3', 'corpus4'} <= set(names)
        if (ROOT / 'corpus/crypto-algorithms').is_dir():
            assert {'tools%d' % k for k in range(1,12)} <= set(names)
        # A selected job must not accidentally dispatch the whole plan.
        record.write_text('')
        proc = subprocess.run(['bash', 'tests/all.sh', '--suite', 'native1'],
                              cwd=ROOT, env=env, capture_output=True, text=True, timeout=5)
        assert proc.returncode == 0, proc.stderr
        # all.sh also calls `knownfail.py keys` through the faked python3 (0a8d265);
        # only bound.py dispatches count as jobs.
        selected = [json.loads(line) for line in record.read_text().splitlines()]
        selected = [args for args in selected if args[:1] == ['tests/bound.py']]
        assert sum('./tests/native.sh' in args for args in selected) == 1
        assert len(selected) == 2, selected  # one readiness check + one job
        record.write_text('')
        proc = subprocess.run(['bash', 'tests/all.sh', '--suite', 'native1',
                               '--suite', 'missing-suite'],
                              cwd=ROOT, env=env, capture_output=True, text=True, timeout=5)
        assert proc.returncode == 2, proc.returncode
        assert 'tests/bound.py' not in record.read_text(), 'unknown selection dispatched/prepared a job'
        # The aggregate must wait even for the slowest preparation job.
        runner.write_text('#!' + sys.executable + '\n' +
            'import os,sys,time,pathlib\n' +
            'a=sys.argv[1:]; root=pathlib.Path(os.environ["CI_BARRIER"])\n' +
            'if "--prepare" in a:\n' +
            ' target=a[a.index("--prepare")+1]\n' +
            ' time.sleep(.3 if target=="win/x86_64" else .01)\n' +
            ' (root/target.replace("/","_")).write_text("ready")\n' +
            'elif "./tests/ape.sh" in a:\n' +
            ' assert len(list(root.iterdir()))==5, "aggregate started before preparation"\n')
        barrier = tmp / 'barrier'; barrier.mkdir()
        env['CI_BARRIER'] = str(barrier)
        args = ['bash', 'tests/all.sh']
        for name in ['ape-prepare%d' % i for i in range(1, 6)] + ['ape']:
            args += ['--suite', name]
        proc = subprocess.run(args, cwd=ROOT, env=env,
                              capture_output=True, text=True, timeout=5)
        assert proc.returncode == 0, proc.stdout + proc.stderr
    expected = ['SHARD=%d/4' % i for i in range(1, 5)]
    assert sorted(expected[::-1]) == expected
    assert sorted(expected[:-1]) != expected
    assert sorted(expected[:-1] + [expected[0]]) != expected
    print('ci plan: legacy coverage kept; 6 file partitions, 2 SHARD families; selection controls ok')


if __name__ == '__main__':
    try:
        check()
    except (AssertionError, OSError, subprocess.TimeoutExpired) as exc:
        print('ci plan: FAIL ' + str(exc), file=sys.stderr)
        sys.exit(1)

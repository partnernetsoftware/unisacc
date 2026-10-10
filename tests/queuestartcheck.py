#!/usr/bin/env python3
"""queuestartcheck (0.0.40, 机房主任 20:22): a queue start is fresh by default and never restores a backup; resume is
explicit and bound to the candidate; every wrong, malformed or unknown source is refused (rc 2) with Q untouched."""
import hashlib, json, os, pathlib, shutil, subprocess, sys, tempfile
TOOL = pathlib.Path(__file__).resolve().parents[1] / 'release/tools/queuestart.py'
def fail(m): print('queuestart: ' + m); sys.exit(1)
T = pathlib.Path(tempfile.mkdtemp(prefix='unisacc-queuestart.'))
def run(Q, B, D, mode):
    r = subprocess.run([sys.executable, str(TOOL), str(Q), str(B), str(D), mode], capture_output=True, text=True, timeout=20)
    return r.returncode, r.stdout
def tree(p): return sorted((str(x.relative_to(p)), x.read_bytes() if x.is_file() else b'') for x in p.rglob('*')) if p.exists() else None
try:
    D = T / 'cand'; D.mkdir(); (D / 'unisacc-next.com').write_bytes(b'candidate'); art = hashlib.sha256(b'candidate').hexdigest()
    B = T / 'backup'; (B / 'state').mkdir(parents=True)
    (B / 'unisacc-next.com.build.json').write_text(json.dumps({'artifact_sha256': art}))
    good = {'stamp': {'a': 's'}, 'jobs': {'a': ['x']}, 'results': {'a': {'rc': 0}}}
    (B / 'state/results.json').write_text(json.dumps(good)); (B / 'state/a.log').write_text('old log\n')
    # fresh: the backup exists for this very artifact and is still not restored
    Q = T / 'q1'; rc, out = run(Q, B, D, 'fresh')
    if rc or (Q / 'results.json').exists() or json.loads((Q / 'start-receipt.json').read_text())['restored'] is not False: fail('fresh restored a backup: %s' % out)
    # fresh into a non-empty state dir is refused and leaves it untouched
    Q = T / 'q2'; Q.mkdir(); (Q / 'results.json').write_text('{"old": 1}'); before = tree(Q)
    rc, out = run(Q, B, D, 'fresh')
    if rc != 2 or tree(Q) != before: fail('fresh continued or changed a non-empty state dir: %s' % out)
    # resume from the matching backup: whole generation restored, receipt names the source
    Q = T / 'q3'; rc, out = run(Q, B, D, 'resume'); r = json.loads((Q / 'start-receipt.json').read_text())
    if rc or not r['restored'] or not r['source'].endswith('/state') or json.loads((Q / 'results.json').read_text()) != good: fail('matching resume failed: %s' % out)
    # resume continues an existing Q only when its receipt names this candidate
    rc, out = run(Q, B, D, 'resume')
    if rc or json.loads((Q / 'start-receipt.json').read_text())['restored'] is not False: fail('continuing a bound state failed: %s' % out)
    for label, setup in (
            ('backup for another artifact', lambda b: (b / 'unisacc-next.com.build.json').write_text(json.dumps({'artifact_sha256': '0' * 64}))),
            ('backup without build.json', lambda b: (b / 'unisacc-next.com.build.json').unlink()),
            ('malformed results.json', lambda b: (b / 'state/results.json').write_text('{not json')),
            ('results.json whose results is not a map', lambda b: (b / 'state/results.json').write_text(json.dumps({'results': []}))),
            ('no state generation', lambda b: shutil.rmtree(b / 'state'))):
        b = T / ('b-' + label.replace(' ', '-')); shutil.copytree(B, b); setup(b)
        Q = T / ('q-' + label.replace(' ', '-')); rc, out = run(Q, b, D, 'resume')
        if rc != 2 or Q.exists(): fail('%s not refused or Q touched: %s' % (label, out))
        if 'REFUSED' not in (b / 'start-refusals.log').read_text(): fail('%s: refusal not logged' % label)
    # existing state of unknown origin (no receipt) or bound to another candidate is refused untouched
    Q = T / 'q4'; Q.mkdir(); (Q / 'junk').write_text('x'); before = tree(Q); rc, out = run(Q, B, D, 'resume')
    if rc != 2 or tree(Q) != before: fail('unknown-origin state continued: %s' % out)
    Q = T / 'q5'; Q.mkdir(); (Q / 'start-receipt.json').write_text(json.dumps({'artifact_sha256': '1' * 64})); before = tree(Q); rc, out = run(Q, B, D, 'resume')
    if rc != 2 or tree(Q) != before: fail('state bound to another candidate continued: %s' % out)
    # a receipt-only Q (a previous empty fresh start) resumed from the backup: atomic import, no tmp left
    Q = T / 'q8'; run(Q, B, D, 'fresh'); rc, out = run(Q, B, D, 'resume')
    if rc or json.loads((Q / 'results.json').read_text()) != good or (T / 'q8.resume-tmp').exists(): fail('receipt-only Q resume failed: %s' % out)
    # receipt-only Q whose receipt is not an object ([]) is refused, untouched
    Q = T / 'q11'; Q.mkdir(); (Q / 'start-receipt.json').write_text('[]'); before = tree(Q); rc, out = run(Q, B, D, 'resume')
    if rc != 2 or tree(Q) != before: fail('receipt-only Q with a non-object receipt resumed: %s' % out)
    # a receipt holding JSON null (not missing) is refused too, untouched and logged; same for a null build.json
    Q = T / 'q14'; Q.mkdir(); (Q / 'start-receipt.json').write_text('null'); before = tree(Q); rc, out = run(Q, B, D, 'resume')
    if rc != 2 or tree(Q) != before or 'REFUSED' not in out: fail('null receipt resumed: %s' % out)
    for i, bad in enumerate(('0', '"s"', 'true', '{}', '{"artifact_sha256": 5}', '{"artifact_sha256": null}')):   # scalar / missing / mistyped field
        Q = T / ('q14m%d' % i); Q.mkdir(); (Q / 'start-receipt.json').write_text(bad); before = tree(Q); rc, out = run(Q, B, D, 'resume')
        if rc != 2 or tree(Q) != before: fail('receipt %s resumed: %s' % (bad, out))
        if 'REFUSED' not in out: fail('receipt %s refusal not reported: %s' % (bad, out))
        if not (B / 'start-refusals.log').exists() or ('%s has a start receipt' % Q) not in (B / 'start-refusals.log').read_text(): fail('receipt %s refusal not logged' % bad)
    for i, bad in enumerate(('0', '"s"', '{}', '{"artifact_sha256": 5}')):
        bb = T / ('b-build-%d' % i); shutil.copytree(B, bb); (bb / 'unisacc-next.com.build.json').write_text(bad)
        Q = T / ('q15m%d' % i); rc, out = run(Q, bb, D, 'resume')
        if rc != 2 or Q.exists(): fail('build.json %s not refused: %s' % (bad, out))
        if 'REFUSED resume' not in (bb / 'start-refusals.log').read_text(): fail('build.json %s refusal not logged' % bad)
    b = T / 'b-null-build'; shutil.copytree(B, b); (b / 'unisacc-next.com.build.json').write_text('null')
    Q = T / 'q15'; rc, out = run(Q, b, D, 'resume')
    if rc != 2 or Q.exists() or 'REFUSED' not in (b / 'start-refusals.log').read_text(): fail('null build.json not refused: %s' % out)
    # the final install rename fails: the old receipt-only Q comes back exactly, no tmp/aside left
    Q = T / 'q12'; run(Q, B, D, 'fresh'); before = tree(Q)
    code = ("import runpy,os,sys; real=os.replace\n"
            "def bad(a,b,**k):\n    if str(a).endswith('.resume-tmp'): raise OSError('injected rename failure')\n    return real(a,b,**k)\n"
            "os.replace=bad; import pathlib; pathlib.Path.replace=lambda self,t: bad(self,t) or pathlib.Path(t)\n"
            "sys.argv=[sys.argv[1]]+sys.argv[2:]; runpy.run_path(sys.argv[0], run_name='__main__')")
    r = subprocess.run([sys.executable, '-c', code, str(TOOL), str(Q), str(B), str(D), 'resume'], capture_output=True, text=True, timeout=20)
    if r.returncode != 2 or tree(Q) != before or (T / 'q12.resume-tmp').exists() or (T / 'q12.resume-old').exists():
        fail('failed install did not roll back to the old Q: rc %s %s %s' % (r.returncode, r.stdout, r.stderr[-300:]))
    # a Q that does not exist yet still resumes from a good backup (positive kept)
    Q = T / 'q13'; rc, out = run(Q, B, D, 'resume')
    if rc or json.loads((Q / 'results.json').read_text()) != good: fail('absent Q + good backup did not resume: %s' % out)
    # build.json that is valid JSON but not an object is refused cleanly
    b = T / 'b-list-build'; shutil.copytree(B, b); (b / 'unisacc-next.com.build.json').write_text('[]')
    Q = T / 'q9'; rc, out = run(Q, b, D, 'resume')
    if rc != 2 or 'REFUSED' not in out or Q.exists(): fail('non-object build.json not refused: %s' % out)
    # a bound receipt plus foreign files and no results.json is not a state this queue wrote
    Q = T / 'q10'; run(Q, B, D, 'fresh'); (Q / 'junk').write_text('x'); before = tree(Q); rc, out = run(Q, B, D, 'resume')
    if rc != 2 or tree(Q) != before: fail('bound receipt + foreign files continued: %s' % out)
    # a copy that fails midway restores nothing (no half state)
    b = T / 'b-unreadable'; shutil.copytree(B, b); os.chmod(b / 'state/a.log', 0)
    Q = T / 'q6'; rc, out = run(Q, b, D, 'resume'); os.chmod(b / 'state/a.log', 0o644)
    if os.geteuid() != 0 and (rc != 2 or Q.exists() or (T / 'q6.resume-tmp').exists()): fail('failed copy left a half state: %s' % out)
    # an unknown mode is a usage error
    if run(T / 'q7', B, D, 'auto')[0] != 2: fail('unknown mode accepted')
    print('queuestart  fresh never restores (backup for the same artifact present); fresh into a non-empty state refused untouched; explicit resume restores a bound generation with receipt, continues only a bound state; other-artifact/no build.json/malformed/non-map results/no generation/unknown-origin/other-candidate sources refused with Q untouched and logged; failed copy restores nothing; receipt-only Q resumes atomically; non-object (array/null/scalar) or field-less/mistyped build.json/receipt (each refusal reported; receipt and build.json refusals each logged) and bound-receipt-plus-foreign-files refused; a failed install rename rolls back to the old Q exactly')
finally:
    shutil.rmtree(T, ignore_errors=True)

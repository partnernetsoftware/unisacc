#!/usr/bin/env python3
"""queuestart.py Q BACKUP CANDIDATE_DIR MODE (0.0.40, 机房主任 20:22): the start mode of a release queue, decided
before any window runs.  0.0.39 B.5 (16:09:40) silently restored a same-artifact backup into a "clean" start.

  fresh  (default)  Q must be absent or empty; the backup is never read or copied; Q is created empty.
  resume (explicit) continue Q when it already holds a state whose start receipt names this candidate, or else
                    copy one complete backup generation (state, then state.old) whose build.json names this
                    candidate and whose results.json is a well-formed queue state.  Anything else is refused.

Every start writes Q/start-receipt.json (mode, restored, source, artifact, checks).  A refusal exits 2 and leaves
Q exactly as it was (never half-copied); the refusal reason is printed and appended to BACKUP/start-refusals.log
when the backup directory exists."""
import hashlib, json, os, pathlib, shutil, sys, time

def sha(p): return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()

def state_ok(d):
    try: data = json.loads((d / 'results.json').read_text())
    except (OSError, ValueError) as e: return 'results.json unreadable (%s)' % type(e).__name__
    if not isinstance(data, dict) or not isinstance(data.get('results'), dict): return 'results.json is not a queue state'
    if 'stamp' in data and not isinstance(data['stamp'], dict): return 'results.json stamp is not a map'
    return None

def main(argv):
    if len(argv) != 4 or argv[3] not in ('fresh', 'resume'):
        print('queue: QUEUE_START must be fresh or resume (got %r); usage Q BACKUP CANDIDATE_DIR fresh|resume' % (argv[3:] or [''])[0]); return 2
    Q, B, D, mode = pathlib.Path(argv[0]), pathlib.Path(argv[1]), pathlib.Path(argv[2]), argv[3]
    cand = D / 'unisacc-next.com'
    if not cand.is_file(): print('queuestart: candidate missing: %s' % cand); return 2
    art = sha(cand)
    def refuse(why):
        print('queue: REFUSED (%s start): %s' % (mode, why))
        if B.is_dir():
            with open(B / 'start-refusals.log', 'a') as f: f.write('%s REFUSED %s %s %s\n' % (time.strftime('%Y-%m-%dT%H:%M:%S%z'), mode, art[:16], why))
        return 2
    def receipt(restored, source, checks):
        Q.mkdir(parents=True, exist_ok=True)
        r = {'mode': mode, 'restored': restored, 'source': source, 'artifact_sha256': art, 'backup': str(B), 'checks': checks,
             'at': time.strftime('%Y-%m-%dT%H:%M:%S%z')}
        tmp = Q / '.start-receipt.tmp'; tmp.write_text(json.dumps(r, indent=1, sort_keys=True) + '\n'); tmp.replace(Q / 'start-receipt.json')
        (Q / 'start-receipt.txt').write_text('mode=%s\nrestored=%s\nsource=%s\nartifact_sha256=%s\nbackup=%s\n' % (mode, 'yes' if restored else 'no', source, art, B))
        if mode == 'fresh': print('queue: fresh start; backup %s left untouched (backup not read)' % B)
        elif restored: print('queue: resumed state from %s (receipt %s/start-receipt.json)' % (source, Q))
        else: print('queue: resume continues %s' % source)
        return 0
    try: prev_art = json.loads((Q / 'start-receipt.json').read_text()).get('artifact_sha256')
    except (OSError, ValueError): prev_art = None
    if mode == 'resume' and prev_art not in (None, art):
        return refuse('%s was started for artifact %s, not this candidate %s' % (Q, str(prev_art)[:16], art[:16]))
    held = Q.is_dir() and any(p.name not in ('start-receipt.json', 'start-receipt.txt') for p in Q.iterdir())   # a receipt alone is an empty start
    if mode == 'fresh':
        if held: return refuse('%s holds a live state already; a fresh start never continues it (QUEUE_START=resume to continue, or use a new QUEUE_STATE)' % Q)
        return receipt(False, 'none', ['Q absent or empty', 'backup not read'])
    if held:
        try: prev = json.loads((Q / 'start-receipt.json').read_text())
        except (OSError, ValueError): return refuse('%s holds a live state already but no readable start receipt (unknown origin); not mixed' % Q)
        if prev.get('artifact_sha256') != art: return refuse('%s was started for artifact %s, not this candidate %s' % (Q, str(prev.get('artifact_sha256'))[:16], art[:16]))
        bad = state_ok(Q)
        if bad and (Q / 'results.json').exists(): return refuse('%s: %s' % (Q, bad))
        return receipt(False, 'existing %s' % Q, ['receipt artifact matches', 'existing state well-formed or not yet written'])
    try: built = json.loads((B / 'unisacc-next.com.build.json').read_text())
    except (OSError, ValueError): return refuse('resume refused: backup %s has no build.json (unisacc-next.com.build.json)' % B)
    if built.get('artifact_sha256') != art: return refuse('resume refused: backup artifact mismatch -- %s is for artifact %s, not this candidate %s' % (B, str(built.get('artifact_sha256'))[:16], art[:16]))
    for g in ('state', 'state.old'):
        src = B / g
        if not src.is_dir(): continue
        bad = state_ok(src)
        if not (src / 'results.json').exists(): continue
        if bad: return refuse('backup generation %s: %s' % (src, bad))
        tmp = Q.with_name(Q.name + '.resume-tmp'); shutil.rmtree(tmp, ignore_errors=True)
        try: shutil.copytree(src, tmp, symlinks=True)
        except OSError as e:
            shutil.rmtree(tmp, ignore_errors=True)
            return refuse('copy of %s failed (%s); nothing restored' % (src, type(e).__name__))
        if Q.is_dir(): Q.rmdir()          # empty by the check above
        tmp.replace(Q)
        return receipt(True, str(src), ['backup build.json artifact matches', 'results.json well-formed', 'copied whole then renamed'])
    return refuse('backup %s has no complete state generation to resume' % B)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

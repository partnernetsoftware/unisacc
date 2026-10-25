#!/usr/bin/env python3
"""gatequeue input identity for tracked symlinks (git's view: a 120000 entry is its target string, never followed).

Cases: symlink to a directory, to a regular file, dangling; each identity is stable across calls and changes when the
target string changes; a real non-regular entry (FIFO) is still refused by name; a missing path is 'missing'."""
import hashlib, os, pathlib, subprocess, sys, tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gatequeue as gq

def sha(text): return hashlib.sha256(text.encode()).hexdigest()
fails = 0
def check(cond, msg):
    global fails
    print(('ok   ' if cond else 'FAIL ') + msg)
    fails += 0 if cond else 1

d = tempfile.mkdtemp(prefix='gqsymlink-'); cwd = os.getcwd(); os.chdir(d)
try:
    os.mkdir('dir'); pathlib.Path('file.txt').write_text('payload')
    os.symlink('dir', 'to_dir'); os.symlink('file.txt', 'to_file'); os.symlink('nowhere', 'dangling')
    check(gq.file_identity('to_dir') == [0o120000, sha('dir')], 'symlink to directory: identity is the target string')
    check(gq.file_identity('to_file') == [0o120000, sha('file.txt')], 'symlink to regular file: identity is the target string, not the content')
    check(gq.file_identity('dangling') == [0o120000, sha('nowhere')], 'dangling symlink: still a link, identity is the target string')
    check(gq.file_identity('to_dir') == gq.file_identity('to_dir'), 'symlink identity is stable across calls')
    subprocess.run(['sh', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cxbin'), os.path.join(os.path.dirname(os.path.abspath(__file__)), 'safedel.cx'), '--purge', '--', 'to_dir'], check=True); os.symlink('other', 'to_dir')   # the one deletion guard
    check(gq.file_identity('to_dir') == [0o120000, sha('other')], 'changing the link target changes the identity')
    check(gq.file_identity('file.txt')[0] == 0o100644, 'regular file keeps the git mode rule (0644 without execute bit)')
    check(gq.file_identity('absent') == ['missing'], 'missing path is an identity (missing)')
    os.mkfifo('pipe')
    try:
        gq.file_identity('pipe'); check(False, 'real non-regular entry is refused by name')
    except SystemExit as e:
        check('queue nonregular input: pipe' in str(e), 'real non-regular entry (FIFO) is refused by name')
finally:
    os.chdir(cwd)
print('gatequeue symlink identity: PASS' if fails == 0 else 'gatequeue symlink identity: FAIL %d' % fails)
sys.exit(1 if fails else 0)

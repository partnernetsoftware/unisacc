#!/usr/bin/env python3
"""docedit.py (0.0.40, 机房主任 20:22): append to a ledger/plan file without ever losing what it held.

  docedit.py append FILE TEXTFILE [--require HEADING]...   read FILE whole first, write FILE+TEXT to a temp file in
                                                           the same directory, fsync, atomically replace, re-read
  docedit.py verify FILE BEFORE  [--require HEADING]...    FILE must start with the bytes of BEFORE (a snapshot taken
                                                           before the edit) and contain every required heading

0586f01a wrote `open(p,'w').write(open(p).read()+note)`: the write-mode open truncated the file before the read, and
plans/v0.0.39.md went from 98 to 6 lines unnoticed.  Both subcommands fail closed (rc 1) when the old text is not
an exact prefix of the new text or a required heading is missing; rc 2 for usage errors."""
import argparse, os, sys, tempfile

def check(old, new, required, name):
    why = []
    if not new.startswith(old): why.append('%s: the previous %d bytes are not an exact prefix of the new %d bytes (truncated or rewritten)' % (name, len(old), len(new)))
    for h in required:
        if h.encode() not in new: why.append('%s: required heading missing: %s' % (name, h))
    return why

def main(argv=None):
    ap = argparse.ArgumentParser(); ap.add_argument('cmd', choices=('append', 'verify')); ap.add_argument('file'); ap.add_argument('other')
    ap.add_argument('--require', action='append', default=[])
    a = ap.parse_args(argv)
    if a.cmd == 'verify':
        old = open(a.other, 'rb').read(); new = open(a.file, 'rb').read()
    else:
        old = open(a.file, 'rb').read()                     # the whole file, BEFORE anything opens it for writing
        add = open(a.other, 'rb').read()
        new = old + (b'' if not old or old.endswith(b'\n') else b'\n') + add
        why = check(old, new, a.require, a.file)
        if why: print('\n'.join('docedit: REFUSED ' + w for w in why)); return 1
        d = os.path.dirname(os.path.abspath(a.file))
        fd, tmp = tempfile.mkstemp(dir=d, prefix='.docedit.')
        try:
            with os.fdopen(fd, 'wb') as f: f.write(new); f.flush(); os.fsync(f.fileno())
            os.chmod(tmp, os.stat(a.file).st_mode & 0o7777)
            os.replace(tmp, a.file)
        except BaseException:
            if os.path.exists(tmp): os.unlink(tmp)
            raise
        new = open(a.file, 'rb').read()
    why = check(old, new, a.require, a.file)
    for w in why: print('docedit: ' + w)
    if not why: print('docedit: %s ok (%d -> %d bytes, prefix kept, %d required headings present)' % (a.file, len(old), len(new), len(a.require)))
    return 1 if why else 0

if __name__ == '__main__':
    sys.exit(main())

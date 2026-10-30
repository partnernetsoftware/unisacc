#!/usr/bin/env python3
# Fixture for the context-index golden (dev only). Usage: context_index_fixture.py DIR
# Builds an owned mailbox under DIR with two notices in inbox/done, private modes,
# fixed mtimes. Notice A is short and multibyte; notice B exceeds the 256-byte preview.
import json, os, sys

SESSION = 'csih2'
NOTICES = [
    ('0123456789abcdef0123456789abcdef', 1700000100, 'hello 中文'),
    ('fedcba9876543210fedcba9876543210', 1700000200, 'x' * 300),
]

def envelope(mid, body):
    obj = {'version': 1, 'id': mid, 'session': SESSION, 'kind': 'notice', 'body': body}
    return json.dumps(obj, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode('utf-8')

def main():
    root = sys.argv[1]
    if not os.path.isabs(root):
        raise SystemExit('DIR must be absolute')
    os.makedirs(root, mode=0o700, exist_ok=True)
    os.chmod(root, 0o700)
    for d in ('inbox', 'inbox/ready', 'inbox/started', 'inbox/done'):
        p = os.path.join(root, d)
        os.makedirs(p, mode=0o700, exist_ok=True)
        os.chmod(p, 0o700)
    lock = os.path.join(root, 'inbox', 'mailbox.lock')
    open(lock, 'a').close()
    os.chmod(lock, 0o600)
    for mid, mtime, body in NOTICES:
        path = os.path.join(root, 'inbox', 'done', mid + '.json')
        with open(path, 'wb') as f:
            f.write(envelope(mid, body))
        os.chmod(path, 0o600)
        os.utime(path, (mtime, mtime))

if __name__ == '__main__':
    main()

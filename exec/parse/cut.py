import json, subprocess, sys
out = json.load(open('/tmp/claude-501/pre.json'))
lo = int(sys.argv[1]) if len(sys.argv) > 1 else 7
for k in range(lo, len(out)):
    src = '\n'.join(out[:k + 1]) + '\nint main() { return 0; }\n'
    f = '/tmp/claude-501/cut%d.c' % k
    open(f, 'w').write(src)
    r = subprocess.run(['python3', 'exec/parse/compare.py', '/tmp/e3delta.json', f], capture_output=True, text=True, timeout=50)
    last = r.stdout.strip().splitlines()
    res = [l for l in last if l.startswith('files')][0]
    print(k, out[k][:40], '|', res, '|', [l for l in last if 'reasons' in l or 'DIFF' in l][:2])
    if 'equal 1' not in res:
        break

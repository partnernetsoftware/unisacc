"""Private, bounded simulator checks for E1 provenance framing."""
import json
import pathlib
import struct
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import sim
KEY = b'\0library/sourcefacts'
MAGIC = b'USLFACT1\n'


class Files:
    def __init__(self, value):
        self.value = value

    def get(self, key):
        return self.value if key == KEY else None


def frame(payload, status=1, stage=2, policy=1):
    return MAGIC + bytes((stage, status, policy)) + struct.pack('<Q', len(payload)) + payload


def run(d, data, resource=None):
    result, val, _ = sim.run(d, data, files=Files(resource), maxsteps=300000)
    return result, val


def check():
    enabled = struct.pack('<Q', 1)
    counts = dict(default=0, valid=0, malformed=0, resource=0)
    with tempfile.TemporaryDirectory(prefix='e1-sourcefacts-') as td:
        models = {}
        for name, flags in [('plain', ['--sourcefacts']), ('positions', ['--positions']), ('locations', ['--locations'])]:
            path = pathlib.Path(td) / (name + '.json')
            subprocess.run([sys.executable, str(HERE.parent / 'build' / 'gen.py'), 'lex', str(path), *flags],
                           check=True, stdout=subprocess.DEVNULL, timeout=25)
            models[name] = json.loads(path.read_text())
            assert models[name]['start'] == 'SF.start'
        samples = [(b'', 1), (b'int x;', 1),
                   (b'int x __attribute__((aligned(16)));', 0),
                   (b'int x asm("x");', 0), (b'int x __asm__("x");', 0),
                   (b'__attribute__ int x;', 1), (b'__attribute__x int x;', 1),
                   (b'/* __attribute__((packed)) */ int x;', 1),
                   (b'char *x="__attribute__((packed))";', 1)]
        result, val = run(models['plain'], b'int x;')
        assert (result, val) == ('accept', b'type\nid=x\n;\neof\n4 tokens\n')
        for name, d in models.items():
            for text, clean in samples:
                payload = (b'UNIPP1\0' + struct.pack('<5I', len(text), 0, 0, 0, 0) + text
                           if name == 'locations' else text)
                result, expected = run(d, payload)
                assert result == 'accept', (name, text, result, expected)
                counts['default'] += 1
                if name == 'locations':
                    result, tokens = run(models['positions'], text)
                    assert result == 'accept'
                    assert expected == b'UNITOK1\0' + struct.pack('<I', len(payload)) + payload + tokens
                for sticky in (0, 1):
                    result, got = run(d, frame(payload, sticky), enabled)
                    assert (result, got) == ('accept', frame(expected, min(sticky, clean), stage=1)), (name, text, result, got)
                    counts['valid'] += 1
            good = frame(payload)
            bad = [good[:i] for i in range(20)] + [good + b'x', payload,
                   frame(payload, stage=1), frame(payload, status=2), frame(payload, policy=0),
                   b'BADFACT1\n' + good[9:], good[:12] + struct.pack('<Q', len(payload)+1) + payload,
                   good[:12] + struct.pack('<Q', 1 << 63) + payload]
            for b in bad:
                assert run(d, b, enabled)[0] == 'reject', (name, b)
                counts['malformed'] += 1
            for resource in (b'', b'1', bytes(8), struct.pack('<Q', 2), enabled + b'x'):
                assert run(d, frame(payload), resource)[0] == 'reject', (name, resource)
                counts['resource'] += 1
    print(json.dumps(counts, sort_keys=True))


if __name__ == '__main__':
    check()

#!/usr/bin/env python3
"""Construction-time package writer; the C runtime reads it without Python.
Manifest rows: route TAB stage TAB input-format TAB output-format TAB model.
Model paths are relative to their manifest. Equal network bytes are stored once.
"""
import argparse
import pathlib
import re
import zlib, hashlib, os, tempfile, fcntl
from networkformat import encode as binary_encode, decode as binary_decode, inflate
from tbl import OPS

NAME = re.compile(r'[A-Za-z0-9_./-]+\Z')



def _actions(flat, count):
    if count < 0 or count > len(flat):
        raise ValueError('bad Q action count')
    result, at = [], 0
    for _ in range(count):
        if at >= len(flat) or not 0 <= flat[at] < len(OPS):
            raise ValueError('bad Q opcode or truncated action')
        end = at + 1 + len(OPS[flat[at]][1])
        if end > len(flat):
            raise ValueError('truncated Q operands')
        if any(not -(2**63) <= x < 2**63 for x in flat[at:end]):
            raise ValueError('Q operand overflow')
        result.append(tuple(flat[at:end])); at = end
    if at != len(flat):
        raise ValueError('extra Q operands')
    return tuple(result)


def decode_q(lines):
    """Decode backward-only action prefixes; every copied item is an action."""
    decoded = []
    for qi, line in enumerate(lines):
        words = line.split()
        try:
            if words[0] == b'Q':
                actions = _actions(list(map(int, words[2:])), int(words[1]))
            elif words[0] == b'C':
                ref, prefix, tail = map(int, words[1:4])
                if not 0 <= ref < qi or not 0 < prefix <= len(decoded[ref]):
                    raise ValueError('bad Q prefix reference')
                actions = decoded[ref][:prefix] + _actions(list(map(int, words[4:])), tail)
            else:
                raise ValueError('unknown Q tag')
        except (IndexError, TypeError) as exc:
            raise ValueError('truncated Q record') from exc
        decoded.append(actions)
    return decoded


def compact_q(data):
    """Serialize repeated action prefixes; H/S bytes and expanded QA stay exact.

    C previousSequence prefixActions tailActions literalTailIntegers
    Only earlier sequences are referenced. This saves file bytes, not memory.
    """
    lines = data.splitlines(keepends=True)
    if not lines or not data.endswith(b'\n'):
        raise ValueError('truncated network')
    head = lines[0].split()
    if len(head) != 7 or head[0] != b'N':
        raise ValueError('bad network header')
    nq, nstr = int(head[2]), int(head[4])
    start = 1 + nstr
    if nq <= 0 or nstr < 0 or start + nq > len(lines):
        raise ValueError('bad network Q extent')
    old = lines[start:start+nq]
    decoded = decode_q(old)
    trie, wires = {}, []
    for qi, actions in enumerate(decoded):
        node, ref, prefix = trie, -1, 0
        for ai, action in enumerate(actions, 1):
            if action not in node:
                break
            node = node[action]; ref, prefix = node[None], ai
        wire = old[qi]
        if prefix:
            tail = [v for a in actions[prefix:] for v in a]
            fields = ['C', str(ref), str(prefix), str(len(actions)-prefix)]
            fields.extend(map(str, tail))
            candidate = (' '.join(fields) + '\n').encode('ascii')
            if len(candidate) < len(wire):
                wire = candidate
        wires.append(wire)
        node = trie
        for action in actions:
            node = node.setdefault(action, {}); node.setdefault(None, qi)
    if decode_q(wires) != decoded:
        raise ValueError('Q prefix round trip differs')
    return b''.join(lines[:start] + wires + lines[start+nq:])


def compressed_model(text, cache=True):
    raw = binary_encode(text)
    if [line.split() for line in binary_decode(raw).splitlines()] != [line.split() for line in text.splitlines()]:
        raise ValueError('binary network round trip differs')
    checksum = zlib.crc32(raw)
    def construct():
        c = zlib.compressobj(9, zlib.DEFLATED, -15)
        return c.compress(raw) + c.flush()
    if not cache:
        return construct(), len(raw), checksum
    key = hashlib.sha256(b'P3/raw-deflate9/UNINETB1/' + zlib.ZLIB_RUNTIME_VERSION.encode() + raw).hexdigest()
    base = pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE', tempfile.gettempdir()+'/unisacc-model-cache'))
    base.mkdir(parents=True, exist_ok=True)
    target = base/('codec-'+key)
    with (base/('codec-'+key+'.lock')).open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        try:
            blob = target.read_bytes()
            if inflate(blob,len(raw),checksum) != raw:
                raise ValueError('cached content differs')
        except (FileNotFoundError,ValueError,zlib.error):
            blob = construct()
            fd, name = tempfile.mkstemp(prefix='codec-', dir=base)
            try:
                with os.fdopen(fd,'wb') as f: f.write(blob)
                os.replace(name,target)
            finally:
                try: pathlib.Path(name).unlink()
                except FileNotFoundError: pass
    return blob, len(raw), checksum


def build(manifests, mounts=(), compressed=False, cache=True):
    stages, models, index, seen, last = [], [], {}, set(), {}
    for manifest in map(pathlib.Path, manifests):
        for line, text in enumerate(manifest.read_text().splitlines(), 1):
            if not text or text.startswith('#'):
                continue
            cols = text.split('\t')
            if len(cols) != 5 or not all(NAME.fullmatch(x) for x in cols[:4]):
                raise ValueError(f'{manifest}:{line}: expected five columns and valid names')
            route, stage, inp, out, path = cols
            if (route, stage) in seen:
                raise ValueError(f'{manifest}:{line}: duplicate stage {route}/{stage}')
            if route in last and last[route] != inp:
                raise ValueError(f'{manifest}:{line}: format mismatch {last[route]} -> {inp}')
            data = (manifest.parent / path).read_bytes()
            if not data.startswith(b'N ') or not data.endswith(b'\n'):
                raise ValueError(f'{manifest}:{line}: expected canonical network text')
            if data not in index:
                index[data] = len(models)
                models.append(data)
            stages.append((route, stage, inp, out, index[data]))
            seen.add((route, stage)); last[route] = out
    if not stages:
        raise ValueError('empty package')
    resources = {}
    for prefix, directory in mounts:
        keyprefix = bytes.fromhex(prefix)
        root = pathlib.Path(directory)
        if not root.is_dir():
            raise ValueError(f'resource mount is not a directory: {root}')
        files = sorted(p for p in root.rglob('*') if p.is_file())
        if not files:
            raise ValueError(f'empty resource mount: {root}')
        for path in files:
            key = keyprefix + path.relative_to(root).as_posix().encode('utf-8')
            data = path.read_bytes()
            if key in resources and resources[key] != data:
                raise ValueError(f'conflicting resource {key!r}')
            resources[key] = data
    head = (f'P 3 {len(models)} {len(stages)} {len(resources)}\n' if compressed else
            (f'P 2 {len(models)} {len(stages)} {len(resources)}\n' if resources
             else f'P 1 {len(models)} {len(stages)}\n'))
    head += ''.join('D ' + ' '.join(map(str, row)) + '\n' for row in stages)
    models = [compact_q(model) for model in models]
    if compressed:
        bodies = [compressed_model(b, cache) for b in models]
        wire = b''.join(f'M {len(b)} {n} 1 {crc}\n'.encode() + b for b,n,crc in bodies)
    else:
        wire = b''.join(f'M {len(b)}\n'.encode() + b for b in models)
    result = head.encode('ascii') + wire
    result += b''.join(f'F {len(k)} {len(v)}\n'.encode() + k + v for k, v in resources.items())
    if len(result) >= 2**31:
        raise ValueError('package exceeds runtime byte extent')
    return result


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('-o', '--output', required=True, type=pathlib.Path)
    ap.add_argument('--mount', nargs=2, action='append', default=[], metavar=('PREFIX_HEX', 'DIRECTORY'))
    ap.add_argument('--compressed', action='store_true', help='P3 binary networks with raw DEFLATE/CRC')
    ap.add_argument('--no-codec-cache', action='store_true')
    ap.add_argument('manifests', nargs='+', type=pathlib.Path)
    args = ap.parse_args()
    try:
        data = build(args.manifests, args.mount, args.compressed, not args.no_codec_cache)
        args.output.write_bytes(data)
    except (OSError, ValueError) as exc:
        ap.exit(1, f'pack: {exc}\n')
    print(f'package: {len(data)} bytes -> {args.output}')

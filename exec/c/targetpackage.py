#!/usr/bin/env python3
"""Select declared route/model/resource closure; never construct new answers.
Public non-target routes (currently tokens) remain available on every target.
All header resources remain: arbitrary user source may include any header.
"""
import argparse
import hashlib
import json
import pathlib
import struct
from packageformat import read_package

TARGETS = tuple(f'{os}/{arch}' for os in ('lnx','osx','win') for arch in ('arm64','x86_64'))

def sha(data):
    return hashlib.sha256(data).hexdigest()

def package_bytes(source):
    if source.startswith((b'P 1 ', b'P 2 ', b'P 3 ')):
        read_package(source)
        return source
    if source[-16:-8] != b'UNIPKG1\n':
        raise ValueError('unsigned package/bundle with terminal footer required')
    n = struct.unpack('<Q', source[-8:])[0]
    if not 0 < n <= len(source)-16:
        raise ValueError('package footer extent')
    data = source[-16-n:-16]
    read_package(data)
    return data

def write_package(package, rows=None, indices=None, resources=None):
    rows = package['rows'] if rows is None else rows
    indices = list(range(len(package['models']))) if indices is None else indices
    resources = package['resources'] if resources is None else resources
    renumber = {old:new for new,old in enumerate(indices)}
    version = package['version']
    if not rows or not indices:
        raise ValueError('empty selected package')
    head = f'P {version} {len(indices)} {len(rows)}'
    if version != 1:
        head += f' {len(resources)}'
    data = (head+'\n').encode()
    for row in rows:
        record = row[:5] + [str(renumber[int(row[5])]).encode()]
        data += b' '.join(record)+b'\n'
    for old in indices:
        wire = package['wires'][old]
        data += b' '.join(wire['record'])+b'\n'+wire['stored']
    for key,value in resources.items():
        data += f'F {len(key)} {len(value)}\n'.encode()+key+value
    read_package(data)
    return data

def select_target(data, target):
    if target not in TARGETS:
        raise ValueError('unknown target: '+target)
    source = read_package(data)
    prefix = target.encode()+b'/'
    foreign = tuple(t.encode()+b'/' for t in TARGETS)
    rows = [r for r in source['rows'] if r[1].startswith(prefix) or not r[1].startswith(foreign)]
    if not any(r[1].startswith(prefix) for r in rows):
        raise ValueError('target has no routes: '+target)
    indices = sorted({int(r[5]) for r in rows})
    arch = target.split('/')[1].encode()
    kernel = b'\0kernel/'+arch
    if kernel not in source['resources']:
        raise ValueError('target ISA kernel missing')
    resources = {k:v for k,v in source['resources'].items()
                 if not k.startswith(b'\0kernel/') or k == kernel}
    selected = write_package(source,rows,indices,resources)
    ledger = {'target':target,'scope':'all target CLI routes and all public routes',
              'source_sha256':sha(data),'package_sha256':sha(selected),
              'source_bytes':len(data),'package_bytes':len(selected),
              'source_networks':len(source['models']),'networks':len(indices),
              'routes':len({r[1] for r in rows}),'stage_rows':len(rows),
              'resources':len(resources),'kernel':kernel.decode(),
              'retained_models':[{'source_index':i,'text_sha256':sha(source['models'][i]),
                                  'stored_sha256':sha(source['wires'][i]['stored']),
                                  'bytes':len(source['wires'][i]['stored'])} for i in indices],
              'removed_network_bytes':sum(len(w['stored']) for i,w in enumerate(source['wires']) if i not in indices),
              'removed_resource_bytes':sum(len(v) for k,v in source['resources'].items() if k not in resources)}
    return selected,ledger

def export_shards(data, directory):
    """Content-addressed stored networks plus exact order/index manifest."""
    p=read_package(data); directory=pathlib.Path(directory); directory.mkdir(parents=True,exist_ok=True)
    records=[]
    for wire in p['wires']:
        digest=sha(wire['stored']); name=digest+'.network'
        path=directory/name
        try: prior=path.read_bytes()
        except FileNotFoundError: path.write_bytes(wire['stored'])
        else:
            if prior != wire['stored']: raise ValueError('content shard digest collision')
        records.append({'sha256':digest,'record':[v.decode('ascii') for v in wire['record']]})
    resources=[]
    for key,value in p['resources'].items():
        digest=sha(value); path=directory/(digest+'.resource')
        try: prior=path.read_bytes()
        except FileNotFoundError: path.write_bytes(value)
        else:
            if prior != value: raise ValueError('resource shard digest collision')
        resources.append({'key_hex':key.hex(),'sha256':digest})
    manifest={'schema':1,'version':p['version'],'package_sha256':sha(data),
              'rows':[[v.decode('ascii') for v in r] for r in p['rows']],
              'networks':records,'resources':resources}
    (directory/'package.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    return manifest

def import_shards(directory):
    directory=pathlib.Path(directory);m=json.loads((directory/'package.json').read_text())
    if m.get('schema') != 1: raise ValueError('unknown shard schema')
    def fetch(digest,suffix):
        if len(digest)!=64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('invalid shard digest')
        raw=(directory/(digest+suffix)).read_bytes()
        if sha(raw)!=digest: raise ValueError('changed shard')
        return raw
    p={'version':m['version'],'rows':[[v.encode('ascii') for v in r] for r in m['rows']],
       'models':[None]*len(m['networks']),
       'wires':[{'record':[v.encode('ascii') for v in n['record']],
                 'stored':fetch(n['sha256'],'.network')} for n in m['networks']],
       'resources':{bytes.fromhex(r['key_hex']):fetch(r['sha256'],'.resource') for r in m['resources']}}
    result=write_package(p)
    if sha(result)!=m['package_sha256']: raise ValueError('recombined package differs')
    return result

if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source',type=pathlib.Path)
    ap.add_argument('--target',choices=TARGETS)
    ap.add_argument('--shards',type=pathlib.Path)
    ap.add_argument('-o',required=True,type=pathlib.Path)
    a=ap.parse_args()
    try:
        data=package_bytes(a.source.read_bytes())
        if a.target: data,ledger=select_target(data,a.target)
        else: ledger={'package_sha256':sha(data),'package_bytes':len(data)}
        if a.shards:
            export_shards(data,a.shards)
            if import_shards(a.shards)!=data: raise ValueError('shard round trip differs')
        a.o.write_bytes(data)
        a.o.with_suffix(a.o.suffix+'.json').write_text(json.dumps(ledger,sort_keys=True,indent=2)+'\n')
        print(json.dumps(ledger,sort_keys=True))
    except (ValueError,OSError,KeyError,TypeError) as e: ap.exit(1,f'targetpackage: {e}\n')

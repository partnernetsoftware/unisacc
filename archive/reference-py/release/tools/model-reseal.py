#!/usr/bin/env python3
"""Observation recipe: invoke existing bounded model jobs; never rewrite frozen receipts.
The source/product/cache identities below are observations, not core-stage identity.
"""
import argparse, hashlib, json, os, pathlib, stat, subprocess, sys, time
READER=['tools','base-c','base-off','base-reference','base-tables-base','base-tables-off','base-net-base','base-net-off','audit','typed','check']
VARIANTS=['lnx-x','lnx-a','osx-x','osx-a','win-x','win-a','obj-lnx-x','obj-lnx-a','obj-osx-x','obj-osx-a','obj-win-x','obj-win-a']
SUFFIXES={'.c','.h','.cx','.sh','.py','.tsv','.json','.inc','.md'}
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()
def manifest(root):
    rows=[]
    for p in sorted(root.rglob('*')):
        mode=p.lstat().st_mode
        if stat.S_ISLNK(mode):raise ValueError('symlink is not a cache artifact: '+str(p))
        if stat.S_ISREG(mode):rows.append((p.relative_to(root).as_posix(),p.stat().st_size,sha(p)))
    return rows
def source_hash(root):
    h=hashlib.sha256()
    for name in ('exec','seed','unisa','include','src','kernel','weights','tests','release'):
        for p in sorted((root/name).rglob('*')):
            if '__pycache__' in p.parts or p.suffix not in SUFFIXES:continue
            mode=p.lstat().st_mode
            if stat.S_ISREG(mode):h.update(p.relative_to(root).as_posix().encode()+b'\0'+bytes.fromhex(sha(p)))
    return h.hexdigest()
def command(model,step):
    if model=='reader':
        if step not in READER:raise ValueError('unknown reader step: '+step)
        return ['sh','exec/parse2/librarytypedreadercheck.sh', 'check' if step=='check' else '--prepare-'+step]
    if step=='tools':tail=['--prepare-tools']
    elif step in VARIANTS:tail=['--prepare',step]
    elif step in ('1','2','3','4'):tail=[step]
    else:raise ValueError('unknown lower step: '+step)
    return ['sh','exec/lower/librarytypedcheck.sh',*tail]
def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--root',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[2]);a.add_argument('--model',choices=('reader','lower'),required=True);a.add_argument('--step');a.add_argument('--list',action='store_true');a.add_argument('--product',type=pathlib.Path);a.add_argument('--out',type=pathlib.Path);v=a.parse_args()
    if v.list:
        for step in READER if v.model=='reader' else ['tools',*VARIANTS,'1','2','3','4']:print(step+'\t'+json.dumps(command(v.model,step),separators=(',',':')))
        return 0
    if not v.step or not v.product or not v.out:a.error('--step, --product and --out required')
    root=v.root.resolve(strict=True);product=v.product.resolve(strict=True);out=v.out.resolve()
    if out.is_relative_to(root):a.error('--out must be outside the repository')
    out.mkdir(parents=True,exist_ok=True);label=v.model+'-'+v.step;cmd=command(v.model,v.step)
    source=source_hash(root);product_sha=sha(product);env=dict(os.environ,MODEL_COM=str(product))
    bounded=[sys.executable,str(root/'tests/bound.py'),'55',*cmd]
    start=time.monotonic()
    with (out/(label+'.stdout')).open('wb') as stdout,(out/(label+'.stderr')).open('wb') as stderr:
        rc=subprocess.call(bounded,cwd=root,env=env,stdout=stdout,stderr=stderr)
    elapsed=time.monotonic()-start
    if source_hash(root)!=source or sha(product)!=product_sha:
        print('STALE observation: source/product changed while running',file=sys.stderr);return 1
    cache_cmd=[sys.executable,str(root/'tests/bound.py'),'55','sh',cmd[1],'--cache-dir']
    q=subprocess.run(cache_cmd,cwd=root,env=env,capture_output=True,timeout=58)
    if q.returncode:sys.stderr.buffer.write(q.stderr);return q.returncode
    cache=pathlib.Path(q.stdout.decode().strip());info=cache.stat()
    if not stat.S_ISDIR(info.st_mode):raise ValueError('cache directory missing')
    entries=manifest(cache);payload=''.join(rel+'\t'+str(n)+'\t'+digest+'\n' for rel,n,digest in entries).encode();artifact=hashlib.sha256(payload).hexdigest()
    (out/(label+'.cache.tsv')).write_bytes(payload)
    record={'step':label,'argv':bounded,'rc':rc,'monotonic_s':elapsed,'product_sha':product_sha,'source_hash':source,'artifact_sha':artifact,'cache_path':str(cache),'producer_sha256':sha(pathlib.Path(__file__).resolve()),'scope':'observation only; cache lifecycle belongs to existing check.sh; no historical JSON rewritten'}
    (out/(label+'.json')).write_text(json.dumps(record,indent=2)+'\n')
    path=out/'steps.tsv'
    try:path.stat()
    except FileNotFoundError:path.write_text('step\targv\trc\tmonotonic_s\tproduct_sha\tsource_hash\tartifact_sha\n')
    row=[label,json.dumps(bounded,separators=(',',':')),str(rc),format(elapsed,'.6f'),product_sha,source,artifact]
    with path.open('a') as f:f.write('\t'.join(row)+'\n')
    print('\t'.join(row));return rc
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,ValueError,subprocess.TimeoutExpired) as e:print('observation mechanism failure: '+str(e),file=sys.stderr);sys.exit(2)

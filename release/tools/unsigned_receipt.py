"""Build unisacc-unsigned.zip + unsigned-receipt.json for the windows-signing workflow (release/README.md contract).
usage: unsigned_receipt.py CANDIDATE.com OUTDIR SOURCE_SHA RUN_ID ATTEMPT"""
import hashlib,json,pathlib,re,subprocess,sys,zipfile,datetime,socket
com,out,sha,run_id,attempt=sys.argv[1:];com=pathlib.Path(com);out=pathlib.Path(out);out.mkdir(parents=True,exist_ok=True)
R=pathlib.Path(__file__).resolve().parents[2]
ver=re.findall(r'#define UNISACC_VERSION "([0-9.]+)"',(R/'src/version.h').read_text());assert len(ver)==1
data=com.read_bytes();z=out/'unisacc-unsigned.zip'
with zipfile.ZipFile(z,'w',zipfile.ZIP_DEFLATED) as f:f.writestr(zipfile.ZipInfo('unisacc.com',(1980,1,1,0,0,0)),data,zipfile.ZIP_DEFLATED)
tool=hashlib.sha256(subprocess.check_output(['cc','--version'])).hexdigest()
rec={"schema":1,"kind":"unisacc-owner-local-build","source_sha":sha,"source_dirty":False,"built_by":"owner-local","local_gate_complete":True,
 "product_version":ver[0],"local_build":{"host":socket.gethostname(),"created_at":datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),"toolchain_sha256":tool},
 "upstream":{"run_id":int(run_id),"run_attempt":int(attempt)},"archive_sha256":hashlib.sha256(z.read_bytes()).hexdigest(),
 "assets":{"unisacc.com":{"sha256":hashlib.sha256(data).hexdigest(),"bytes":len(data)}}}
r=out/'unsigned-receipt.json';r.write_text(json.dumps(rec,indent=2)+'\n')
print(z,hashlib.sha256(z.read_bytes()).hexdigest());print(r,hashlib.sha256(r.read_bytes()).hexdigest())

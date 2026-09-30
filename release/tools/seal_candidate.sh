#!/bin/bash
# seal_candidate.sh DIR TAG NOTE -- push DIR's candidate to GHCR by tag, write release/candidate.json (digest-bound).
set -eu; D=$1; TAG=$2; NOTE=$3; R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); cp $D/unisacc-next.com $T/unisacc.com; cp $D/unisacc-next.com.build.json $T/unisacc.com.build.json; cp $D/model-audit/models.json $T/models.json
gh auth token | oras login ghcr.io -u "$(gh api user --jq .login)" --password-stdin >/dev/null
out=$(cd $T && oras push --format json ghcr.io/partnernetsoftware/unisacc-candidate:$TAG unisacc.com unisacc.com.build.json models.json)
digest=$(printf '%s' "$out" | python3 -c 'import sys,json;print(json.load(sys.stdin)["digest"])')
python3 - "$T" "$TAG" "$digest" "$NOTE" <<'PY'
import sys,json,hashlib,pathlib,subprocess
t,tag,digest,note=sys.argv[1:5];b=json.loads(pathlib.Path(t,'unisacc.com.build.json').read_text());raw=pathlib.Path(t,'unisacc.com').read_bytes()
assert hashlib.sha256(raw).hexdigest()==b['artifact_sha256']
ver=__import__('re').findall(r'"([0-9.]+)"',open('src/version.h').read())[0]
rec={'schema':1,'version':ver,'ref':f'ghcr.io/partnernetsoftware/unisacc-candidate@{digest}','tag':f'ghcr.io/partnernetsoftware/unisacc-candidate:{tag}','unisacc_com_sha256':b['artifact_sha256'],'bytes':len(raw),'sources_sha256':b['sources_sha256'],'sealed_from_commit':subprocess.check_output(['git','rev-parse','HEAD']).decode().strip(),'members':['unisacc.com','unisacc.com.build.json','models.json'],'note':note}
pathlib.Path('release/candidate.json').write_text(json.dumps(rec,indent=2)+'\n');print(json.dumps(rec)[:300])
PY
rm -rf $T

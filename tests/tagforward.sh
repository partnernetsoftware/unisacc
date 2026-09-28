#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
"$_BOUND" 60 python3 - "$UA" "$T" <<'PYTEST'
import subprocess,sys,pathlib
ua,tmp=sys.argv[1:]; prefix=(['sh'] if ua.endswith('.com') else [])+[ua]
forms=['return sizeof(*p);','return sizeof(struct S);','static int n=sizeof(struct S);return n;']
for i,form in enumerate(forms):
 f=pathlib.Path(tmp)/('bad%d.c'%i)
 f.write_text('struct S {int x;}; int main(void){struct S;struct S *p=0;'+form+'}')
 commands=[['cc','-fsyntax-only',str(f)],['python3','-m','unisa','tape',str(f)]]+[prefix+['-O%d'%n,'-S',str(f),'-o','-'] for n in range(3)]
 for cmd in commands:
  p=subprocess.run(cmd,capture_output=True,timeout=10)
  assert p.returncode==1 and b'incomplete' in p.stderr,(cmd,p.returncode,p.stderr)
print('tagforward: 15 compilation rejections; invalid inputs never executed')
PYTEST

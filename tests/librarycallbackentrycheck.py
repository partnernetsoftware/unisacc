#!/usr/bin/env python3
"""Entry/format selftest ONLY. No constructor, compiler, model, signature or auth is run."""
import argparse,json,pathlib,re
p=argparse.ArgumentParser();g=p.add_mutually_exclusive_group(required=True);g.add_argument('--reader',action='store_true');g.add_argument('--auth',action='store_true');a=p.parse_args();R=pathlib.Path(__file__).resolve().parents[1]
files=(['tests/librarycallbacke3check.sh','tests/librarycallbacke3check.cx','tests/librarycallbacke3capture.c'] if a.reader else ['tests/librarycallbackauthcheck.sh','tests/librarycallbackauthcheck.cx','tests/librarycallbackauthprobe.c','tests/librarycallbacksignaturecapture.c','tests/fixtures/librarycallback/auth-A.c','tests/fixtures/librarycallback/auth-B.c'])
for name in files:assert (R/name).is_file(),name
if a.reader:
 sh=(R/files[0]).read_text();assert '--cases tests/seals/librarycallbacke3/cases.json --checker tests/librarycallbacke3check.cx --run' in sh
 assert 'librarycallbacklowercheck.cx' in (R/files[1]).read_text()
 c=(R/files[2]).read_text();assert 'rf_identity_gate(input,inputn,2,' in c and 'observed_original_core_run' in c
 sample={'format':'CALLBACK_LOWER_CASES1','cases':[{'id':'format-only','part':1,'kind':'bytes','model':'models/on/model.tbl','stage':'e3:lnx/x86_64/tape/O0','input':{'state':'PRESENT','path':'input.bytes','sha256':'0'*64},'ledger':{'state':'PRESENT','path':'resources.tsv','sha256':'0'*64},'reference':{'state':'PRESENT','path':'expected.bytes','sha256':'0'*64}}]}
 q=json.loads(json.dumps(sample));assert q['cases'] and all(x['part']==1 for x in q['cases'])
 for key in ('input','ledger','reference'):assert re.fullmatch('[0-9a-f]{64}',q['cases'][0][key]['sha256'])
else:
 sh=(R/files[0]).read_text();assert '--auth' in sh and '--checker tests/librarycallbackauthcheck.cx' in sh
 c=(R/'tests/librarycallbackauthprobe.c').read_text();assert 'us_callables_init' in c and 'library_callable_make_dispatch' in c
 assert 'long' in (R/files[-2]).read_text() and 'double' in (R/files[-1]).read_text()
print(('reader' if a.reader else 'auth')+' entry/format selftest rc0; NO construction/model/auth/ABI PASS')

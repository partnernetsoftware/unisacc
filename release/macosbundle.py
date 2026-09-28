#!/usr/bin/env python3
"""Private macOS qualification slices; no notarization upload or release publish.
Each invocation is bounded to 55 seconds, each external step to 20 seconds.
Use build, sign, dmg, assess as separate invocations. A signed shell is not
an Apple signature on the unchanged inner APE, or a notarization result.
"""
import argparse, hashlib, json, os, plistlib, re, shutil, signal, subprocess, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
DEADLINE = 0

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def run(args, okay=True):
    budget = min(20, int(DEADLINE-time.monotonic())-2)
    if budget < 1: raise RuntimeError('55-second slice budget exhausted')
    p = subprocess.Popen(list(map(str, args)), stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try: out, err = p.communicate(timeout=budget)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL); p.communicate()
        raise RuntimeError('bounded step timed out: '+str(list(map(str,args))))
    result = {'argv': list(map(str, args)), 'returncode': p.returncode,
              'stdout': out.decode(errors='replace'), 'stderr': err.decode(errors='replace')}
    if okay and p.returncode: raise RuntimeError(json.dumps(result))
    return result

def main():
    global DEADLINE
    DEADLINE = time.monotonic()+55
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('phase', choices=['build','sign','dmg','assess'])
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--payload', type=Path)
    ap.add_argument('--sha256')
    ap.add_argument('--identity')
    ap.add_argument('--keychain', help='explicit private signing keychain; never changes login ACL')
    ns = ap.parse_args(); work = ns.work.resolve(); app = work/'Unisacc.app'
    entry = app/'Contents/MacOS/unisacc'; resource = app/'Contents/Resources/unisacc.com'
    recpath = work/'qualification.json'
    if ns.phase == 'build':
        if not ns.payload or not ns.sha256 or not re.fullmatch('[0-9a-f]{64}', ns.sha256):
            ap.error('build requires --payload and exact --sha256')
        try: work.stat()
        except FileNotFoundError: pass
        else: raise RuntimeError('work must be new; never reuse previous qualification artifacts')
        payload = ns.payload.resolve()
        if sha(payload) != ns.sha256: raise RuntimeError('source payload differs from required SHA256')
        work.mkdir(mode=0o700)
        (app/'Contents/MacOS').mkdir(parents=True); resource.parent.mkdir()
        shutil.copyfile(payload, resource); resource.chmod(0o755)
        (resource.parent/'payload.sha256').write_text(ns.sha256+'\n')
        versions = re.findall(r'#define UNISACC_VERSION "([0-9.]+)"', (ROOT/'src/version.h').read_text())
        if len(versions) != 1: raise RuntimeError('source version is ambiguous')
        version = versions[0]
        plist = {'CFBundleName':'Unisacc','CFBundleDisplayName':'Unisacc',
                 'CFBundleIdentifier':'com.partnernetsoftware.unisacc',
                 'CFBundleExecutable':'unisacc','CFBundlePackageType':'APPL',
                 'CFBundleShortVersionString':version,'CFBundleVersion':version,
                 'LSMinimumSystemVersion':'11.0',
                 'LSApplicationCategoryType':'public.app-category.developer-tools'}
        (app/'Contents/Info.plist').write_bytes(plistlib.dumps(plist))
        steps=[]
        for arch in ['arm64','x86_64']:
            steps.append(run(['/usr/bin/cc','-arch',arch,'-mmacosx-version-min=11.0',
                '-O2','-Wall','-Wextra','-Wno-deprecated-declarations',
                '-DPAYLOAD_SHA256="'+ns.sha256+'"',ROOT/'release/macos-launcher.c','-o',work/('launcher-'+arch)]))
        steps.append(run(['/usr/bin/lipo','-create',work/'launcher-arm64',work/'launcher-x86_64','-output',entry]))
        rec={'schema':1,'kind':'unisacc-private-macos-qualification','release_eligible':False,
             'source_sha':run(['git','-C',ROOT,'rev-parse','HEAD'])['stdout'].strip(),
             'payload_source':str(payload),'payload_sha256':ns.sha256,
             'launcher_before_sha256':sha(entry),'launcher_source_sha256':sha(ROOT/'release/macos-launcher.c'),
             'builder_source_sha256':sha(ROOT/'release/macosbundle.py'),
             'build':steps,'notarization':'pending: no submission','staple':'pending',
             'inner_ape_apple_signature':False,'scope':'CLI bundle adapter; private qualification only'}
    else:
        rec=json.loads(recpath.read_text())
        if sha(resource)!=rec['payload_sha256']: raise RuntimeError('sealed payload changed')
        if ns.phase in ['sign','dmg'] and not ns.identity: ap.error('sign and dmg require explicit --identity')
        if ns.phase == 'sign':
            steps=[]
            for target in [entry, app]:
                steps.append(run(['/usr/bin/codesign','--force','--options','runtime','--timestamp','--sign',ns.identity]+(['--keychain',ns.keychain] if ns.keychain else [])+[target]))
                steps.append(run(['/usr/bin/codesign','--verify','--strict','--verbose=2',target]))
            rec['identity']=ns.identity; rec['sign']=steps
            rec['launcher_after_sha256']=sha(entry)
            rec['signature_details']=run(['/usr/bin/codesign','-d','--verbose=4',app])
            details=rec['signature_details']['stderr']
            if 'TeamIdentifier=L2N7M5M544' not in details or 'Timestamp=' not in details or 'runtime' not in details:
                raise RuntimeError('expected company team, secure timestamp, and hardened runtime absent')
        elif ns.phase == 'dmg':
            run(['/usr/bin/codesign','--verify','--strict','--verbose=2',app])
            dmg=work/'unisacc-private-macos-universal.dmg'
            dmgroot=work/'dmgroot'
            try: dmgroot.stat()
            except FileNotFoundError: pass
            else: shutil.rmtree(dmgroot)
            dmgroot.mkdir(); shutil.copytree(app,dmgroot/'Unisacc.app')
            rec['dmg_signature_kind']='ad-hoc rehearsal' if ns.identity=='-' else 'Developer ID requested'
            rec['dmg']=[run(['/usr/bin/hdiutil','create','-volname','Unisacc','-srcfolder',dmgroot,'-ov','-format','UDZO',dmg]),
                        run(['/usr/bin/codesign','--force','--timestamp','--sign',ns.identity]+(['--keychain',ns.keychain] if ns.keychain else [])+[dmg]),
                        run(['/usr/bin/codesign','--verify','--strict','--verbose=2',dmg]),
                        run(['/usr/bin/hdiutil','verify',dmg])]
            rec['dmg_sha256']=sha(dmg)
            mount=work/'mount'; mount.mkdir(exist_ok=True)
            mounted=False
            try:
                rec['dmg'].append(run(['/usr/bin/hdiutil','attach','-readonly','-nobrowse','-mountpoint',mount,dmg]))
                mounted=True
                if sha(mount/'Unisacc.app/Contents/Resources/unisacc.com')!=rec['payload_sha256']:
                    raise RuntimeError('mounted DMG payload differs from source seal')
                rec['dmg'].append(run(['/usr/bin/codesign','--verify','--strict','--verbose=2',mount/'Unisacc.app']))
                rec['mounted_app_payload_sha256']=rec['payload_sha256']
            finally:
                if mounted: run(['/usr/bin/hdiutil','detach',mount])
        elif ns.phase == 'assess':
            rec['gatekeeper_app']=run(['/usr/sbin/spctl','-a','-t','exec','-vv',app],okay=False)
            dmg=work/'unisacc-private-macos-universal.dmg'
            rec['gatekeeper_dmg']=run(['/usr/sbin/spctl','-a','-t','open','--context','context:primary-signature','-vv',dmg],okay=False)
            rec['gatekeeper_accepted'] = all(rec[k]['returncode']==0 for k in ['gatekeeper_app','gatekeeper_dmg'])
            rec['trust'] = ('Gatekeeper app/DMG accepted; notarization/staple require separate receipts'
                            if rec['gatekeeper_accepted'] else 'Gatekeeper rejection recorded')
    if sha(resource)!=rec['payload_sha256']: raise RuntimeError('signing changed the inner APE')
    recpath.write_text(json.dumps(rec,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({'phase':ns.phase,'receipt':str(recpath),'payload_sha256':rec['payload_sha256'],
                      'release_eligible':False},ensure_ascii=False))
if __name__ == '__main__': main()

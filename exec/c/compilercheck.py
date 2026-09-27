#!/usr/bin/env python3
"""Driver contract checks; compiler semantics stay in freshly built models."""
import json,os,pathlib,resource,signal,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd, **kw):
    return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,**kw)
def ok(cmd, **kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
src=pathlib.Path('examples/hello.c').resolve();n=0
drivers=[p/'driver-cc',p/'driver-ua',p/'driver-asm']
part=sys.argv[4] if len(sys.argv)>4 else 'all'
assert part in ('all','core','resources'),part
if part in ('all','core'):
    # The migrated pipeline builds the driver itself, not just its input programs.
    netdriver=p/'driver-net'
    netdriver.write_bytes(ok([p/'run','--bundle',p/'models.pkg',target,'exec/c/compiler.c']))
    assert netdriver.read_bytes()==(p/'driver-ua').read_bytes(), 'network-built driver differs'
    netdriver.chmod(0o755)
    assert ok([netdriver,'--models',p/'compiler.pkg',src,'-b',target,'-O2'])==ok([ua,src,'-b',target,'-O2'])
    for exe in drivers:
        base=[exe,'--models',p/'compiler.pkg']
        # Version queries need no input or package, including outside the repo.
        for flag in ['--version','-version']:
            r=run([exe,flag],cwd=p)
            assert r.returncode==0 and not r.stderr and r.stdout==ok([ua,flag]),r
        for level in range(3):
            for mode in ['-E','-S','-b']:
                flags=['-b',target,mode]+([target] if mode=='-b' else [])+['-O'+str(level)]
                got=ok([*base,src,*flags]);want=ok([ua,src,*flags]);assert got==want,(exe,flags)
                n+=1
        # Build-system options have the reference's no-linker semantics.
        for compat in [['-lm','-L/nowhere','-xc'],
                       ['-l','m','-L','/nowhere','-x','c','-g','-std=c99']]:
            flags=['-b',target,'-S','-O2']
            want=ok([ua,*compat,src,*flags])
            assert ok([*base,*compat,src,*flags])==want
            assert ok([*base,src,*compat,*flags])==want
        for flag in ['-l','-L','-x']:
            r=run([*base,src,flag])
            assert r.returncode==1 and not r.stdout and b'missing compatibility argument' in r.stderr
        # stdin reaches the first network unchanged.
        flags=['-b',target,'-S','-O1']
        assert ok([*base,'-',*flags],input=src.read_bytes())==ok([ua,'-',*flags],input=src.read_bytes())
        # Parser/source failure cannot truncate an existing output file.
        bad=p/'bad.c';bad.write_text('int main( { this is invalid; }')
        out=p/'sentinel';out.write_bytes(b'preserve')
        r=run([*base,bad,'-o',out]);assert r.returncode!=0 and out.read_bytes()==b'preserve'
        # Source IO errors retain the product contract, also for a later unit.
        missing=p/'missing-source.c'
        for inputs in [[missing],[src,missing]]:
            r=run([*base,*inputs,'-o',out])
            assert r.returncode==1 and not r.stdout and out.read_bytes()==b'preserve'
            assert r.stderr==('unisacc: error: cannot open '+str(missing)+'\n').encode(),r.stderr
        for args in [['-E',src,src],['-o'],['-b'],['-D'],['-U'],['-include']]:
            r=run([*base,*args]);assert r.returncode==1 and not r.stdout
        r=run([*base,src,'-b','unknown/target','-o',out]);assert r.returncode!=0 and out.read_bytes()==b'preserve'
        r=run([*base,src,'-o',p]);assert r.returncode==1 and b'cannot open output' in r.stderr
        def limit():
            resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024));signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
        r=run([*base,src,'-b',target,'-o',p/'limited'],preexec_fn=limit)
        assert r.returncode==1 and b'short output write' in r.stderr,(r.returncode,r.stderr)
    # Undefined function checks must wait for definitions in later input units.
    ud=p/'undefined.c';late=p/'later-definition.c';sentinel=p/'undefined.out'
    for text in [
            'int main(void){return nosuchfn(3);}',
            'int missing(int); int main(void){return missing(1);}',
            'int main(void){first(); second(); first(); return 0;}',
            'int main(void){return outer(inner());}']:
        ud.write_text(text)
        ref=run([ua,ud,'-S','-o','-'])
        assert ref.returncode==1 and b'undefined function' in ref.stderr
        for exe in drivers:
            sentinel.write_bytes(b'keep')
            r=run([exe,'--models',p/'compiler.pkg',ud,'-b',target,'-S','-o',sentinel])
            assert r.returncode==1 and not r.stdout and r.stderr==ref.stderr,(text,exe,r.stderr,ref.stderr)
            assert sentinel.read_bytes()==b'keep'
    for text in ['int later(int); int main(void){return later(7);} int later(int x){return x;}',
                 'int main(void){return later(7);} int later(int x){return x;}']:
        ud.write_text(text)
        want=ok([ua,ud,'-S','-o','-'])
        for exe in drivers:
            assert ok([exe,'--models',p/'compiler.pkg',ud,'-b',target,'-S'])==want
    ud.write_text('int later(int); int main(void){return later(7);}')
    late.write_text('int later(int x){return x;}')
    want=ok([ua,ud,late,'-S','-o','-'])
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg',ud,late,'-b',target,'-S'])==want
    print('undefined functions: exact named errors, prototypes, duplicates and later definitions pass')
    # Dependency files follow successful real resource reads, never a textual
    # search for include directives. The ledger canonicalises repeated paths.
    dep_src=p/'deps.c';dep_head=p/'outer.h';dep_leaf=p/'leaf.h'
    dep_head.write_text('#include "leaf.h"\n');dep_leaf.write_text('#define DEP_VALUE 7\n')
    dep_src.write_text('#include "outer.h"\n#if 0\n#include "missing.h"\n#endif\nint main(void){return DEP_VALUE;}\n')
    dep_out=p/'deps.tape';dep_file=p/'deps.d'
    flags=['-b',target,'-MD','-S','-o',dep_out]
    ok([ua,dep_src,*flags]);dep_want=dep_file.read_bytes()
    assert str(dep_head).encode() in dep_want and str(dep_leaf).encode() in dep_want
    assert b'missing.h' not in dep_want
    for exe in drivers:
        base=[exe,'--models',p/'compiler.pkg','-b',target]
        for switch in ['-MD','-MMD']:
            dep_file.unlink(missing_ok=True)
            ok([*base,dep_src,switch,'-S','-o',dep_out])
            assert dep_file.read_bytes()==dep_want,(exe,dep_file.read_bytes(),dep_want)
        explicit=p/'explicit.d';explicit.unlink(missing_ok=True)
        ok([*base,'-MF',explicit,'-MD',dep_src,'-S','-o',dep_out])
        assert explicit.read_bytes()==dep_want
        saved=dep_src.read_text()
        dep_src.write_text('#include "outer.h"\n'+saved)
        ok([*base,dep_src,'-MD','-S','-o',dep_out])
        assert dep_file.read_bytes()==dep_want, 'cached repeated read changed prerequisite set'
        dep_src.write_text(saved)
        other=p/'deps-other.c';other.write_text('int auxiliary(void){return 0;}\n')
        ok([*base,dep_src,other,'-MD','-S','-o',dep_out])
        multi=dep_file.read_bytes()
        assert str(dep_src).encode() in multi and str(other).encode() in multi
        assert str(dep_head).encode() in multi and str(dep_leaf).encode() in multi
        carried=p/'carried-deps.c';carried.write_text('#include <stdio.h>\nint main(void){return 0;}\n')
        ok([*base,carried,'-MD','-S','-o',dep_out],cwd=p)
        assert b'stdio.h' not in dep_file.read_bytes(), 'carried resource is not a disk prerequisite'
        # A failed dependency write must fail and must not truncate compilation output.
        dep_out.write_bytes(b'keep')
        r=run([*base,'-MF',p,dep_src,'-S','-o',dep_out])
        assert r.returncode==1 and b'dependency output' in r.stderr and dep_out.read_bytes()==b'keep'
        r=run([*base,dep_src,'-MF']);assert r.returncode==1 and not r.stdout
    print('dependencies: nested actual reads, inactive include, MD/MMD/MF and IO failure pass')
    # Token dump is generated by the plain lexer model, not decoded by C.
    tok=p/'tokens.c'
    for text in ['int x = 1;\n',
                 '#define X 7\nint main(void){return X;}\n',
                 'char *s="a\\n"; /* comment */ unsigned long x=0xff;\n',
                 '__linux__ __ELF__ __unix__ __x86_64__ __UNISA__\n']:
        tok.write_text(text)
        want=ok([ua,'-dump-tokens',tok])
        assert want.endswith(b' tokens\n')
        for exe in drivers:
            got=ok([exe,'--models',p/'compiler.pkg','-dump-tokens',tok])
            assert got==want,(text,exe,got,want)
    print('token dump: plain lexer network, macros/literals/predefines match reference')
    print(f'compiler driver core: {n} mode/level matches and failure/dependency contracts',flush=True)
if part in ('all','resources'):
    # Raw CLI resources are interpreted by E2, in both the Python action oracle
    # and the actual threshold-network runtime. No C-side macro parser is used.
    sys.path.insert(0,str(pathlib.Path('exec/pp').resolve()))
    import sim
    delta=json.loads((p/'e2.json').read_text());loaded=sim.load(delta)
    probe=p/'cli.c';probe.write_text('#ifdef X\nX\n#else\n17\n#endif\n__UNISA__\n')
    def cli(opts,defs=(),undefs=(),includes=(),incdir='',nostd=False):
        flags=['-b',target,'-E',*opts]
        want=ok([ua,probe,*flags])
        for exe in drivers:
            assert ok([exe,'--models',p/'compiler.pkg',probe,*flags])==want,opts
        files=sim.Files()
        for key,values in [('defines',defs),('undefines',undefs),('includes',includes)]:
            files.cache[('\0cli/'+key).encode()]=b''.join(str(v).encode()+b'\0' for v in values)
        files.cache[b'\0cli/include-dir']=str(incdir).encode()
        files.cache[b'\0cli/nostdinc']=b'\1' if nostd else b''
        verdict,out,_=sim.run(delta,probe.read_bytes(),str(probe),files,maxsteps=50000000,loaded=loaded)
        assert verdict=='accept' and out==want,(opts,verdict,out,want)
    for opts,ds,us in [(['-DX=3'],['X=3'],[]),(['-D','X'],['X'],[]),
        (['-DX='],['X='],[]),(['-DX=1+2'],['X=1+2'],[]),(['-DX=-3'],['X=-3'],[]),
        (['-DX=1','-DX=2'],['X=1','X=2'],[]),(['-DX=1','-UX'],['X=1'],['X']),
        (['-U','X','-DX=1'],['X=1'],['X']),(['-D__UNISA__=4'],['__UNISA__=4'],[]),
        (['-U__UNISA__'],[],['__UNISA__'])]: cli(opts,ds,us)
    h1=p/'first.h';h2=p/'second.h'
    h1.write_text('#define X 7\n');h2.write_text('#undef X\n#define X 9\n')
    cli(['-include',h1,'-include',h2,'-DX=4'],['X=4'],includes=[h1,h2])
    # Quoted source-relative headers outrank -I; -I outranks carried headers.
    idir=p/'headers';idir.mkdir();(idir/'stdio.h').write_text('#define PICK 29\n')
    (p/'local.h').write_text('#define LOCAL 31\n');(idir/'local.h').write_text('#define LOCAL 99\n')
    probe.write_text('#include <stdio.h>\n#include "local.h"\nPICK LOCAL\n')
    cli(['-I',idir],incdir=idir);cli(['-I'+str(idir)],incdir=idir)
    cli(['-nostdinc','-I',idir],incdir=idir,nostd=True)
    probe.write_text('#include <stdio.h>\nint main(void){return 0;}\n')
    for exe in drivers:
        r=run([exe,'--models',p/'compiler.pkg','-nostdinc',probe,'-b',target,'-S'])
        assert r.returncode!=0 and not r.stdout and b'no such file for #include' in r.stderr,(exe,r.returncode,r.stdout,r.stderr)
    probe.write_text('int main(void){ printf("hi %d\\n",42); return 0; }\n')
    # The preprocessor must leave an undeclared printf untouched, without autoinc.
    cli(['-nostdinc'],nostd=True)
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg','-nostdinc','-run',probe])==b'hi 42\n'
    # Signed formatting is independently checked at the emitted-code boundary.
    probe.write_text('int main(void){printf("%d/%d/%d/%d/%d/%d\\n",0,1,-1,10,9223372036854775807L,-9223372036854775807L-1);return 0;}\n')
    expected=b'0/1/-1/10/9223372036854775807/-9223372036854775808\n'
    assert ok([ua,'-nostdinc','-run',probe])==expected
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg','-nostdinc','-run',probe])==expected
        flags=['-nostdinc','-b',target,'-S']
        assert ok([exe,'--models',p/'compiler.pkg',probe,*flags])==ok([ua,probe,*flags])
    print('undeclared printf decimal fallback: tape equality and integer boundary execution pass')
    fallback=pathlib.Path('exec/parse2/probes/printf_fallback.c').resolve()
    expected=(b'-1 2 4294967295 1f 1F 37 1f ok ! %\n'
              b'inner a\nouter 0 b\nescaped z / Q / 7\n')
    assert ok([ua,'-nostdinc','-run',fallback])==expected
    flags=['-nostdinc','-b',target,'-S']
    want=ok([ua,fallback,*flags])
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg',fallback,*flags])==want
        assert ok([exe,'--models',p/'compiler.pkg','-nostdinc','-run',fallback])==expected
    print('printf fallback: nine conversions, nested slots/pools, escapes/adjacent formats pass')


    probe.write_text('int main(void) { return VALUE; }\n')
    flags=['-DVALUE=7','-b',target,'-O2']
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg',probe,*flags])==ok([ua,probe,*flags])
    # Assembly driver consumes the same explicit package from an isolated cwd;
    # this is the real compiler CLI, not a standalone core_run harness.
    asmdir=p/'asm-isolated';asmdir.mkdir()
    asm=asmdir/'compiler';asm.write_bytes((p/'driver-asm').read_bytes());asm.chmod(0o755)
    (asmdir/'models.pkg').write_bytes((p/'compiler.pkg').read_bytes())
    (asmdir/'hello.c').write_bytes(src.read_bytes())
    base=[asm,'--models','models.pkg']
    for mode in ['-E','-S']:
        flags=['-b',target,mode,'-O2']
        assert ok([*base,'hello.c',*flags],cwd=asmdir)==ok([ua,src,*flags])
    ok([*base,'hello.c','-O2'],cwd=asmdir)
    assert (asmdir/'a.out').read_bytes()==ok([ua,src,'-b',target,'-O2'])
    assert ok([asmdir/'a.out'])==b'hello from C99\n'
    before=set(asmdir.iterdir())
    assert ok([*base,'-run','hello.c'],cwd=asmdir)==b'hello from C99\n'
    assert set(asmdir.iterdir())==before
    print('assembly compiler driver: isolated package, native output and memory run pass')
    # Decimal rounding and the entire carried math header must survive the real
    # network route, not just the converter's unit harness. Independent cc runs
    # check the probes' zero-exit expectations as well as reference tape spelling.
    probes=[pathlib.Path('exec/parse2/probes/'+name+'.c') for name in
            ['decimal_literals', 'math_header', 'brace_string', 'string_rows', 'void_cast', 'array_shapes', 'wide_strings']]
    probes += [pathlib.Path('exec/c/probes/'+name+'.c') for name in
               ['compound_integer', 'compound_pointer', 'address_lvalue', 'compound_literals',
                'bitfield_edges', 'bitfield_enum_scope', 'bitfield_nested', 'bitfield_result']]
    for source in probes:
        source=source.resolve(); name=source.stem
        host=p/(name+'-cc'); native=p/(name+'-model')
        ok(['cc','-w',source,'-lm','-o',host]); expected=ok([host])
        for level in ['-O0','-O2']:
            assert ok([*base,'-run',source,level],cwd=asmdir)==expected
        ok([*base,source,'-O2','-o',native],cwd=asmdir); assert ok([native])==expected
    print('decimal/math/string/void/compound probes: host cc and ASM network driver, memory/native pass')
    # Public-shaped commands operate with only the container and source in cwd.
    isolated=p/'isolated';isolated.mkdir()
    com=isolated/'compiler.com';com.write_bytes((p/'driver.com').read_bytes())
    (isolated/'hello.c').write_bytes(src.read_bytes())
    base=['sh',com]
    for mode in ['-E','-S']:
        flags=['-b',target,mode,'-O2']
        assert ok([*base,'hello.c',*flags],cwd=isolated)==ok([ua,src,*flags])
    ok([*base,'hello.c','-O2'],cwd=isolated)
    assert (isolated/'a.out').read_bytes()==ok([ua,src,'-b',target,'-O2'])
    assert ok([isolated/'a.out'])==b'hello from C99\n'
    before=set(isolated.iterdir())
    assert ok([*base,'-run','hello.c'],cwd=isolated)==b'hello from C99\n'
    assert set(isolated.iterdir())==before, 'memory run created a file'
    print('compiler driver resources: macros, headers, printf, math and isolated container pass',flush=True)

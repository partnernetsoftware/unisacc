/* The command line, tinycc-shaped:

     unisacc -run FILE.c [args...]     compile and run, nothing on disk
     unisacc FILE.c -b os/arch         write the executable
     unisacc FILE.c -c                 write the tape
     unisacc FILE.c -t os/arch         the tape for a target
     unisacc FILE.c                    the token dump (the lexer's instrument)
     -I dir, -D name[=n]               as everywhere else

   Flags are taken from anywhere on the line, because this tool grew up
   writing them AFTER the file and its own suites still spell it that way;
   the first argument that is not a flag is the input, and for `-run` what
   follows the input belongs to the PROGRAM. */
int tb_read(char *path, int target, int force_origin);
int tb_target_id(char *target);
int tb_encode(int textlen, int origin);
extern char tb_file[MAXOUT];
int main(void) {
    int fd; int i; int p; int L; int k; int fi; int runit; int dump; int verb; int dumptok; int werror; int force_origin; int emitbin;
    char *a; char *t; long e; int n; int j;
    char *outpath;
    int (*entry)(long, long);
    int objwant; int bgiven; int funit; int asmwant;
    asmwant = 0;
    t = "lnx/x86_64"; fi = 0; runit = 0; dump = 0; verb = 0; outpath = 0; dumptok = 0; werror = 0; force_origin = 0; emitbin = 0;
    ninput = 0; objwant = 0; bgiven = 0; funit = 0;
    ftrim_libc = 1;
    if (__argc() >= 2) { if (strsame(__argv(1), "ar")) return tl_ar(__argc()); }   /* `unisacc ar ...` (R17-3) */
    if (__argc() >= 2) { if (strsame(__argv(1), "as")) return at_main(__argc()); }  /* `unisacc as ...` (R18-1) */
    /* Default mode is RUN (owner, 2026-10-01; 0.0.17 R17-10): with no mode or
       output flag at all, `unisacc FILE.c [args]` compiles and runs in memory,
       exactly as `-run` does -- deliberately unlike cc's silent a.out.  A file
       is written only on request: -o, -b/-t, -S/-c, -E, -M*, -dump-tokens. */
    i = 1; j = 0;
    while (i < __argc()) {
        a = __argv(i);
        if (a[0] == 45 && a[1] == 45 && a[2] == 0) break;          /* `--` */
        if (a[0] == 45) {
            if (a[1] == 111 || a[1] == 98 || a[1] == 116 || a[1] == 83 || a[1] == 99 || a[1] == 69 || a[1] == 77) j = 1;
            if (strsame(a, "-run") || strsame(a, "-dump-tokens") || strsame(a, "--tapebin")
                || strsame(a, "--version") || strsame(a, "-version") || strsame(a, "--check-oracle")) j = 1;
        }
        i = i + 1;
    }
    if (j == 0) runit = 1;                 /* R11-3: library bodies on demand by default; -fno-trim-libc restores the full set */
    i = 1;
    while (i < __argc()) {
        a = __argv(i);
        if (a[0] == 45 && a[1] == 45 && a[1 + 1] == 0) {   /* `--`: argv starts */
            i = i + 1; break;
        }
        if (strsame(a, "--check-oracle")) {
            int b1; int b2;
            model_dims(); setup();
            b1 = oracle_pass(1); b2 = oracle_pass(0 - 1);
            printf("oracle  questions %d   cached answers that differ from the net: %d\n",
                   oracle_n, b1 + b2);
            return (b1 + b2) ? 1 : 0;
        }
        /* `-version` / `--version`: which build is this.  `-v` is taken --
           it prints how many times each stage was asked. */
        if (strsame(a, "-version") || strsame(a, "--version")) {
            printf("unisacc %s\n", UNISACC_VERSION);
            return 0;
        }
        if (a[0] == 45 && a[1]) {
            if (strsame(a, "--force-origin")) { force_origin = 1;
            } else { if (strsame(a, "--tapebin")) { emitbin = 1;
            } else { if (a[1] == 73) {                              /* -I */
                /* every -I, in order -- it used to keep only the last one */
                if (noptinc < 16) {
                    if (a[2]) optincs[noptinc] = a + 2; else { i = i + 1; optincs[noptinc] = __argv(i); }
                    noptinc = noptinc + 1;
                }
            } else { if (a[1] == 68) {                     /* -D */
                if (noptd < 16) {
                    if (a[2]) optd[noptd] = a + 2; else { i = i + 1; optd[noptd] = __argv(i); }
                    noptd = noptd + 1;
                }
            } else { if (a[1] == 114) { runit = 1;         /* -run */
            /* -S writes the tape: the tape IS this compiler's assembly-level
               IR, the same level gcc's -S stops at.  -c is kept as a synonym
               because the suites have always spelled it that way -- but
               there are no object files and no linker here, so `-c` never
               means what it means to gcc, and the usage line says so. */
            } else { if (a[1] == 99 || a[1] == 83) { dump = 1;   /* -c, -S */
                if (a[1] == 99) objwant = 1;             /* -c -b lnx/ARCH writes a relocatable ELF */
                if (a[1] == 83) asmwant = 1;             /* -S -b lnx/ARCH writes that object as GNU assembly */
            } else { if (a[1] == 118) { verb = 1;          /* -v */
            } else { if (a[1] == 111) {                    /* -o */
                if (a[2]) outpath = a + 2; else { i = i + 1; outpath = __argv(i); }
            } else { if (a[1] == 85) {                     /* -U */
                if (noptu < 16) {
                    if (a[2]) optu[noptu] = a + 2; else { i = i + 1; optu[noptu] = __argv(i); }
                    noptu = noptu + 1;
                }
            } else { if (strsame(a, "-dump-tokens")) { dumptok = 1;   /* the lexer's instrument */
            } else { if (strsame(a, "-include")) {         /* -include FILE */
                i = i + 1;
                if (nopti < 8) { opti[nopti] = __argv(i); nopti = nopti + 1; }
            } else { if (strsame(a, "-Werror")) { werror = 1; warnall = 1;
            } else { if (strsame(a, "-Wall") || strsame(a, "-Wextra")) { warnall = 1;
            } else { if (strpre(a, "-ferror-limit=")) { maxerr = 0; k = 14;
                while (a[k] >= 48 && a[k] <= 57) { maxerr = maxerr * 10 + (a[k] - 48); k = k + 1; }
            } else { if (strsame(a, "-ftrim-libc") || strsame(a, "-libneed")) { ftrim_libc = 1;
            } else { if (strsame(a, "-fno-trim-libc")) { ftrim_libc = 0;
            } else { if (strsame(a, "-nostdinc")) { nostdinc = 1;
            } else { if (strsame(a, "-MD") || strsame(a, "-MMD")) { wantdeps = 1; depfile = depfile ? depfile : "";
            } else { if (strsame(a, "-MF")) { i = i + 1; wantdeps = 1; depfile = __argv(i);
            } else { if (strsame(a, "-MT") || strsame(a, "-MQ")) { i = i + 1;
                if (__argv(i) == 0) return emsg("unisacc: error: missing dependency target", 0);
                if (ndeptargets < 16) { deptargets[ndeptargets] = __argv(i); depquoted[ndeptargets] = a[2] == 81; ndeptargets = ndeptargets + 1; }
            } else { if (strsame(a, "-funit")) { funit = 1;   /* separate compilation (docs/toolchain.md §7) */
            } else { if (strsame(a, "-MP")) { depphony = 1;
            } else { if (strsame(a, "-M") || strsame(a, "-MM")) {   /* like gcc: -E implied, only the .d line */
                wantdeps = 1; deponly = 1; pponly = 1; dump = 1; depfile = depfile ? depfile : "";
            /* -l and -L: the library is in the headers, so there is nothing
               to link and nothing to search.  -x c: the only language. */
            } else { if (a[1] == 108 || a[1] == 76) {
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 120) {                    /* -x LANG */
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 69) { pponly = 1; dump = 1;  /* -E */
            } else { if (a[1] == 98 || a[1] == 116) {      /* -b, -t */
                if (a[1] == 98) { dump = 2; bgiven = 1; } else dump = 1;
                i = i + 1; t = __argv(i);
            /* Flags a build system passes that mean nothing here: there is
               one dialect (C99), one optimisation level, and no separate
               debug info.  A compiler that REFUSES them cannot be dropped
               into an existing Makefile, which is most of what `CC=` is. */
            } else { if (a[1] == 87 || a[1] == 119 || a[1] == 103
                      || a[1] == 79 || a[1] == 102 || a[1] == 115
                      || a[1] == 112 || a[1] == 109) {    /* -W -w -g -O -f -std -pipe -m */
                if (a[1] == 79) {             /* -O, -O0..-O3, -Os: [H1] */
                    optlevel = 1;
                    if (a[2] >= 48 && a[2] <= 57) optlevel = a[2] - 48;
                }
            } else { if (j == 0 && fi) return emsg("unisacc: error: unknown option (arguments for the program go after --) ", a); return emsg("unisacc: error: unknown option ", a); } } } } } } } } } } } } } } } } } } } } } } } } } } } }
        } else {
            /* Several inputs make ONE program.  Under `-run` the line also
               carries the PROGRAM's arguments, so the inputs are the `.c`
               files at the front: the first argument that is not one ends
               the list and begins argv.  `--` ends it explicitly, for a
               program whose own first argument is a .c file. */
            if (fi == 0) fi = i;
            if (runit) {
                if (isdotc(a) == 0) break;
            }
            if (ninput < 64) { inputs[ninput] = a; ninput = ninput + 1; }
        }
        i = i + 1;
    }
    if (fi == 0) {
        model_dims(); setup();
        return emsg("usage: unisacc FILE.c [FILE.c...] [-- args...]      compile and RUN (the default)\n"
               "       unisacc [-E] [-I dir] [-D name[=n]] FILE.c [FILE.c...] [-S | -b os/arch | -dump-tokens | -M | -MM] -o out\n"
               "  -fno-trim-libc  keep every library body (default: bodies on demand, -ftrim-libc)\n"
               "  -S  write the tape, this compiler's assembly-level IR"
               " (-c is a synonym: there are no object files)", 0);
    }
    if (emitbin && dump == 0 && runit == 0) dump = 1;
    /* `-c` with an explicit `-b`: a relocatable object.  Bare `-c` stays the
       tape (86 suites spell -S that way) until Mach-O and COFF objects exist
       too, see docs/toolchain.md §4. */
    objwant = objwant && bgiven && dump == 2;
    /* `-S -b lnx/ARCH`: the object -c -b would write, as GNU assembly
       (src/asmtext.c).  Bare -S stays the tape. */
    asmwant = asmwant && bgiven && pponly == 0;
    if (asmwant) {
        if (!(t[0] == 108 && t[1] == 110 && t[2] == 120 && t[3] == 47)) return emsg("unisacc: error: assembly text (-S -b) is written for Linux targets in this version (Mach-O and COFF text: 0.0.19); the target was ", t);
        dump = 2; objwant = 1;
        if (outpath == 0) { outpath = deptarget(0, inputs[0]); outpath[blen(outpath) - 1] = 115; }
    }
    if (funit && objwant == 0 && (dump != 1 || pponly)) return emsg("unisacc: error: -funit needs -c -b os/arch (a unit object) or -S (its tape)", 0);
    unitmode = funit;
    objextern = objwant;                   /* R17-9 (a): a whole-program object may read cc's data symbols */
    if (objwant) {
        /* lnx -> ELF, osx -> Mach-O, win -> COFF (docs/toolchain.md) */
        if (outpath == 0) outpath = deptarget(0, inputs[0]);
    }
    if (depfile) { if (depfile[0] == 0) {
        /* gcc's names: -M/-MM go to -o or stdout; -MD/-MMD to -o with the
           suffix .d, or to the input's basename .d in the current directory */
        static char dname[520]; char *base; int k; int dot; int b;
        if (deponly) depfile = outpath ? outpath : "-";
        else {
            base = outpath ? outpath : inputs[0];
            k = 0; dot = 0 - 1; b = 0;
            while (base[k] && k < 512) { if (base[k] == 47) { dot = 0 - 1; if (outpath == 0) b = k + 1; } if (base[k] == 46) dot = k; k = k + 1; }
            if (dot < b) dot = k;
            k = b; while (k < dot) { dname[k - b] = base[k]; k = k + 1; }
            dname[dot - b] = 46; dname[dot - b + 1] = 100; dname[dot - b + 2] = 0;
            depfile = dname;
        }
    } }
    if (runit) {
        if (istapebin(__argv(fi))) {
            if (ninput != 1 || tb_read(__argv(fi), tb_target_id(HOST_TARGET), force_origin)) return 1;
        } else { if (istape(__argv(fi))) {
            if (ninput != 1 || fe_read(__argv(fi))) return 1;
        } else { if (isobject(__argv(fi))) {                         /* unit objects: link, then run */
            if (fe_link(inputs, ninput)) return 1;
            if (strsame(tl_first, HOST_TARGET) == 0) return emsg("unisacc: error: these objects were compiled for another target; running needs this machine's: ", tl_first);
        }
        else { if (fe_units(inputs, ninput, HOST_TARGET)) return 1; } } }
        if (werror && nwarn > 0) return 1;           /* -Werror: nothing runs */
        /* argv[0] is the program, which is its first source file; the rest
           of the line follows the inputs */
        n = __argc() - (fi + ninput) + 1;
        j = fi + ninput;                             /* the program's first argument... */
        if (j < __argc()) { if (strsame(__argv(j), "--")) { j = j + 1; n = n - 1; } }   /* ...not the `--` that ends the inputs */
        if (n > 255) n = 255;
        if (n < 1) n = 1;
        runargv[0] = __argv(fi);
        k = 1;
        while (k < n) { runargv[k] = __argv(j + k - 1); k = k + 1; }
        runargv[n] = 0;
        /* ...and after the NULL, our own environment, where a kernel would
           put envp: getenv walks __argv past argc [S-15 D2] */
        k = n + 1; j = __argc() + 1;
        while (k < 4095 && __argv(j) != 0) { runargv[k] = __argv(j); k = k + 1; j = j + 1; }
        runargv[k] = 0;
        /* bk_run writes argc/argv into the program's own cells, so this call
           assumes nobody's calling convention */
        e = bk_run(out, nout, n, (long)runargv);
        entry = (int (*)(long, long))e;
        return entry(0, 0);
    }
    /* No mode given: what `cc FILE.c` does -- an executable for this machine,
       named a.out unless -o says otherwise.  The token dump used to be the
       default here and printed the lexer's view instead. */
    if (dump == 0 && dumptok == 0) {
        dump = 2; t = DEFAULT_TARGET;
#ifdef _WIN32
        if (outpath == 0) outpath = "a.exe";      /* what Windows can run */
#else
        if (outpath == 0) outpath = "a.out";
#endif
    }
    if (dump) {
        int ofd; int r; int binlen;
        /* the destination is opened FIRST: `-E -o x.i` writes from inside
           the front end, before there is a tape to write */
        /* ...but only for -E.  Anything else is opened after the front end
           has succeeded: a failed compile must not leave an empty a.out
           behind, newer than its sources, for make to trust. */
        ofd = 1;
        if (outpath && pponly && deponly == 0) {
            ofd = wopen(outpath);
            if (ofd < 0) return emsg("unisacc: error: cannot write ", outpath);
        }
        bkfd = ofd;
        if (istapebin(__argv(fi))) { if (ninput != 1 || tb_read(__argv(fi), tb_target_id(t), force_origin)) return 1; }
        else { if (istape(__argv(fi))) { if (fe_read(__argv(fi))) return 1; }
        else { if (isobject(__argv(fi))) {
            if (fe_link(inputs, ninput)) return 1;
            /* the program is for the objects' target: -b may only repeat it */
            if (bgiven == 0) t = tl_first;
            else { if (strsame(t, tl_first) == 0) return emsg("unisacc: error: -b differs from the target these objects were compiled for: ", tl_first); }
        } else {
            r = fe_units(inputs, ninput, t);
            if (r == 2) {                                            /* -E is done */
                if (deponly) return writedeps(deptarget(outpath, inputs[0]), inputs, ninput);
                if (ofd != 1) __close(ofd); return 0; }
            if (r) return 1;
            if (werror && nwarn > 0) return 1;       /* -Werror: nothing written */
        } } }
        binlen = 0;
        if (emitbin) {
            binlen = tb_encode(nout, istape(__argv(fi)) ? 0 : tb_target_id(t));
            if (binlen <= 0) return emsg("unisacc: error: cannot encode tapebin", 0);
        }
        if (outpath && pponly == 0) {
            ofd = wopen(outpath);
            if (ofd < 0) return emsg("unisacc: error: cannot write ", outpath);
            bkfd = ofd;
        }
        if (depfile) { if (writedeps(deptarget(outpath, inputs[0]), inputs, ninput)) return 1; }
        if (emitbin) { __write(ofd, tb_file, binlen); if (ofd != 1) __close(ofd); return 0; }
        if (dump == 2 && asmwant) {
            at_cap = at_src; at_capn = 0;
            bk_object(out, nout, t);
            at_cap = 0; at_fd = ofd;
            if (at_dis(at_src, at_capn)) { if (ofd != 1) __close(ofd); return emsg("unisacc: error: -S: ", at_err); }
            if (ofd != 1) __close(ofd);
            return 0;
        }
        if (dump == 2) { if (objwant) bk_object(out, nout, t); else bk_build(out, nout, t); if (ofd != 1) __close(ofd); return 0; }
        __write(ofd, out, nout);
        if (ofd != 1) __close(ofd);
        /* `-c -v`: how many times each stage was asked, so a test can check
           that no table-shaped stage is decided in code */
        if (verb) {
            nout = 0;
            es("asked pp "); en(nask[S_PP]); es(" lex "); en(nask[S_LEX]);
            es(" parse "); en(nask[S_PARSE]); es(" type "); en(nask[S_TYPE]);
            es(" scope "); en(nask[S_SCOPE]); es(" irsel "); en(nask[S_IRSEL]);
            es(" tyinfo "); en(nask[S_TYINFO]); es(" pfconv "); en(nask[S_PFCONV]);
            ec(10);
            __write(2, out, nout);
        }
        return 0;
    }
    /* the token dump: the lexer's instrument.  No header selection here --
       the Python side it is compared with does that in its driver. */
    nibuf = 0; toinit = 0; hasinit = 0;
    fnresume = 0 - 1;
    model_dims();
    setup();
    srcpath = __argv(fi);
    fd = ropen(srcpath);
    if (fd < 0) return enoinput(srcpath);
    nsrc = __read(fd, src, MAXSRC);
    __close(fd);
    if (nsrc >= MAXSRC - 1) return emsg("unisacc: error: source too large", 0);
    tgt = "lnx/x86_64";
    splice();
    decomment();
    preprocess();
    expandsrc();
    if (lex() < 0) return 1;
    i = 0;
    while (i < ntok) {
        p = voff(TOKV, tkind[i]);
        L = vlen(TOKV, tkind[i]);
        __write(1, TOKV + p, L);
        if (tkind[i] == 2) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        if (tkind[i] == 3) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        if (tkind[i] == 4) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        __write(1, "\n", 1);
        i = i + 1;
    }
    printf("%d tokens\n", ntok);
    return 0;
}
#endif

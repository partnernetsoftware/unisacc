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
int main(void) {
    int fd; int i; int p; int L; int k; int fi; int runit; int dump; int verb; int dumptok; int werror;
    char *a; char *t; long e; int n; int j;
    char *outpath;
    int (*entry)(long, long);
    t = "lnx/x86_64"; fi = 0; runit = 0; dump = 0; verb = 0; outpath = 0; dumptok = 0; werror = 0;
    ninput = 0;
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
            if (a[1] == 73) {                              /* -I */
                if (a[2]) optinc = a + 2; else { i = i + 1; optinc = __argv(i); }
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
            } else { if (strsame(a, "-nostdinc")) { nostdinc = 1;
            } else { if (strsame(a, "-MD") || strsame(a, "-MMD")) { wantdeps = 1; depfile = depfile ? depfile : "";
            } else { if (strsame(a, "-MF")) { i = i + 1; wantdeps = 1; depfile = __argv(i);
            } else { if (strsame(a, "-MT") || strsame(a, "-MQ")) { i = i + 1;
            } else { if (strsame(a, "-MP") || strsame(a, "-M") || strsame(a, "-MM")) {
            /* -l and -L: the library is in the headers, so there is nothing
               to link and nothing to search.  -x c: the only language. */
            } else { if (a[1] == 108 || a[1] == 76) {
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 120) {                    /* -x LANG */
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 69) { pponly = 1; dump = 1;  /* -E */
            } else { if (a[1] == 98 || a[1] == 116) {      /* -b, -t */
                if (a[1] == 98) dump = 2; else dump = 1;
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
            } else { return emsg("unisacc: error: unknown option ", a); } } } } } } } } } } } } } } } } } } } } } }
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
        return emsg("usage: unisacc [-run] [-E] [-I dir] [-D name[=n]]"
               " FILE.c [FILE.c...] [-S | -b os/arch | -dump-tokens] [-o out]"
               " [-- args...]\n"
               "  -S  write the tape, this compiler's assembly-level IR"
               " (-c is a synonym: there are no object files)", 0);
    }
    if (depfile) { if (depfile[0] == 0) {
        static char dname[520]; char *base; int k; int dot;
        base = outpath ? outpath : inputs[0];
        k = 0; dot = 0 - 1;
        while (base[k] && k < 512) { dname[k] = base[k]; if (base[k] == 46) dot = k; if (base[k] == 47) dot = 0 - 1; k = k + 1; }
        if (dot < 0) dot = k;
        dname[dot] = 46; dname[dot + 1] = 100; dname[dot + 2] = 0;
        depfile = dname;
    } }
    if (runit) {
        if (fe_units(inputs, ninput, HOST_TARGET)) return 1;
        if (werror && nwarn > 0) return 1;           /* -Werror: nothing runs */
        /* argv[0] is the program, which is its first source file; the rest
           of the line follows the inputs */
        n = __argc() - (fi + ninput) + 1;
        if (n > 255) n = 255;
        if (n < 1) n = 1;
        runargv[0] = __argv(fi);
        k = 1;
        while (k < n) { runargv[k] = __argv(fi + ninput + k - 1); k = k + 1; }
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
        int ofd; int r;
        /* the destination is opened FIRST: `-E -o x.i` writes from inside
           the front end, before there is a tape to write */
        /* ...but only for -E.  Anything else is opened after the front end
           has succeeded: a failed compile must not leave an empty a.out
           behind, newer than its sources, for make to trust. */
        ofd = 1;
        if (outpath && pponly) {
            ofd = wopen(outpath);
            if (ofd < 0) return emsg("unisacc: error: cannot write ", outpath);
        }
        bkfd = ofd;
        if (istape(__argv(fi))) { if (fe_read(__argv(fi))) return 1; }
        else {
            r = fe_units(inputs, ninput, t);
            if (r == 2) { if (ofd != 1) __close(ofd); return 0; }  /* -E is done */
            if (r) return 1;
            if (werror && nwarn > 0) return 1;       /* -Werror: nothing written */
        }
        if (outpath && pponly == 0) {
            ofd = wopen(outpath);
            if (ofd < 0) return emsg("unisacc: error: cannot write ", outpath);
            bkfd = ofd;
        }
        if (depfile) { if (writedeps(outpath ? outpath : "a.out", inputs, ninput)) return 1; }
        if (dump == 2) { bk_build(out, nout, t); if (ofd != 1) __close(ofd); return 0; }
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

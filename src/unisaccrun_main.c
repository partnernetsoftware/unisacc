/* unisaccrun -- compile a C file and run it, without writing an executable.
 *
 * `unisaccrun FILE.c [args...]`.  The front end makes a tape, the back end
 * assembles that tape AT THE ADDRESSES it will run at in memory, and this
 * file jumps into it.  No temporary file, and so no code signature: macOS on
 * arm64 refuses to execute an unsigned file, but it will run a mapping this
 * process made and then marked executable.
 *
 * It is not a JIT: nothing is profiled, nothing is recompiled.  The whole
 * program is compiled once, ahead of running it -- only the output lands in
 * memory instead of on disk.  The compiled code keeps the tape's own calling
 * convention and exits through a syscall, so it never returns here.
 */
#ifdef __linux__
#ifdef __aarch64__
#define HOST_TARGET "lnx/arm64"
#else
#define HOST_TARGET "lnx/x86_64"
#endif
#else
#ifdef __aarch64__
#define HOST_TARGET "osx/arm64"
#else
#define HOST_TARGET "osx/x86_64"
#endif
#endif

char *runargv[256];

int main(void) {
    long e; int n; int k;
    int (*entry)(long, long);
    if (__argc() < 2) { printf("usage: unisaccrun FILE.c [args...]\n"); return 1; }
    if (fe_tape(__argv(1), HOST_TARGET)) return 1;
    /* the program's own argv: argv[0] is its source, then our own tail */
    n = __argc() - 1;
    if (n > 255) n = 255;
    k = 0;
    while (k < n) { runargv[k] = __argv(k + 1); k = k + 1; }
    runargv[n] = 0;
    /* bk_run puts argc/argv into the program's cells, so the entry takes
       nothing: this call must not depend on whose calling convention the
       compiler that built THIS file uses */
    e = bk_run(out, nout, n, (long)runargv);
    entry = (int (*)(long, long))e;
    return entry(0, 0);
}

/* check.c — unisacc-only suite. Replaces check.sh.
 *   unisacc check.c suite.c probes/u_run.c
 * No gcc/cc/clang. A real pty for term size is probes/term_size.py,
 * started from here. Exit 0 only when every check is ok.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);
int suite_slice_count(void);
int suite_slice_fill(int i, const char **argv, int cap, const char **name);
int suite_run_selftest(void);

static int pass_n, fail_n;
static const char *U;
static char work[64];

static void ok(const char *m) { printf("  ok    %s\n", m); pass_n++; }
static void no(const char *m) { printf("  FAIL  %s\n", m); fail_n++; }

static void first_line(const char *s, char *d, int n) {
    int i = 0;
    while (s[i] && s[i] != '\n' && i + 1 < n) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static int run_args(char **args, int n, char *out, int cap, int *rc) {
    return u_spawn(U, args, n, NULL, out, cap, rc);
}

static void selftest(const char *label, char **args, int n) {
    char out[2048], line[180], msg[240];
    int rc;
    run_args(args, n, out, (int)sizeof out, &rc);
    first_line(out, line, (int)sizeof line);
    snprintf(msg, sizeof msg, "%s (unisacc rc=%d: %s)", label, rc, line);
    if (rc == 0) ok(msg); else no(msg);
}

static int count_in(const char *s, const char *needle) {
    int c = 0;
    const char *p = s;
    size_t n = strlen(needle);
    if (!n) return 0;
    while ((p = strstr(p, needle))) { c++; p += n; }
    return c;
}

static int read_file(const char *path, char *buf, int cap) {
    FILE *f = fopen(path, "r");
    int n;
    if (!f) { if (cap) buf[0] = 0; return -1; }
    n = (int)fread(buf, 1, (size_t)cap - 1, f);
    fclose(f);
    if (n < 0) n = 0;
    buf[n] = 0;
    return n;
}

static int pty_size(char *out, int cap) {
    char *av[4];
    int st, pfd[2];
    pid_t pid;
    int off = 0;
    if (pipe(pfd) != 0) return -1;
    pid = fork();
    if (pid == 0) {
        dup2(pfd[1], 1); dup2(pfd[1], 2);
        close(pfd[0]); close(pfd[1]);
        av[0] = (char *)"python3";
        av[1] = (char *)"probes/term_size.py";
        av[2] = (char *)U;
        av[3] = NULL;
        execvp("python3", av);
        _exit(127);
    }
    close(pfd[1]);
    while (off + 1 < cap) {
        int k = (int)read(pfd[0], out + off, (size_t)cap - 1 - off);
        if (k <= 0) break;
        off += k;
    }
    out[off] = 0;
    close(pfd[0]);
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

int main(void) {
    char out[8192], path[96], msg[300];
    int rc;
    FILE *f;
    U = getenv("UNISACC");
    if (!U || !U[0]) U = "/Users/wjc/repos/unisacc/unisacc.com";
    if (access(U, 1) != 0) { printf("no unisacc at %s\n", U); return 1; }
    snprintf(work, sizeof work, "/tmp/csih-check-%d", (int)getpid());
    if (mkdir(work, 0700) != 0) { printf("no temp\n"); return 2; }

    printf("csih — backend=unisacc (%s)\n\n", U);
    printf("each module alone (via its cli file):\n");
    {
        int i;
        if (suite_run_selftest() == 0) ok("suite match rules");
        else no("suite match rules");
        for (i = 0; i < suite_slice_count(); i++) {
            const char *av[48];
            const char *name = NULL;
            int n = suite_slice_fill(i, av, 48, &name);
            char label[64];
            snprintf(label, sizeof label, "%s selftest", name ? name : "?");
            selftest(label, (char **)av, n);
        }
    }
    printf("\nthe closed loop (session → field → gate → write back):\n");

    pty_size(out, (int)sizeof out);
    if (strstr(out, "uni=cols=100 rows=30 source=tty"))
        ok("term size reads the real tty (unisacc)");
    else { snprintf(msg, sizeof msg, "term size on a pty: %.180s", out); no(msg); }
    if (strstr(out, "uni-small=cols=80 rows=24 source=fallback"))
        ok("term size falls back instead of trusting a 2x5 tty (the 43690 guard)");
    else { snprintf(msg, sizeof msg, "term size small-tty guard: %.180s", out); no(msg); }

    snprintf(path, sizeof path, "%s/uni.jsonl", work);
    f = fopen(path, "w");
    if (f) {
        fputs("{\"role\":\"user\",\"text\":\"go\"}\n{\"role\":\"model\",\"p\":0.20}\n{\"role\":\"model\",\"p\":0.83}\n", f);
        fclose(f);
    }
    {
        char *a[] = {"gate.c","json.cx","session.c","loop.c","run", path};
        char line[160];
        run_args(a, 6, out, (int)sizeof out, &rc);
        first_line(out, line, (int)sizeof line);
        snprintf(msg, sizeof msg, "loop run stdout+rc (unisacc rc=%d: %s)", rc, line);
        if (rc == 0) ok(msg); else no(msg);
    }
    read_file(path, out, (int)sizeof out);
    if (strstr(out, "\"verdict\":\"CONTINUE\"") && strstr(out, "\"role\":\"gate\""))
        ok("the verdict is IN the transcript (loop closed, not just printed)");
    else no("no gate verdict record written back");

    snprintf(path, sizeof path, "%s/m1.c", work);
    f = fopen(path, "w"); if (f) { fputs("int main(void){return 0;}\n", f); fclose(f); }
    {
        char *a[] = {path, path};
        char m2[96];
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (strstr(out, "duplicate") || strstr(out, "multiple definitions") || strstr(out, "redefinition"))
            ok("two mains are rejected BY NAME (0.0.19 fixed the misleading error)");
        else if (strstr(out, "not covered"))
            ok("two mains still cannot combine (message is still the old vague one)");
        else no("unisacc accepted two mains — re-read loop.c's header");
        snprintf(m2, sizeof m2, "%s/m2.c", work);
        f = fopen(m2, "w"); if (f) { fputs("int other(void){return 0;}\n", f); fclose(f); }
        a[1] = m2;
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (rc == 0) ok("still true: one main + one library composes");
        else no("one-main + library no longer composes");
    }

    printf("\nTUI probes (probes/tui_run.c):\n");
    {
        char *a[] = {"probes/tui_run.c", "probes/u_run.c"};
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (rc == 0 && count_in(out, "unisacc-ok") == 5 && count_in(out, "expected-red") == 1)
            ok("5 unisacc TUI questions ok, 1 expected red (sigaction)");
        else {
            snprintf(msg, sizeof msg, "TUI probes unisacc rc=%d ok=%d expected-red=%d",
                     rc, count_in(out, "unisacc-ok"), count_in(out, "expected-red"));
            no(msg);
        }
    }

    printf("\nunisacc libc coverage (probes/libc_cover.c):\n");
    {
        char *a[] = {"probes/libc_cover.c", "probes/u_run.c"};
        char *p;
        int nerr = -1, nmiss = -1;
        run_args(a, 2, out, (int)sizeof out, &rc);
        p = strstr(out, "defined: ");
        if (p) nerr = atoi(p + 9);
        p = strstr(out, "Missing: ");
        if (p) nmiss = atoi(p + 9);
        if (nerr == 43) ok("errno now 43/43 — unisacc 0.0.19 closed the gap");
        else { snprintf(msg, sizeof msg, "errno coverage is %d/43", nerr); no(msg); }
        if (nmiss == 0) ok("functions: no gaps — unisacc now provides execl/execlp too");
        else { snprintf(msg, sizeof msg, "expected 0 missing functions, found %d", nmiss); no(msg); }
    }

    {
        char *a[] = {"probes/dup_symbol.c"};
        run_args(a, 1, out, (int)sizeof out, &rc);
        if (rc == 0 && !strstr(out, "STILL BROKEN"))
            ok("dup-globals are now REJECTED by unisacc (0 broken; was 2 before 0.0.19)");
        else { snprintf(msg, sizeof msg, "dup-symbol probe rc=%d", rc); no(msg); }
    }

    printf("\nTUI end to end (probes/tui_e2e.c):\n");
    {
        char *a[] = {"probes/tui_e2e.c"};
        run_args(a, 1, out, (int)sizeof out, &rc);
        if (rc == 0) ok("typed text reaches the transcript and a verdict is rendered");
        else no("tui e2e failed");
    }

    printf("\nnetwork (probes/net_cover.c + probes/http_roundtrip.c):\n");
    {
        char *a[] = {"probes/net_cover.c", "probes/u_run.c"};
        char net[8192];
        run_args(a, 2, net, (int)sizeof net, &rc);
        if (rc == 0 && (strstr(net, "ok    round trip") || strstr(net, "round trip ok")))
            ok("loopback TCP round trip (unisacc)");
        else no("net round trip differs");
        if (strstr(net, "trap still present"))
            no("the _unisa_ret trap is back — socket() needs <sys/wait.h> again");
        else {
            ok("socket() builds without <sys/wait.h> (the _unisa_ret trap is fixed)");
            read_file("net.c", out, (int)sizeof out);
            if (strstr(out, "include <sys/wait.h>"))
                no("net.c still includes <sys/wait.h> for the fixed trap — remove it");
            else
                ok("net.c no longer carries the dead <sys/wait.h> include");
        }
    }
    {
        char *a[] = {"probes/http_roundtrip.c", "probes/u_run.c"};
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (rc == 0) ok("real HTTP round trip against a local server (unisacc)");
        else no("http round trip failed");
    }
    {
        char *a[] = {"probes/llm_roundtrip.c", "probes/u_run.c"};
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (rc == 0) ok("real LLM round trip (request -> net -> gate -> transcript, unisacc)");
        else no("llm round trip failed");
    }
    {
        char *a[] = {"probes/agent_roundtrip.c", "probes/u_run.c"};
        run_args(a, 2, out, (int)sizeof out, &rc);
        if (rc == 0) ok("agent loop end-to-end (exec -> answer -> go:stop, unisacc)");
        else no("agent roundtrip failed");
    }

    printf("\n  %d ok, %d fail\n", pass_n, fail_n);
    return fail_n ? 1 : 0;
}

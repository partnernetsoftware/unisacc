/* suite_cli.c — the one main for the argv table.
 *   unisacc suite.c suite_cli.c selftest
 *   unisacc suite.c suite_cli.c list
 *   unisacc suite.c suite_cli.c rows <basename>
 * list and rows print one slice per line: name, then the argv, then selftest.
 * rows prints "none" when no slice names that basename.
 */
int printf(const char *fmt, ...);
int strcmp(const char *a, const char *b);
int suite_slice_count(void);
int suite_slice_fill(int i, const char **argv, int cap, const char **name);
int suite_run_selftest(void);

static void print_row(int i) {
    const char *av[24];
    const char *nm = 0;
    int n = suite_slice_fill(i, av, 24, &nm);
    int k;
    printf("%s", nm ? nm : "?");
    for (k = 0; k < n; k++) printf(" %s", av[k] ? av[k] : "");
    printf("\n");
}

int main(int argc, char **argv) {
    int i;
    if (argc >= 2 && !strcmp(argv[1], "selftest")) return suite_run_selftest();
    if (argc >= 2 && !strcmp(argv[1], "list")) {
        for (i = 0; i < suite_slice_count(); i++) print_row(i);
        return 0;
    }
    if (argc >= 3 && !strcmp(argv[1], "rows")) {
        int any = 0;
        for (i = 0; i < suite_slice_count(); i++) {
            const char *av[24];
            const char *nm = 0;
            int n = suite_slice_fill(i, av, 24, &nm);
            int k, hit = 0;
            const char *want = argv[2];
            if (!strcmp(want, "tui.c")) want = "cdsh.c";
            for (k = 0; k < n; k++)
                if (av[k] && !strcmp(av[k], want)) hit = 1;
            if (!hit) continue;
            any = 1;
            print_row(i);
        }
        if (!any) printf("none\n");
        return 0;
    }
    printf("usage: selftest | list | rows <basename>\n");
    return 2;
}

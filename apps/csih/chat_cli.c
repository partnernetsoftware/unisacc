/*
 * chat_cli.c — the `main` for chat.c, moved out so chat.c can be a library.
 *
 * Same rule as every other module here (gate/json/session/render/term): unisacc
 * runs a source as a program, a program has one `main`, so a module that keeps
 * its own cannot be linked with any other. chat.c is meant to be linked into
 * the TUI, so its `main` lives here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int    ok;
    int    cont;
    double p;
    char   verdict[192];
} chat_turn;

chat_turn chat_turn_run(const char *path, const char *user_text, double threshold);
int       chat_note_p(const char *path, double p);
int       chat_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return chat_run_selftest();
    if (!strcmp(cmd, "say")) {
        /* usage: chat_cli say <transcript> <threshold> <text> */
        chat_turn t;
        const char *path = argc > 2 ? argv[2] : "/tmp/cdsh-chat.jsonl";
        double thr = argc > 3 ? atof(argv[3]) : 0.5;
        const char *text = argc > 4 ? argv[4] : "";
        t = chat_turn_run(path, text, thr);
        printf("%s\n", t.verdict);
        return t.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "note")) {
        /* usage: chat_cli note <transcript> <p> */
        const char *path = argc > 2 ? argv[2] : "/tmp/cdsh-chat.jsonl";
        double p = argc > 3 ? atof(argv[3]) : 0.0;
        return chat_note_p(path, p) == 0 ? 0 : 1;
    }
    printf("usage: chat_cli selftest | say <transcript> <threshold> <text> | note <transcript> <p>\n");
    return 64;
}

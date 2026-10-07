/*
 * plugin_cli.c — catalog entry. `selftest` does not link any extra plugin.
 */
#include <string.h>

#include "plugin_api.h"

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return plugin_run_selftest();
    return 1;
}

#include <stdio.h>

int plugin_builtin_n(void);
const plugin_t *plugin_builtin(int i);

static int plugin_row_seen(int before) {
    int j;
    const plugin_t *a = plugin_builtin(before);
    if (!a) return 1;
    for (j = 0; j < before; j++) {
        const plugin_t *b = plugin_builtin(j);
        if (b && !strcmp(b->name, a->name)) return 1;
    }
    return 0;
}

int plugin_run_selftest(void) {
    int i, n = plugin_builtin_n();
    for (i = 0; i < n; i++) {
        const plugin_t *row = plugin_builtin(i);
        if (!row || !plugin_find(row->name) || !row->kind) {
            printf("missing %s\n", row ? row->name : "?");
            return 1;
        }
        {
            const char *nm = plugin_name(row->kind);
            if (!nm || strcmp(nm, row->name)) {
                printf("name mismatch %s\n", row->name);
                return 1;
            }
        }
        if (plugin_row_seen(i)) continue;
        printf("%s\n", row->name);
    }
    if (plugin_find("nosuch")) {
        printf("present nosuch\n");
        return 1;
    }
    printf("absent nosuch\n");
    {
        const char *mh = plugin_help("mind");
        if (!mh || !strstr(mh, "markdown-tree-dag")
            || !strstr(mh, "mermaid-flowchart-memory-palace")) {
            printf("mind help format\n");
            return 1;
        }
    }
    {
        const char *page = plugin_page("tree");
        if (strcmp(page, "思维树.md")) {
            printf("page tree\n");
            return 1;
        }
        printf("%s\n", page);
        page = plugin_page("palace");
        if (strcmp(page, "记忆宫殿.md")) {
            printf("page palace\n");
            return 1;
        }
        printf("%s\n", page);
    }
    if (plugin_page("nosuch")[0] != 0) {
        printf("page nosuch\n");
        return 1;
    }
    {
        static cdsh_plugin row;
        char cat[512];
        row.name = "probe";
        row.help = "selftest extra";
        row.run = 0;
        row.next = 0;
        plugin_register(&row);
        {
            const char *ph = plugin_help("probe");
            if (!ph || strcmp(ph, "selftest extra")) {
                printf("help extra\n");
                return 1;
            }
        }
        plugin_catalog(cat, (int)sizeof cat);
        if (!strstr(cat, "probe: selftest extra\n")) {
            printf("catalog extra\n");
            return 1;
        }
        printf("probe\n");
    }
    return 0;
}

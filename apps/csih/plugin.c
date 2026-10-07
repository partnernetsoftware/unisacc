/*
 * plugin.c — the capability catalog.
 *
 * Built-ins stay listed here. A new capability is its own .c that calls
 * plugin_register; it is not a new branch in the agent. unisacc does not run
 * constructors, so that .c exposes cdsh_plugin_boot and the call entry
 * invokes it. The selftest does not link that file.
 */
#include <stdio.h>
#include <string.h>

#include "plugin_api.h"

/* kind numbers match agent_kind. file's kind is ACT_READ; op selects
 * write and edit. answer is a built-in name too, so the prompt can
 * quote plugin_help instead of a second handwritten list. */
static const plugin_t plugins[] = {
    { "exec",   "run a shell command; why is one short sentence", 1 },
    { "file",   "read, write, or edit a file",                 2 },
    { "file",   "read, write, or edit a file",                 3 },
    { "file",   "read, write, or edit a file",                 4 },
    { "answer", "final reply to the user",                     5 },
    { "mind",   "思维树.md 用 markdown-tree-dag（├── 包含，══> 跨枝）；记忆宫殿.md 用 mermaid-flowchart-memory-palace（一段 mermaid flowchart）。add 只追加一行 - ，read 读回整页", 8 }
};
#define BUILTIN_COUNT ((int)(sizeof plugins / sizeof plugins[0]))

/* True when name already appears in plugins[0, before). */
static int builtin_seen(const char *name, int before) {
    int j;
    for (j = 0; j < before; j++)
        if (!strcmp(plugins[j].name, name)) return 1;
    return 0;
}

/* First plugins[] row with this name, or -1. Duplicate file rows keep kind 2. */
static int builtin_at(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < BUILTIN_COUNT; i++)
        if (!strcmp(plugins[i].name, name)) return i;
    return -1;
}

/* First plugins[] row with this kind, or -1. */
static int builtin_by_kind(int kind) {
    int i;
    for (i = 0; i < BUILTIN_COUNT; i++)
        if (plugins[i].kind == kind) return i;
    return -1;
}

static cdsh_plugin *extra;

void plugin_register(cdsh_plugin *p) {
    if (!p || !p->name) return;
    p->next = extra;
    extra = p;
}

int plugin_find(const char *name) {
    cdsh_plugin *p;
    if (!name) return 0;
    if (builtin_at(name) >= 0) return 1;
    for (p = extra; p; p = p->next)
        if (p->name && !strcmp(p->name, name)) return 1;
    return 0;
}

int plugin_kind(const char *name) {
    int i = builtin_at(name);
    return i >= 0 ? plugins[i].kind : 0;
}

const char *plugin_name(int kind) {
    int i = builtin_by_kind(kind);
    return i >= 0 ? plugins[i].name : 0;
}

const char *plugin_help(const char *name) {
    cdsh_plugin *p;
    int i;
    if (!name) return 0;
    i = builtin_at(name);
    if (i >= 0) return plugins[i].help;
    for (p = extra; p; p = p->next)
        if (p->name && !strcmp(p->name, name) && p->help) return p->help;
    return 0;
}

int plugin_catalog(char *out, int outlen) {
    cdsh_plugin *p, *earlier;
    int i, seen, n = 0;
    if (!out || outlen < 1) return 0;
    out[0] = 0;
    for (i = 0; i < BUILTIN_COUNT; i++) {
        if (builtin_seen(plugins[i].name, i)) continue;
        n += snprintf(out + n, (size_t)(outlen - n), "%s: %s\n",
                      plugins[i].name, plugins[i].help);
        if (n >= outlen) return outlen - 1;
    }
    for (p = extra; p; p = p->next) {
        if (!p->name || !p->help) continue;
        if (builtin_seen(p->name, BUILTIN_COUNT)) continue;
        seen = 0;
        for (earlier = extra; earlier != p; earlier = earlier->next)
            if (earlier->name && !strcmp(earlier->name, p->name)) seen = 1;
        if (seen) continue;
        n += snprintf(out + n, (size_t)(outlen - n), "%s: %s\n",
                      p->name, p->help);
        if (n >= outlen) return outlen - 1;
    }
    return n;
}

/* Map a mind target to its file. tree is 思维树.md, palace is 记忆宫殿.md,
 * anything else is the empty string. */
const char *plugin_page(const char *name) {
    if (!name) return "";
    if (!strcmp(name, "tree")) return "思维树.md";
    if (!strcmp(name, "palace")) return "记忆宫殿.md";
    return "";
}

int plugin_run(const char *name, const char *arg, char *out, int outlen) {
    cdsh_plugin *p;
    if (!plugin_find(name) || !out || outlen < 1) return 1;
    out[0] = 0;
    for (p = extra; p; p = p->next) {
        if (p->name && !strcmp(p->name, name) && p->run)
            return p->run(arg ? arg : "", out, outlen);
    }
    snprintf(out, (size_t)outlen, "%s: listed", name);
    return 0;
}

/* Walk the built-in table. The assertions live in plugin_cli.c. */
int plugin_builtin_n(void) { return BUILTIN_COUNT; }

const plugin_t *plugin_builtin(int i) {
    if (i < 0 || i >= BUILTIN_COUNT) return 0;
    return &plugins[i];
}

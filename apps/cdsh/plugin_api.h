#ifndef PLUGIN_API_H
#define PLUGIN_API_H

/* External name. kind is the internal ACT_* number from agent.c:
 * exec=1, file=2 (op then picks read/write/edit), answer=5, mind=8.
 * go:continue and go:stop are not rows. */
typedef struct {
    const char *name;
    const char *help;
    int kind;
} plugin_t;

/* A row linked only when its .c is on the unisacc.com command line. */
typedef struct cdsh_plugin {
    const char *name;
    const char *help;
    int (*run)(const char *arg, char *out, int outlen);
    struct cdsh_plugin *next;
} cdsh_plugin;

/* 1 if `name` is in the built-in table or a registered row, else 0. */
int plugin_find(const char *name);

/* Internal kind for a built-in name, or 0 if the name is not a built-in. */
int plugin_kind(const char *name);

/* Help line for a built-in or a registered row, or NULL. */
const char *plugin_help(const char *name);

/* Catalog name for an internal kind, or NULL. file's read/write/edit share "file". */
const char *plugin_name(int kind);

/* Unique "name: help" lines for built-ins, then registered rows. */
int plugin_catalog(char *out, int outlen);

/* Map a mind target to its page: tree->思维树.md, palace->记忆宫殿.md,
 * anything else -> "". */
const char *plugin_page(const char *name);

/* Run a registered row by name. Returns 0 on success. */
int plugin_run(const char *name, const char *arg, char *out, int outlen);

void plugin_register(cdsh_plugin *p);

/* Prints exec, file, mind, then "absent nosuch". Returns 0 only if all hold. */
int plugin_run_selftest(void);

#endif /* PLUGIN_API_H */

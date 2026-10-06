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

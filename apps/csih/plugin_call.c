/*
 * plugin_call.c — invoke a catalog name. The extra .c on the file list
 * supplies cdsh_plugin_boot. This is not an agent action kind.
 */
#include <stdio.h>
#include <string.h>

#include "plugin_api.h"

void cdsh_plugin_boot(void);

int main(int argc, char **argv) {
    char buf[128];
    const char *name;
    int rc;
    cdsh_plugin_boot();
    if (argc < 3 || strcmp(argv[1], "call") != 0) {
        printf("usage: call <name>\n");
        return 2;
    }
    name = argv[2];
    if (!plugin_find(name)) {
        printf("absent %s\n", name);
        return 1;
    }
    rc = plugin_run(name, "", buf, (int)sizeof buf);
    if (rc != 0) {
        printf("run failed\n");
        return 1;
    }
    printf("%s\n", buf);
    return 0;
}

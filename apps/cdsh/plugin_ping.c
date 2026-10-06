/*
 * plugin_ping.c — one capability, one translation unit.
 * Linked only when this file is on the unisacc.com command line.
 * It does not add an agent action kind; the catalog name is "ping".
 */
#include <stdio.h>

#include "plugin_api.h"

static int ping_run(const char *arg, char *out, int outlen) {
    (void)arg;
    snprintf(out, (size_t)outlen, "ping:ok");
    return 0;
}

static cdsh_plugin ping_row;

void cdsh_plugin_boot(void) {
    ping_row.name = "ping";
    ping_row.help = "answer a catalog ping";
    ping_row.run = ping_run;
    ping_row.next = 0;
    plugin_register(&ping_row);
}

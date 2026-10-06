/* State directory. agent.c and tui.c include this. The function is static
 * so the TUI image does not gain another .c. The link target is relative
 * ".cdsh": a target of "cdsh" would point at $HOME/cdsh. */
#ifndef CSIH_HOME_H
#define CSIH_HOME_H

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static void csih_home_bind(const char *home) {
    char neu[512], old[512];
    if (!home || !home[0]) return;
    snprintf(neu, sizeof neu, "%s/.csih", home);
    if (access(neu, 0) == 0) return;
    snprintf(old, sizeof old, "%s/.cdsh", home);
    if (access(old, 0) == 0 && symlink(".cdsh", neu) == 0) return;
    mkdir(neu, 0750);
}

#endif

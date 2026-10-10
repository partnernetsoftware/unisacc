/* module csih.home: provider, body from the former csih_home.h (static removed) */
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
void csih_home_bind(const char *home) {
    char neu[512], old[512];
    if (!home || !home[0]) return;
    snprintf(neu, sizeof neu, "%s/.csih", home);
    if (access(neu, 0) == 0) return;
    snprintf(old, sizeof old, "%s/.cdsh", home);
    if (access(old, 0) == 0 && symlink(".cdsh", neu) == 0) return;
    mkdir(neu, 0750);
}

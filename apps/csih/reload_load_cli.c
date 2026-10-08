#include "reload_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char out[131073];
static reload_io_token token;

int main(int argc, char **argv)
{
    unsigned long cap;
    char *end;
    size_t len;
    char why[256];
    int rc;
    size_t i;
    int cleared = 1;

    if (argc != 7) {
        fprintf(stderr, "usage: %s load PATH SESSION HANDOFF HASH CAP\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "load") != 0) {
        fprintf(stderr, "usage: %s load PATH SESSION HANDOFF HASH CAP\n", argv[0]);
        return 2;
    }
    end = NULL;
    cap = strtoul(argv[6], &end, 10);
    if (argv[6][0] == '\0' || end == argv[6] || *end != '\0' ||
        cap > 131073UL) {
        fprintf(stderr, "usage: %s load PATH SESSION HANDOFF HASH CAP\n", argv[0]);
        return 2;
    }

    for (i = 0; i < sizeof(out); i++) {
        out[i] = (char)0xAA;
    }
    memset(&token, 0xAA, sizeof(token));
    len = 7;
    memset(why, 0, sizeof(why));

    rc = reload_io_load(argv[2], argv[3], argv[4], argv[5],
                        out, (size_t)cap, &len, &token,
                        why, sizeof(why));

    if (rc == 0) {
        printf("rc=0 len=%zu why=ok session=%s handoff=%s hash=%s dev=%llu ino=%llu\n",
               len, token.session_id, token.handoff_id, token.candidate_hash,
               (unsigned long long)token.dev, (unsigned long long)token.ino);
        fwrite(out, 1, len, stdout);
        return 0;
    }

    if (len != 0) {
        cleared = 0;
    }
    for (i = 0; i < sizeof(token); i++) {
        if (((unsigned char *)&token)[i] != 0) {
            cleared = 0;
            break;
        }
    }
    if (cap > 0 && out[0] != '\0') {
        cleared = 0;
    }
    printf("rc=%d len=%zu why=%s cleared=%d\n", rc, len, why, cleared);
    return 1;
}

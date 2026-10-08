#include <stdio.h>
#include <stddef.h>
#include <string.h>

int journal_checkpoint(const char *path, long long *offset, char *why, size_t cap);

int main(int argc, char **argv)
{
    long long offset = 77;
    char why[256];
    int rc;

    why[0] = '\0';
    if (argc != 3 || strcmp(argv[1], "checkpoint") != 0) {
        fprintf(stderr, "usage: %s checkpoint <path>\n", argv[0]);
        return 2;
    }

    rc = journal_checkpoint(argv[2], &offset, why, sizeof(why));
    printf("rc=%d offset=%lld why=%s\n", rc, offset, why);
    return rc == 0 ? 0 : 1;
}

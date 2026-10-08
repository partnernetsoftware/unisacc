#include "reload_session.h"
#include <stdio.h>
#include <string.h>

static reload_session_state a, b;
static char input[131073], encoded[131073];

int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "roundtrip") != 0) return 2;
    FILE *f = fopen(argv[2], "rb");
    if (!f) return 2;
    size_t n = fread(input, 1, 131072, f);
    if (n == 131072) {
        int extra = fgetc(f);
        if (extra != EOF) { fclose(f); return 2; }
    }
    if (ferror(f)) { fclose(f); return 2; }
    if (fclose(f) != 0) return 2;
    input[n] = '\0';
    char why[256];
    size_t encoded_len = 0;
    if (reload_session_decode_v2(input, n, &a, why, sizeof why) != 0) {
        printf("FAIL decode_v2 input: %s\n", why);
        return 1;
    }
    if (reload_session_encode_v2(&a, encoded, sizeof encoded, &encoded_len, why, sizeof why) != 0) {
        printf("FAIL encode_v2: %s\n", why);
        return 1;
    }
    if (reload_session_decode_v2(encoded, encoded_len, &b, why, sizeof why) != 0) {
        printf("FAIL decode_v2 encoded: %s\n", why);
        return 1;
    }
    if (encoded_len >= sizeof encoded || memcmp(&a, &b, sizeof a) != 0) {
        printf("FAIL roundtrip mismatch\n");
        return 1;
    }
    puts("PASS");
    if (fwrite(encoded, 1, encoded_len, stdout) != encoded_len) return 2;
    return 0;
}

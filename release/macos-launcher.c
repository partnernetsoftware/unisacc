/* Native CLI adapter only. The sealed APE remains the compiler. */
#include <CommonCrypto/CommonDigest.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef PAYLOAD_SHA256
#error PAYLOAD_SHA256 must be supplied by the bundle builder
#endif
static int fail(const char *s) { fprintf(stderr, "Unisacc: %s\n", s); return 126; }
int main(int argc, char **argv) {
    char raw[PATH_MAX], exe[PATH_MAX], payload[PATH_MAX], manifest[PATH_MAX];
    uint32_t size = sizeof(raw);
    if (_NSGetExecutablePath(raw, &size) || !realpath(raw, exe)) return fail("cannot locate bundle entry");
    char *slash = strrchr(exe, '/');
    if (!slash) return fail("invalid bundle entry path");
    *slash = 0;
    slash = strrchr(exe, '/');
    if (!slash || strcmp(slash, "/MacOS")) return fail("entry is outside Contents/MacOS");
    *slash = 0;
    if (snprintf(payload, sizeof(payload), "%s/Resources/unisacc.com", exe) >= sizeof(payload) ||
        snprintf(manifest, sizeof(manifest), "%s/Resources/payload.sha256", exe) >= sizeof(manifest))
        return fail("bundle path is too long");
    FILE *f = fopen(manifest, "rb");
    char seal[67] = {0};
    if (!f) return fail("missing payload manifest");
    size_t n = fread(seal, 1, sizeof(seal), f); fclose(f);
    if (n != 65 || seal[64] != '\n' || memcmp(seal, PAYLOAD_SHA256, 64)) return fail("payload manifest differs from entry seal");
    f = fopen(payload, "rb");
    if (!f) return fail("missing sealed unisacc.com");
    CC_SHA256_CTX ctx; CC_SHA256_Init(&ctx);
    unsigned char buf[16384], digest[CC_SHA256_DIGEST_LENGTH];
    while ((n = fread(buf, 1, sizeof(buf), f))) CC_SHA256_Update(&ctx, buf, (CC_LONG)n);
    int bad = ferror(f); fclose(f);
    if (bad) return fail("cannot read sealed payload");
    CC_SHA256_Final(digest, &ctx);
    char hex[65];
    for (int i = 0; i < 32; i++) snprintf(hex + i * 2, 3, "%02x", digest[i]);
    if (strcmp(hex, PAYLOAD_SHA256)) return fail("payload SHA256 differs from entry seal");
    char **args = calloc((size_t)argc + 2, sizeof(char *));
    if (!args) return fail("out of memory");
    args[0] = "/bin/sh"; args[1] = payload;
    for (int i = 1; i < argc; i++) args[i + 1] = argv[i];
    execv(args[0], args);
    perror("Unisacc: exec /bin/sh");
    return 126;
}

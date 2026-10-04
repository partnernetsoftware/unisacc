/* examples/https/post.c -- HTTPS POST from a unisacc-compiled program through the system libcurl
 * (0.0.25 N1).  Nothing TLS is built into unisacc: the program loads libcurl with RTLD_GLOBAL, and
 * every prototyped-but-undefined curl function is then forwarded to it by name (src/fwdstub.c).
 * libcurl verifies the server certificate against the system trust store by default.
 *   unisacc examples/https/post.c                  (-run, macOS or Linux)
 *   unisacc examples/https/post.c -b lnx/arm64 -o post && ./post
 * curl_easy_setopt and curl_easy_getinfo are variadic, which the forwarder does not handle: on macOS
 * arm64 variadic arguments travel on the stack, so they go through uffi_call with a fixed-argument
 * count (libffi); on Linux variadic integer/pointer arguments use the same registers, so a plain
 * three-argument prototype is enough.  Without an API key the server answers 401: the TLS round
 * trip itself is what this shows. */
#include <stdio.h>
#include <string.h>
#include <unisacc_ffi.h>
void *curl_easy_init(void);
int curl_easy_perform(void *h);
void curl_easy_cleanup(void *h);
void *curl_slist_append(void *list, const char *s);
#define CURLOPT_URL 10002
#define CURLOPT_POSTFIELDS 10015
#define CURLOPT_HTTPHEADER 10023
#define CURLOPT_NOBODY 44
#define CURLINFO_RESPONSE_CODE 0x200002
#ifdef __APPLE__
static int setopt(void *h, int opt, void *val) {   /* curl_easy_setopt is variadic: call it as one */
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_setopt");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_POINTER;
    vals[0] = &h; vals[1] = &opt; vals[2] = &val;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
static int getinfo(void *h, int info, long *out) {   /* variadic as well */
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_getinfo");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_POINTER;
    vals[0] = &h; vals[1] = &info; vals[2] = &out;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
#else
int curl_easy_setopt(void *h, int opt, void *val);
int curl_easy_getinfo(void *h, int info, long *out);
#define setopt curl_easy_setopt
#define getinfo curl_easy_getinfo
#endif
int main(void) {
    void *h, *hdr = 0; long code = 0; int rc;
#ifdef __APPLE__
    if (!uffi_dlopen("/usr/lib/libcurl.4.dylib", 2 | 8)) { printf("no libcurl\n"); return 2; }
#else
    if (!uffi_dlopen("libcurl.so.4", 2 | 0x100)) { printf("no libcurl\n"); return 2; }
#endif
    h = curl_easy_init();
    hdr = curl_slist_append(hdr, "Content-Type: application/json");
    setopt(h, CURLOPT_URL, "https://api.deepseek.com/v1/chat/completions");
    setopt(h, CURLOPT_HTTPHEADER, hdr);
    setopt(h, CURLOPT_POSTFIELDS, "{}");
    rc = curl_easy_perform(h);
    getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    printf("\nperform rc %d http %ld\n", rc, code);
    curl_easy_cleanup(h);
    return rc != 0;
}

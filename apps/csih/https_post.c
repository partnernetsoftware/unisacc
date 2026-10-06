/* https_post.c — standalone probe. net.c calls libcurl itself as of unisacc 0.0.26.
 *   unisacc https_post.c post URL ctype bodyfile outfile
 * outfile gets one status line. The body is curl's stdout. DEEPSEEK_API_KEY is sent only
 * to api.deepseek.com. Exit 0 when curl finished, even on HTTP 401.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unisacc_ffi.h>
void *curl_easy_init(void);
int curl_easy_perform(void *h);
void curl_easy_cleanup(void *h);
void *curl_slist_append(void *list, const char *s);
void curl_slist_free_all(void *list);
#define CURLOPT_WRITEDATA  10001
#define CURLOPT_URL        10002
#define CURLOPT_POSTFIELDS 10015
#define CURLOPT_HTTPHEADER 10023
#define CURLINFO_RESPONSE_CODE 0x200002
/* libcurl fwrite's the body to the host libc stdout. unisacc fflush is a no-op,
 * and a unisacc function is not a host callback, so flush the host FILE*. */
static void flush_host_stdout(void) {
    void *ff, *slot, *file;
    int kinds[1];
    void *vals[1];
    int r = 0;
    ff = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fflush");
    slot = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "__stdoutp");
    if (!ff || !slot) return;
    file = *(void **)slot;
    kinds[0] = UFFI_POINTER;
    vals[0] = &file;
    uffi_call(ff, UFFI_INT, kinds, vals, 1, 1, &r);
}
#ifdef __APPLE__
static int curl_setopt(void *h, int opt, void *val) {
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_setopt");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_POINTER;
    vals[0] = &h; vals[1] = &opt; vals[2] = &val;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
static int curl_getinfo(void *h, int info, long *out) {
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
#define curl_setopt curl_easy_setopt
#define curl_getinfo curl_easy_getinfo
#endif
int main(int argc, char **argv) {
    const char *method, *url, *ctype, *bodypath, *outpath, *key;
    char body[65536], auth[600], chead[180];
    void *h, *hdr = 0;
    long code = 0;
    int rc;
    FILE *in, *out;
    size_t n = 0;
    if (argc < 6) { fprintf(stderr, "usage: post|get URL ctype bodyfile outfile\n"); return 64; }
    method = argv[1]; url = argv[2]; ctype = argv[3]; bodypath = argv[4]; outpath = argv[5];
#ifdef __APPLE__
    if (!uffi_dlopen("/usr/lib/libcurl.4.dylib", 2 | 8)) return 2;
#else
    if (!uffi_dlopen("libcurl.so.4", 2 | 0x100)) return 2;
#endif
    if (!strcmp(method, "post")) {
        in = fopen(bodypath, "r");
        if (!in) return 3;
        n = fread(body, 1, sizeof body - 1, in);
        fclose(in);
        body[n] = 0;
    }
    h = curl_easy_init();
    if (!h) return 2;
    hdr = curl_slist_append(0, "Accept: application/json");
    if (ctype && ctype[0]) {
        snprintf(chead, sizeof chead, "Content-Type: %s", ctype);
        hdr = curl_slist_append(hdr, chead);
    }
    key = getenv("DEEPSEEK_API_KEY");
    if (key && key[0] && strstr(url, "api.deepseek.com") && strlen(key) < 500) {
        snprintf(auth, sizeof auth, "Authorization: Bearer %s", key);
        hdr = curl_slist_append(hdr, auth);
    }
    curl_setopt(h, CURLOPT_URL, (void *)url);
    curl_setopt(h, CURLOPT_HTTPHEADER, hdr);
    if (!strcmp(method, "post")) curl_setopt(h, CURLOPT_POSTFIELDS, body);
    rc = curl_easy_perform(h);
    flush_host_stdout();
    curl_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(h);
    if (hdr) curl_slist_free_all(hdr);
    out = fopen(outpath, "w");
    if (!out) return 3;
    fprintf(out, "%ld\n", code);
    fclose(out);
    return rc == 0 ? 0 : 1;
}

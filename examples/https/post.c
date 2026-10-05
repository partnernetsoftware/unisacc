/* examples/https/post.c -- HTTPS POST from a unisacc-compiled program through the system libcurl.
 * Nothing TLS is built into unisacc.  `-lcurl` loads the host libcurl before main (0.0.28 N1'),
 * and every prototyped-but-undefined curl function -- the variadic curl_easy_setopt and
 * curl_easy_getinfo included -- is forwarded to it by name (src/fwdstub.c).  libcurl verifies the
 * server certificate against the system trust store by default.
 *   unisacc examples/https/post.c -lcurl                          (-run, macOS or Linux)
 *   unisacc examples/https/post.c -lcurl -b lnx/arm64 -o post && ./post
 * Without an API key the server answers 401: the TLS round trip itself is what this shows.  The
 * response body libcurl writes stays in the HOST C library's stdout buffer, which nothing flushes
 * when the program exits (docs/host-interop.md); the status line below is unisacc's own printf.
 * Windows does not forward to host libraries yet. */
#include <stdio.h>
void *curl_easy_init(void);
int curl_easy_setopt(void *h, int opt, ...);
int curl_easy_getinfo(void *h, int info, ...);
int curl_easy_perform(void *h);
void curl_easy_cleanup(void *h);
void *curl_slist_append(void *list, const char *s);
#define CURLOPT_URL 10002
#define CURLOPT_POSTFIELDS 10015
#define CURLOPT_HTTPHEADER 10023
#define CURLOPT_WRITEFUNCTION 20011
#define CURLINFO_RESPONSE_CODE 0x200002
int main(void) {
    void *h = curl_easy_init(), *hdr = 0; long code = 0; int rc;
    hdr = curl_slist_append(hdr, "Content-Type: application/json");
    curl_easy_setopt(h, CURLOPT_URL, "https://api.deepseek.com/v1/chat/completions");
    curl_easy_setopt(h, CURLOPT_HTTPHEADER, hdr);
    curl_easy_setopt(h, CURLOPT_POSTFIELDS, "{}");
    rc = curl_easy_perform(h);
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    printf("\nperform rc %d http %ld\n", rc, code);
    curl_easy_cleanup(h);
    return rc != 0;
}

/* host_l0.h -- declarations of the L0 and csih helper functions that the
   csih library units reach through `#pragma unisa use`, for check.sh's host cc
   fuzz build (host cc ignores the pragma). check.sh compiles their sources
   (zstr.cx, cx/text/text.cx, cx/mem/bytes.cx) beside the library units. */
#ifndef CSIH_HOST_L0_H
#define CSIH_HOST_L0_H
long tx_len(const char *s, long cap);
int tx_same(const char *a, long an, const char *b, long bn);
int tx_eq(const char *a, const char *b, long cap);
int tx_prefix(const char *s, long n, const char *p, long pn);
long tx_chr(const char *s, long n, int c);
long tx_find(const char *s, long n, const char *p, long pn);
int tx_copy(char *dst, long cap, const char *src, long n);
long zs_len(const char *s);
int zs_eq(const char *a, const char *b);
int zs_starts(const char *s, const char *p);
int zs_cmp(const char *a, const char *b);
char *zs_find(const char *s, const char *w);
char *zs_chr(const char *s, int c);
char *zs_rchr(const char *s, int c);
int zs_has_nul(const char *s, long cap);
int mm_copy(void *dst, long dcap, const void *src, long n);
int mm_move(void *dst, long dcap, const void *src, long n);
int mm_fill(void *dst, long dcap, int byte, long n);
int mm_same(const void *a, const void *b, long n);
int mm_cmp(const void *a, const void *b, long n);
void *mm_alloc(long n);
void *mm_zalloc(long n);
void *mm_grow(void *p, long n);
void mm_free(void *p);
char *mm_strdup(const char *s);
#endif

/* malloc/malloc.h -- 0.0.34 L2.  The macOS zone interface over this library's own malloc
 * (SQLite's Apple allocator).  Every zone is the one heap: malloc_default_zone and
 * malloc_create_zone return the same zone, whose members are this header's functions in
 * Apple's field order (size, malloc, calloc, valloc, free, realloc, destroy, zone_name). */
#ifndef _UNISA_MALLOC_MALLOC_H
#define _UNISA_MALLOC_MALLOC_H
#include <stddef.h>
#include <stdlib.h>
#ifdef __APPLE__
typedef struct _malloc_zone_t {
    void *reserved1; void *reserved2;
    size_t (*size)(struct _malloc_zone_t *, const void *);
    void *(*malloc)(struct _malloc_zone_t *, size_t);
    void *(*calloc)(struct _malloc_zone_t *, size_t, size_t);
    void *(*valloc)(struct _malloc_zone_t *, size_t);
    void (*free)(struct _malloc_zone_t *, void *);
    void *(*realloc)(struct _malloc_zone_t *, void *, size_t);
    void (*destroy)(struct _malloc_zone_t *);
    const char *zone_name;
} malloc_zone_t;
/* the usable bytes: malloc's header records the block (or mapping) size */
#if !__UNISA_FTRIM_LIBC || __UN_malloc_size
static size_t malloc_size(const void *__u_p) {
    if (__u_p == NULL) return 0;
    return (size_t)(*(long *)((char *)__u_p - 16) - 16);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_size
static size_t _u_mz_size(malloc_zone_t *__u_z, const void *__u_p) { return malloc_size(__u_p); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_malloc
static void *_u_mz_malloc(malloc_zone_t *__u_z, size_t __u_n) { return malloc(__u_n); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_calloc
static void *_u_mz_calloc(malloc_zone_t *__u_z, size_t __u_n, size_t __u_s) { return calloc(__u_n, __u_s); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_valloc
static void *_u_mz_valloc(malloc_zone_t *__u_z, size_t __u_n) { return NULL; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_free
static void _u_mz_free(malloc_zone_t *__u_z, void *__u_p) { free(__u_p); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_realloc
static void *_u_mz_realloc(malloc_zone_t *__u_z, void *__u_p, size_t __u_n) { return realloc(__u_p, __u_n); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_mz_destroy
static void _u_mz_destroy(malloc_zone_t *__u_z) { }
#endif
static malloc_zone_t _u_mz_zone;   /* filled on first use: the calls name each member, so -ftrim-libc keeps them */
#if !__UNISA_FTRIM_LIBC || __UN_malloc_default_zone
static malloc_zone_t *malloc_default_zone(void) {
    _u_mz_zone.size = _u_mz_size; _u_mz_zone.malloc = _u_mz_malloc; _u_mz_zone.calloc = _u_mz_calloc;
    _u_mz_zone.valloc = _u_mz_valloc; _u_mz_zone.free = _u_mz_free; _u_mz_zone.realloc = _u_mz_realloc;
    _u_mz_zone.destroy = _u_mz_destroy; _u_mz_zone.zone_name = "DefaultMallocZone";
    return &_u_mz_zone;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_create_zone
static malloc_zone_t *malloc_create_zone(size_t __u_start, unsigned __u_flags) { return malloc_default_zone(); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_set_zone_name
static void malloc_set_zone_name(malloc_zone_t *__u_z, const char *__u_name) { }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_zone_malloc
static void *malloc_zone_malloc(malloc_zone_t *__u_z, size_t __u_n) { return malloc(__u_n); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_zone_calloc
static void *malloc_zone_calloc(malloc_zone_t *__u_z, size_t __u_n, size_t __u_s) { return calloc(__u_n, __u_s); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_zone_free
static void malloc_zone_free(malloc_zone_t *__u_z, void *__u_p) { free(__u_p); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_malloc_zone_realloc
static void *malloc_zone_realloc(malloc_zone_t *__u_z, void *__u_p, size_t __u_n) { return realloc(__u_p, __u_n); }
#endif
#endif
#endif

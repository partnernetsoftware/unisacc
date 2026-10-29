/* LB-1: host adapter types. Framing lives in cx/compiler/hostbind.cx. */
#ifndef UNISA_HOST_BIND_H
#define UNISA_HOST_BIND_H
#include <stdint.h>
#include <stddef.h>
typedef struct { unsigned target, count, indirect, mask; } HostRequirements;
typedef struct { unsigned target, kind, available; uint64_t address[4]; } HostProvider;
typedef struct { unsigned target, kind, mask; uint64_t address[4]; } HostBindPlan;
int hb_target_id(const char *target);
int hb_requirements(const unsigned char *b, unsigned long long n, HostRequirements *r, char *error, unsigned long long cap);
int hb_provider_filter(const unsigned char *b, unsigned long long n, unsigned target, unsigned *mask, char *error, unsigned long long cap);
int hb_bind(const HostRequirements *r, const HostProvider *p, unsigned filter, HostBindPlan *out, char *error, unsigned long long cap);
static unsigned host_target_id(const char *target) { return (unsigned)hb_target_id(target); }
static int host_requirements(const unsigned char *b, size_t n, HostRequirements *r, char *error, size_t cap) {
    return hb_requirements(b, (unsigned long long)n, r, error, (unsigned long long)cap);
}
static int host_provider_filter(const unsigned char *b, size_t n, unsigned target, unsigned *mask, char *error, size_t cap) {
    return hb_provider_filter(b, (unsigned long long)n, target, mask, error, (unsigned long long)cap);
}
static int host_bind(const HostRequirements *r, const HostProvider *p, unsigned filter, HostBindPlan *out, char *error, size_t cap) {
    return hb_bind(r, p, filter, out, error, (unsigned long long)cap);
}
#endif

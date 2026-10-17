/* Internal host/word types. Target C type rules remain in the existing tables. */
#ifndef UNISA_INTERNAL_TYPES_H
#define UNISA_INTERNAL_TYPES_H
#include <stdint.h>
#ifdef __UNISA__
typedef unsigned long long us_addr;
typedef unsigned long long us_size;
typedef long long us_offset;
typedef unsigned long long us_u64;
typedef long long us_i64;
#else
#include <stddef.h>
typedef uintptr_t us_addr;
typedef size_t us_size;
typedef int64_t us_offset;
typedef uint64_t us_u64;
typedef int64_t us_i64;
#endif
typedef char us_addr_matches_pointer[(sizeof(us_addr) == sizeof(void *)) ? 1 : -1];
typedef char us_size_matches_pointer[(sizeof(us_size) == sizeof(void *)) ? 1 : -1];
typedef char us_machine64_is_eight[(sizeof(us_u64) == 8 && sizeof(us_i64) == 8) ? 1 : -1];
typedef char us_offset_is_eight[(sizeof(us_offset) == 8) ? 1 : -1];
#endif

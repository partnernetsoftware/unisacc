/* sys/_word.h -- the carried library's machine word: a pointer, a handle, a byte count or a
 * host-channel slot. It is `long` on the LP64 targets (lnx, osx) and `long long` on LLP64
 * Windows, so it is 8 bytes everywhere and the same C type as before off Windows.
 * __hostcall reads its argument vector as 8-byte slots (src/back_encode.c BK_HOST_*), and the
 * E3 builtins return 8-byte values (exec/parse2 callcontrol sysemit), so bridge arrays,
 * builtin results and pointer casts use __u_word, never plain `long`.
 * These are macros, not typedefs: off Windows they expand to exactly the tokens the headers
 * had before (`long`, `unsigned long`), so the LP64 preprocessed output stays byte-identical. */
#ifndef _UNISA_SYS_WORD_H
#define _UNISA_SYS_WORD_H
#if defined(_WIN64)
#define __u_word long long
#define __u_uword unsigned long long
_Static_assert(sizeof(__u_word) == 8 && sizeof(__u_word) == sizeof(void *), "__u_word is the 8-byte machine word");
#else
#define __u_word long
#define __u_uword unsigned long
#endif
#endif

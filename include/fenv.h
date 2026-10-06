/* fenv.h -- 0.0.30 H3.  The floating-point environment functions forward to the system C library
 * on Linux and macOS; the program's floating-point arithmetic is the hardware's, so the rounding
 * mode and the exception flags they set and read are the ones its own operations use.  The
 * constants are the architecture's control and status bits (x86-64 x87/SSE, AArch64 FPCR/FPSR).
 * fenv_t and fexcept_t are opaque and at least as large as every host's. */
#ifndef _UNISA_FENV_H
#define _UNISA_FENV_H
#ifndef _WIN32
typedef struct { unsigned long __u_opaque[8]; } fenv_t;
typedef unsigned long fexcept_t;
#ifdef __aarch64__
#define FE_INVALID    0x01
#define FE_DIVBYZERO  0x02
#define FE_OVERFLOW   0x04
#define FE_UNDERFLOW  0x08
#define FE_INEXACT    0x10
#define FE_TONEAREST  0x000000
#define FE_UPWARD     0x400000
#define FE_DOWNWARD   0x800000
#define FE_TOWARDZERO 0xC00000
#else
#define FE_INVALID    0x01
#define FE_DIVBYZERO  0x04
#define FE_OVERFLOW   0x08
#define FE_UNDERFLOW  0x10
#define FE_INEXACT    0x20
#define FE_TONEAREST  0x000
#define FE_DOWNWARD   0x400
#define FE_UPWARD     0x800
#define FE_TOWARDZERO 0xC00
#endif
#define FE_ALL_EXCEPT (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT)
int feclearexcept(int __u_e);
int fegetexceptflag(fexcept_t *__u_f, int __u_e);
int feraiseexcept(int __u_e);
int fesetexceptflag(const fexcept_t *__u_f, int __u_e);
int fetestexcept(int __u_e);
int fegetround(void);
int fesetround(int __u_r);
int fegetenv(fenv_t *__u_env);
int feholdexcept(fenv_t *__u_env);
int fesetenv(const fenv_t *__u_env);
int feupdateenv(const fenv_t *__u_env);
#endif
#endif

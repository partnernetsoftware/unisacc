/* <setjmp.h> for the unisa C subset (C99 7.13).
 * setjmp and longjmp are compiler intrinsics, not library calls: setjmp
 * must capture the calling function's own frame and stack registers, which
 * a forwarded host libc call (with its own wrapper frame) cannot do.  The
 * front end recognises setjmp/_setjmp/__builtin_setjmp and
 * longjmp/_longjmp/__builtin_longjmp at the call site and emits tape that
 * saves r6 (frame), r7 (stack) and a resume label into the buffer, and
 * restores them and jumps there.  No signal mask is saved (sigsetjmp is
 * absent).  Values that live in automatic objects changed between setjmp
 * and longjmp are kept (every local lives in memory here), which is more
 * than C99 7.13.2.1 promises without `volatile`. */
#ifndef _UNISA_SETJMP_H
#define _UNISA_SETJMP_H
typedef long jmp_buf[8];
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);
int _setjmp(jmp_buf env);
void _longjmp(jmp_buf env, int val);
#endif

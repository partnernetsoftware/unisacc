/* POSIX wait status for the host cx driver on Windows.
   include/cx.h includes <sys/wait.h> before <spawn.h>. MinGW subsets often
   have no sys/wait.h, and the product include/sys/wait.h calls _ux_sym.
   -I tools/cxcc-hostinc finds this file first. The macros match the
   (exit & 0xff) << 8 encoding that cxcc_waitpid stores. waitpid itself is
   cxcc_waitpid in spawn.h. Linux and OSX do not use this file. */
#ifndef CXCC_HOST_SYS_WAIT_H
#define CXCC_HOST_SYS_WAIT_H
#include <errno.h>
#include <sys/types.h>
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)
#endif

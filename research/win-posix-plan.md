# Windows POSIX layer over forwarding — plan (2026-10-02)

Legend: [R] = read in the tree at 6a9f062; [P] = proposal, not built, not run.

## 0. What exists today [R]
- `src/back_encode.c:46` `BK_NIMP 14`, `BK_IMPS` = GetStdHandle WriteFile ReadFile CloseHandle CreateFileA
  ExitProcess GetCommandLineA VirtualAlloc VirtualProtect VirtualFree FlushInstructionCache SetFilePointer
  DeleteFileA MoveFileExA (all kernel32). `bk_imp[]` = IAT slot per import.
- `src/back_image.c:675-730` `bk_pe`: one kernel32 descriptor, ILT+IAT of `BK_NIMP+1` qwords in .rdata,
  data dir 12 points at the IAT.
- `src/back_image.c:762` `bk_win_imports`: in `-run` on Windows, walks the running compiler's own PE
  import table and copies each `BK_IMPS` entry into `bk_impval[]`; dies if any is missing. So -run gets
  imports only from what unisacc.exe itself imports — any new import must be in `BK_IMPS` (which it
  automatically is, since unisacc.exe is written by `bk_pe`).
- Syscall-shaped ops on win go through catalog `winimp`/`retconv` columns (`unisa/catalog.py:270`,
  gold `weights/gold/abi.tsv`), with `TO_WINSAVE/TO_WINREST` around the call (back_lower.c:684/694).
- libc on win = `#ifdef _WIN32` branches in include/stdio.h (fopen -> `__open` with CreateFileA
  access/disposition), unistd.h, sys/stat.h, fcntl.h, poll.h, termios.h, time.h, sys/ioctl.h:
  i.e. bodies grown per need — exactly what the owner wants to stop.
- `.hostcall/.hostaddr` refused when `bkos==2` (back_lower.c:881). Slots read at
  `BK_DATA_BASE+bk_shift-32+8*i` (back_encode.c TO_HOSTADDR x86 :880, arm :336).
  `BK_HOST_X86` / `BK_HOST_ARM` are SysV / AAPCS64 call sequences (6 args from a long[6]).

## 1. Generic channel on Windows [P]
1. `BK_IMPS` += `LoadLibraryA GetProcAddress FreeLibrary GetLastError` (BK_NIMP 18). Order chosen so
   `__hostaddr0..3` = dlopen/dlsym/dlclose/dlerror analogues (dlerror -> GetLastError returns a code,
   not a string; `unisacc_ffi.h` wraps with `#ifdef _WIN32`).
2. Slot fill: the PE loader writes the IAT, not the data page. Two options:
   (a) at entry, after TO_WINSTDH (back_lower.c:848), emit 4 copies `IAT[14+i] -> DATA_BASE-32+8i`
       (new tiny TO_WINHOST, or 4 existing load/store pairs); in -run copy `bk_impval[14..17]`.
   (b) point TO_HOSTADDR on bkos==2 directly at `bk_imp[14+i]` (IAT slot address). Simplest; no data
       write; recommended. The object path (back_encode.c:29 refusal) stays as is.
3. New `BK_HOST_WIN_X86` sequence: save rbx, rsp->rbx; `sub rsp, 32+16`; `and rsp,-16`;
   rcx,rdx,r8,r9 <- a[0..3]; a[4],a[5] -> [rsp+32],[rsp+40]; `call r11`; restore rsp from rbx; pop rbx.
   Callee-saved on Win64 adds rsi/rdi/xmm6-15 (callee preserves them, so caller is fine); tape regs
   held in rsi/rdi are safe; volatile r10/r11 must not hold live tape values (check regmap).
   No `xor eax,eax` needed (Win64 varargs pass in both int & xmm regs by convention; integer-only ok).
4. Windows arm64: AAPCS64 for integer args = existing `BK_HOST_ARM` unchanged except x18 is reserved
   (TEB) — verify the sequence never writes x18 (it doesn't in the listed words; re-check).
5. Lowering: replace the refusal with `if (bkos==2)` select the win sequence; no `bk_dyn`.
6. Gold tables: none touched — `.hostcall/.hostaddr` are TO_* ops outside catalog/abi.tsv (they are
   already in BKOPS without catalog rows). `BK_IMPS` mirror: check `exec/` and unisa for a "pe.IMPORTS"
   copy (comment at back_encode.c:46 names it) and the `kernel/` emit — regenerate if one exists.
7. fwdstub (src/fwdstub.c, main.c fe_units_fwd): enable for bkos==2 with library list
   `ucrtbase.dll, kernel32.dll, ws2_32.dll` probed in order via LoadLibraryA/GetProcAddress, cached.

## 2. POSIX layer as headers [P]
`include/sys/_win.h` (included by unistd/fcntl/sys/stat/time/stdio under `_WIN32`):
- `__w_fn(i, "name", "dll")`: table of `long __w_cache[N]`, resolved once by GetProcAddress.
- fd table: `long __w_fd[256]` HANDLEs + flags (append, text); 0..2 from GetStdHandle at first use.
- open -> CreateFileW (flags->access/disposition/share), read/write -> ReadFile/WriteFile
  (O_APPEND: SetFilePointerEx end before write), close, lseek -> SetFilePointerEx,
  fstat -> GetFileInformationByHandle, stat -> GetFileAttributesExW, unlink -> DeleteFileW,
  rename -> MoveFileExW(REPLACE_EXISTING), mkdir/rmdir -> CreateDirectoryW/RemoveDirectoryW,
  getcwd/chdir -> Get/SetCurrentDirectoryW, getpid -> GetCurrentProcessId, sleep/usleep -> Sleep,
  clock_gettime -> GetSystemTimePreciseAsFileTime / QueryPerformanceCounter, time, getenv ->
  GetEnvironmentVariableW, opendir/readdir -> FindFirstFileW/FindNextFileW, isatty -> GetFileType,
  pipe -> CreatePipe, spawn/system/popen -> CreateProcessW (+ quoting per MSVCRT rules),
  waitpid -> WaitForSingleObject+GetExitCodeProcess, sockets -> ws2_32 (WSAStartup lazily,
  SOCKET kept in the fd table with a socket bit).
- Paths: UTF-8 -> UTF-16 via MultiByteToWideChar(CP_UTF8=65001), '/' kept (Win32 accepts it);
  outputs back via WideCharToMultiByte.
- errno: `__w_errno(GetLastError())` table (mingw `dosmap`): 2,3->ENOENT(2), 5->EACCES(13),
  6->EBADF(9), 8,14->ENOMEM(12), 17->EXDEV(18), 18->ENOENT? (no more files), 32,33->EACCES,
  80,183->EEXIST(17), 87->EINVAL(22), 109->EPIPE(32), 112->ENOSPC(28), 145->ENOTEMPTY(41 ucrt),
  267->ENOTDIR(20); default EINVAL. Use ucrt numeric values.
- Rule: every body is a mapping to one Win32 call; anything with a ucrt equivalent (printf family,
  strtod, qsort, math) is forwarded to ucrtbase, not reimplemented.

## 3. Steps, each gated [P]
1. 4 imports + TO_HOSTADDR on win. Gate: probe printing `__hostaddr1()(LoadLibraryA("kernel32"),"GetTickCount")!=0`
   on win/x86_64 and win/arm64 via tests/crossnative.sh; existing win suites unchanged (STRICT=1).
2. Win `__hostcall` x86 + arm. Gate: 0..6-arg probe calling ucrt `snprintf`-free functions
   (e.g. `_ultoa`, `CreateFileW`+`WriteFile`), output byte-equal to a mingw-w64 build of the same probe.
3. fwdstub on win. Gate: tests that already pass via forwarding on osx/lnx, run on win; diff vs mingw.
4. `_win.h` fd + file I/O; delete the old `_WIN32` bodies in stdio/unistd they replace. Gate:
   file-I/O probe set (open/read/lseek/stat/rename/unlink/mkdir/getcwd, UTF-8 names) equal to mingw.
5. errno + time + dirs + env. Gate: errno probe per error path equal to mingw/ucrt.
6. process (CreateProcessW, pipe, popen) + Winsock. Gate: spawn/echo/socket loopback probes.
7. tests/nativeboot.sh: self-host on Windows still green; size budget check.
Each step: commit, then run its gate in the background, ≤60 s per run.

## 4. Size estimate [P]
Channel: +4 IAT entries (~100 B), win hostcall ~70 B x86 / ~120 B arm. Layer: ~40 wrappers x
~150-300 B + errno table ~0.5 KB + fd table bss 4 KB -> code 10-20 KB, consistent with "tens of KB";
only referenced wrappers are emitted (FTRIM `__UN_*` gating already exists).

## 5. Risks
- Win64 varargs: callee expects floats duplicated in int regs; `__hostcall` is integer-only, so
  printf-with-double via ucrt needs a float-aware variant or keep our printf.
- struct stat: ucrt `_stat64` layout differs from our sys/stat.h; fill our own struct from
  BY_HANDLE_FILE_INFORMATION, never pass it to ucrt.
- Text vs binary: we use CreateFile handles directly -> always binary, no CRLF translation; programs
  writing "\n" get LF (matches cosmopolitan; mingw ucrt stdout is text mode -> reference diffs on CRLF:
  set reference to `_setmode(_O_BINARY)` or normalise).
- Console: WriteFile to console with UTF-8 needs SetConsoleOutputCP(65001) or WriteConsoleW.
- Shadow space/alignment bugs crash only some callees — gate with ≥6-arg and stack-probe-heavy
  functions (CreateProcessW).
- x18 on win/arm64; r10/r11 liveness on x86.
- -run imports: unisacc.exe must itself carry the 4 imports — fine once rebuilt, but an old seed
  compiling a new one dies in bk_win_imports ("does not import"); bootstrap order matters.
- fd vs SOCKET vs HANDLE confusion in select/poll; ws2_32 select only takes sockets.

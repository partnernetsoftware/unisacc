# libc route table (FORWARD / KEEP / REFUSE per family × OS)

Status: **design only, nothing run.** Basis: prd.md §5.1 ruling (forward to the system libc where it
exists), research/libc-forward-handoff.md, research/libc-unify-design.md (D2).
Legend: **[read]** = taken from the tree on 2026-10-02 (include/*.h, src/fwdstub.c,
unisa/catalog.py, unisa/emit_x86.py, unisa/image/macho.py, weights/gold/abi.tsv);
**[prop]** = proposal.

## 0. What exists today [read]

- Every libc function is a `static` body guarded `#if !__UNISA_FTRIM_LIBC || __UN_<name>`.
- OS gates: intrinsics `__read __write __open __close __lseek __ioctl __fstat __stat __lstat
  __mmap __munmap __exit __unlink __rename __mkdir __fcntl __poll __ppoll __gettimeofday
  __ftruncate __getdirentries64 __getdents64` (catalog ops, per-OS numbers in the abi table) and
  `__syscall6` via `_UNISA_NR_*`/`_UNISA_SYSC` for everything else (unistd 21 uses, sockets,
  wait, clock_gettime/nanosleep in sys/_timespec.h).
- Windows: abi table `winimp` column maps a catalog op to a Win32 import
  (ExitProcess WriteFile ReadFile VirtualAlloc VirtualProtect VirtualFree CloseHandle
  CreateFileA SetFilePointer DeleteFileA MoveFileExA); `emit_x86._winbody`/`emit_arm` adapt
  argument shapes per op (fd->HANDLE translation, &written, shadow space, stack args).
  Headers branch `#ifdef _WIN32` (stdio fopen builds CreateFileA shapes, stdlib _UNISA_MAP uses
  VirtualAlloc flags); fcntl/poll/termios/unistd(part)/time(part)/sys/{stat,ioctl,select,socket,wait}
  are `#ifndef _WIN32` (refuse by absence).
- macOS: macho.py emits LC_LOAD_DYLIB libSystem and eager binds only
  `dlopen dlsym dlclose dlerror` (DLIMPORTS) into four __DATA slots.
- `-run` forwarding: src/fwdstub.c generates one C stub per host function:
  `uffi_dlsym(RTLD_DEFAULT,name)` once, then the uffi bridge (unisacc_ffi.h, `__hostcall`,
  `__hostaddr0..3`) marshals args by kind {int,ptr,float,double} × width into the host C ABI.
  unisacc_ffi.h `#error`s unless macOS and arm64/x86_64.
- Coupled state: `FILE *` is a cast fd, unbuffered (stdio.h); `errno` is `static int` per TU
  (errno.h); malloc is a size-class allocator over `__mmap` (stdlib.h); environ via
  `_unisa_environ`/`__argv`; signal.h is in-process only (no sigaction).

## 1. Families, gates, and route [read gates / prop route]

Columns lnx / osx / win. F = FORWARD (system symbol), K = KEEP bundled, R = REFUSE.
Linux is static ELF with no loader, so "F" there is impossible until dynamic linking exists:
lnx stays K (syscalls) throughout.

| header: family (functions) | gates used today [read] | lnx | osx | win |
|---|---|---|---|---|
| string.h (mem*, str*, strtok, strcoll) | none (pure) | K | K: pure, no state; forwarding adds call cost and breaks byte folding | K |
| ctype.h (is*, to*) | none | K | K: C locale only; system versions read a locale table | K |
| math.h (~60: sin..pow, f-variants, _m_* kernels) | none (fsqrt op) | K | K (determinism: libm results differ per OS; folding needs identical bits) | K |
| stdlib numeric (abs div atoi strto* qsort bsearch rand) | none | K | K | K |
| stdlib memory (malloc calloc realloc free) | `__mmap __munmap` | K | K by default; **F as a set only** (libSystem malloc/calloc/realloc/free together) when any forwarded function returns or frees heap memory (getaddrinfo, getline-by-system, strdup-by-system) | K over VirtualAlloc; F to `HeapAlloc/HeapFree` optional, never mixed |
| stdlib env/exit (getenv atexit exit abort) | `__argv __argc __exit` | K | K: getenv reads the bundled envp; atexit/exit must stay one runtime with stdio (rule 5); `_exit` may F `_exit` | K (`ExitProcess`) |
| stdio formatting (printf family, sscanf, _u_vfmt) | none | K | K: pure; system printf would need system FILE | K |
| stdio streams (fopen fclose fread fwrite fseek ftell fgets fputc getline setvbuf tmpfile ungetc ...) | `__open __read __write __lseek __close` | K | **K (FILE stays bundled)**; the fd underneath is what forwards — see unistd row. Forwarding fopen would split FILE across two runtimes | K over the fd table |
| stdio remove/rename/perror/strerror | `__unlink __rename __write` | K | F `unlink` `rename` (thin), strerror K (string table must match errno values we set) | K (DeleteFileA/MoveFileExA, already) |
| unistd file I/O (read write close lseek ftruncate fsync dup dup2 pipe unlink rmdir symlink readlink access getcwd isatty) | catalog ops + `__syscall6` | K | **F** `read write close lseek ftruncate fsync dup dup2 pipe unlink rmdir symlink readlink access getcwd isatty` — first step | read/write/close/lseek/unlink K via winimp; others R today; candidates `CreatePipe`, `GetCurrentDirectoryW`, `FlushFileBuffers`, `SetEndOfFile`, `DuplicateHandle` with own fd table |
| unistd process (fork execv* getpid getppid sleep usleep) | `__syscall6` | K | F `getpid getppid usleep nanosleep`; fork/exec* F `fork execve` (libSystem handles the Darwin fork return convention, removing a quirk) | getpid F `GetCurrentProcessId`; sleep F `Sleep`; **fork R** (no faithful mapping); exec* R or later `CreateProcessW`+wait (not exec semantics) |
| fcntl.h (open fcntl) | `__open __fcntl` | K | **F** `open fcntl` (variadic! stub must pass mode as a promoted int in the variadic slot; on arm64 Darwin variadics go on the stack) | R (fopen uses CreateFileA directly) |
| sys/stat.h (stat lstat fstat mkdir chmod) | `__stat __lstat __fstat __mkdir` | K | **F** `stat lstat fstat mkdir chmod` — struct stat layout is the Darwin 64-bit one already; verify field offsets | R today; F `GetFileAttributesExW`/`CreateDirectoryW` with own struct fill |
| dirent.h (opendir readdir closedir rewinddir seekdir telldir) | `__open __getdirentries64/__getdents64 __lseek __close` | K | F `opendir readdir closedir rewinddir seekdir telldir` **as a set** (DIR* is a libSystem object; never mix with bundled DIR) — removes the private getdirentries64 dependency | R; later `FindFirstFileW/FindNextFileW` |
| time.h (time clock_gettime nanosleep gmtime localtime mktime strftime ctime) | `__gettimeofday`, `__syscall6`, `__open/__read` (tzfile load) | K | **F** `clock_gettime nanosleep gettimeofday time` — first step. gmtime/mktime/strftime K (pure, deterministic). localtime: K (own tzfile parser) or F `localtime_r` — prefer K for byte-stable tests | time F `GetSystemTimePreciseAsFileTime`; nanosleep F `Sleep`; rest K |
| poll.h / sys/select.h (poll select) | `__poll __ppoll` | K | F `poll select` (fd_set layout = Darwin's) | R (WSAPoll only for sockets; no faithful console/file poll) |
| termios.h / sys/ioctl.h (tcgetattr tcsetattr cfmakeraw ioctl) | `__ioctl` | K | F `tcgetattr tcsetattr cfmakeraw ioctl` (ioctl variadic) | R (`SetConsoleMode` is not termios) |
| signal.h (signal raise) | `__exit` only | K | K now (in-process, documented); F `signal raise` only together with real delivery — a later decision, since a forwarded handler would be called in the host ABI | K |
| sys/wait.h (wait waitpid) | `__syscall6` | K | F `wait waitpid` | R (pairs with fork) |
| sys/socket.h (socket..recvmsg, 16) | `__syscall6` | K | F all 16 (sockaddr layouts are BSD already) | R today; F `ws2_32` (`WSAStartup socket connect...`) needs SOCKET≠fd mapping in own fd table |
| netdb.h (getaddrinfo freeaddrinfo gai_strerror) | `__open __read __close` (/etc/hosts reader) | K (hosts-only) | F `getaddrinfo freeaddrinfo gai_strerror` **as a set and only with forwarded malloc or never freeing via bundled free** (DNS resolution gained) | R / later `ws2_32 getaddrinfo` |
| arpa/inet.h, netinet/in.h (inet_*, hton*) | none | K | K (pure) | K |
| errno.h (errno) | — | K | **coupling point**: forwarded calls set libSystem's `*__error()`; the stub must copy `*__error()` into bundled `errno` after a failing return (and our errno is per-TU static: needs one shared definition first) | stub copies `GetLastError()` mapped to errno |
| locale.h (setlocale localeconv) | none | K | K ("C" only) | K |
| wchar.h (wcslen) | none | K | K | K |
| setjmp.h, pthread, dlfcn (user), iconv | absent | — | absent; dlopen exists only via unisacc_ffi | — |

## 2. Stateful coupling rules [prop]

1. **One owner per object.** FILE*, DIR*, addrinfo*, heap blocks each belong wholly to one runtime.
   Families that return such objects forward or keep as a whole set (dirent set, netdb set + free).
2. **malloc** stays bundled until a forwarded function hands back system heap memory; then forward
   malloc/calloc/realloc/free together, never one of them.
3. **errno**: make it a single program-wide object first (today `static int errno` per TU), then every
   forwarding stub does `if (failed) errno = *__error();` (Win: mapped GetLastError).
4. **stdio buffering**: bundled stdio is unbuffered over fds, so forwarding the fd layer (read/write)
   below it is safe; forwarding fopen/printf is not (system buffers would not be flushed by bundled exit).
5. **exit/atexit**: if any stdio is forwarded later, exit must be libSystem `exit` so its flushes run;
   until then keep bundled exit → `__exit`.
6. **environ**: getenv stays bundled reading `__argv` envp; forwarding setenv/getenv is a set.

## 3. Cheapest macOS first step [prop, mechanism partly read]

Families: unistd file I/O + fcntl open + sys/stat + time (clock_gettime, nanosleep, gettimeofday).
All have scalar/pointer args, no ownership transfer, layouts already Darwin's.

Needed:
- **Image writer** (unisa/image/macho.py): generalise DLIMPORTS from the fixed four to a list of
  bound symbols (one __DATA slot + one bind opcode each, ordinal 1 libSystem). The bind stream and
  slot table become per-program, driven by which `__UN_*` forwarded names survive trimming.
- **ABI adapter**: generated code uses a private convention, so each forwarded symbol needs a stub
  that marshals into the host C ABI. Two existing models [read]:
  (a) fwdstub.c/uffi: generic dlsym + kind/width marshalling at run time — works now for `-run`;
  reusable for images by resolving through the already-bound `dlsym` slot (no writer change;
  cost: one dlsym per symbol on first call). Cheapest path: emit the forwarded libc bodies as
  fwdstub-shaped C (`uffi_dlsym((void*)-2,"read")`) under `__UNISA_FWD_LIBC`.
  (b) winimp: per-op hand-written shape in the encoder (emit_x86 `_winbody`) via an import slot —
  faster, but one table row and encoder case per function; does not scale to ~60 functions.
  Recommend (a) first, (b)-style direct binds later only for hot calls (read/write).
- Variadics (`open`, `fcntl`, `ioctl`): Darwin arm64 passes variadic args on the stack; the uffi
  bridge must have a variadic mode or stubs call fixed-arity wrappers (`open` with 3 args declared
  non-variadic is wrong on arm64). Must be verified before forwarding them.
- unisacc_ffi.h currently `#error`s off macOS; that is correct for this route.

## 4. Interactions [prop]

- **Trimming (__UNISA_FTRIM_LIBC)**: forwarded bodies keep the same `__UN_<name>` guards, so trimming
  still decides inclusion; the image writer's bind list must be derived from the surviving set (an
  unused bind costs nothing at run but changes bytes). Set-forwarded families need the guard to
  pull in siblings (e.g. `__UN_readdir` implies the DIR set) — same mechanism as `_unisa_*` helpers.
- **Self-bootstrap fixed point**: the compiler itself uses file I/O + malloc; forwarding read/write/open
  changes its own image bytes on osx once, then the fixed point must be re-established (stage2==stage3).
  Keep the switch a single macro so both stages pick the same route; do not let the route depend on
  the host that runs the compiler (cross-compiling osx from lnx must emit identical bytes).
- **Six-target byte folding**: KEEP families are identical source across targets and fold; forwarded
  families diverge per OS by design (osx stubs, win imports, lnx syscalls). Folding claims must be
  restated as "pure families fold; OS-route families are per-OS". math/string/ctype/printf staying
  KEEP is what preserves cross-target determinism of program output.
- Linux: unchanged (static ELF, syscalls) unless a dynamic-loader item is opened; record that as the
  explicit exception to the ruling.

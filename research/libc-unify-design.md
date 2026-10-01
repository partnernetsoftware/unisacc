# One Unix-shaped libc across Linux, macOS, Windows — design report

Status: **design only. Nothing was run.** Every cost and claim below is an estimate to be checked by the listed experiment.
Input: the abstract brief only (no repository read).

## 1. Diagnosis

### What Cosmopolitan actually does
- **Linux/BSD:** raw system calls, its own libc (musl-derived pieces).
- **macOS x86-64:** raw `syscall` (Apple tolerates it but does not guarantee ABI). **macOS arm64:** cannot use raw syscalls reliably, so the APE loader
  is a small native Mach-O that `dlopen`s libSystem and Cosmopolitan forwards several calls through it. So on macOS it is a *mix*: polyfill where
  stable, forward where Apple forces it.
- **Windows:** **polyfill on Win32 (kernel32/ntdll/ws2_32/advapi32), not on msvcrt/ucrt.** It keeps its own fd table (`g_fds`) mapping ints to
  HANDLEs with a kind tag (file, console, pipe, socket), translates paths (`/c/x` → `C:\x`, `/` separators, UTF-8 → UTF-16), maps Win32 errors to
  its own errno *numbers that are variables chosen per OS at start-up*, emulates signals (console ctrl handler + polling at syscall boundaries +
  SEH for SIGSEGV), emulates `fork` by copying the address space into a suspended child (large, fragile), implements `poll` over sockets via
  WSAPoll and over other handles by polling loops, and `execve` via CreateProcess with command-line quoting.
- Win32 symbols are imported by a fixed import table — same shape as this compiler already has.

### What transfers to a header-bodied, on-demand library
| Transfers well | Does not transfer |
|---|---|
| fd table over HANDLEs with kind tag (small, static data in a body) | fork emulation (needs whole-image control and a custom loader) |
| Win32 error → errno map (one table, ~60 entries) | runtime errno renumbering (we fix errno per target at compile time instead — better for determinism) |
| path translation + UTF-8/UTF-16 conversion | single binary for all OSes (APE); we compile per target already |
| CreateProcess + argv quoting for `posix_spawn`/`system`/`exec*`-then-exit | signal delivery into blocking calls (only partial) |
| WSAPoll-based `poll`/`select` for sockets | ptys/termios beyond ConPTY basics |
| identifier-closure trimming = Cosmopolitan's "only link what you use" | |

Key point: the identifier closure already gives what Cosmopolitan gets from static linking + `--gc-sections`. The library can grow by bodies
without growing the shipped binary beyond header text (deflate-compressible).

### Forward vs reimplement, per family (owner's forwarder framing)
| Family | Windows | macOS (both ISAs) | Why |
|---|---|---|---|
| file I/O open/read/write/close/lseek/fstat | **forward to Win32** (CreateFileW/ReadFile/...) via fd table; **not** ucrt `_open` | forward to libSystem *or* keep syscalls on x86-64; arm64: prefer libSystem | ucrt fds are their own table, mixing with sockets is impossible; Win32 is the stable ABI |
| stat/dirent/realpath/unlink/rename/mkdir | forward to Win32 (GetFileAttributesExW, FindFirstFileW, MoveFileExW) + path adapt | forward | pure adaptation |
| time (clock_gettime, nanosleep, gettimeofday) | forward (QueryPerformanceCounter, GetSystemTimePreciseAsFileTime, Sleep/waitable timer) | forward to libSystem (it has them) | removes current macOS equivalents |
| sockets | forward to ws2_32, socket handle stored in fd table | forward | SOCKET ≠ int; adapt errno from WSAGetLastError |
| poll/select | adapter: WSAPoll for sockets, refuse/limited for others | forward | the hard part on Windows |
| process: posix_spawn, waitpid, system, popen | forward to CreateProcessW/WaitForSingleObject | forward | |
| fork | **refuse** (ENOSYS, compile-time diagnostic preferred) | forward | see §4 |
| signals | minimal: raise/signal for SIGINT/SIGTERM/SIGABRT/SIGSEGV via console handler + SEH; kill() only SIGKILL/SIGTERM→TerminateProcess | forward | |
| termios/pty | forward subset onto console modes (ENABLE_VIRTUAL_TERMINAL_*); tcgetattr/tcsetattr raw/cooked only | forward | |
| mmap/mprotect | forward to VirtualAlloc/CreateFileMapping (anonymous + read-only file maps) | forward | |
| stdio/string/math | keep own bodies (already shared, deterministic) | keep own | no reason to forward; determinism |
| errno values | own fixed per-target numbering (Linux numbers on Windows, documented) or ucrt numbers — pick one, see design B | libSystem values | |

macOS note: forwarding requires the produced Mach-O to import libSystem (normal for any macOS binary; dyld does the loading, not us). Raw
syscalls on arm64 macOS are an Apple-unsupported ABI; forwarding there reduces risk. `-run`'s dlsym+libffi path already proves the forward shape.

## 2. Designs

### D1. Cosmopolitan-style Win32 polyfill, header-bodied ("cosmo-lite")
- Windows: fd table over HANDLEs (kind: file/pipe/console/socket/dir), path translation, Win32 error→errno table, CreateProcess spawn,
  WSAPoll, fork refused. macOS/Linux unchanged (syscalls).
- Cost: ~3–5k lines of bodies over time; zero binary growth beyond header text; import table grows by ~80 Win32 symbols.
- Experiment (1 day): fd table + open/read/write/close/lseek/fstat on Windows; probe program prints results; diff against mingw-w64 build of
  same probe in the Windows VM.

### D2. Forwarder everywhere ("thin shim") — owner's framing
- Each POSIX entry is a static body that adapts types/errno/paths and calls the system: Win32 on Windows, libSystem on macOS (both ISAs),
  syscalls stay on Linux (there is no stable libc ABI to forward to without glibc/musl choice).
- Same fd-table core as D1 on Windows (unavoidable: Win32 has no int fds); on macOS bodies become one-line forwards, deleting the macOS
  syscall-number table and the pipe2/clock_gettime equivalents.
- Cost: Windows same as D1; macOS *shrinks*. Needs libSystem import support in the Mach-O writer (likely present for `-run`'s host path? verify).
- Experiment: macOS arm64: forward `clock_gettime`, `open`, `read` to libSystem in a built (not `-run`) binary; probe vs host cc byte-equal output.

### D3. Forward to ucrt POSIX-ish names (`_open`, `_read`, `_stat64`, `_spawnvp`)
- Windows: call ucrt's `_open` family directly; ints come from ucrt's table.
- Cost: lowest code. Problems: sockets are not ucrt fds (no unified poll/close), ucrt path is ANSI-codepage unless `_wopen`, CRT init/ownership
  questions, errno is ucrt's. Good only for files.
- Experiment: `_open/_read/_close` probe vs mingw; then try `close()` on a socket — expected to demonstrate the split.

### D4. midipix-style: POSIX personality as a separate runtime DLL
- A real POSIX subsystem (own DLL, own process model). Correct and complete; violates "small, no loader of our own, no prebuilt library". Listed
  as precedent only.

### D5. Cygwin/MSYS2 newlib: link against cygwin1.dll / msys-2.0.dll
- fork works, full POSIX; requires the external DLL at run time, GPL/LGPL license, semantics differ from native Windows (paths, ctty). Fails "native
  Windows unaffected" and "no huge dependency". Optional external target at most.

### D6. mingw-w64 CRT model: native Windows libc + a small POSIX-extras header set
- Ship only what mingw ships (unistd.h subset: access, getcwd, unlink, `_mkdir`; no fork, no sockets in unistd). Programs needing more get a
  clear refusal. Cheapest, very native; does not reach "ordinary Unix programs compile unchanged".
- Experiment: compile the existing POSIX probe set against a mingw-subset header layer; count how many pass.

### D7. wine reversed / tcc-win32 headers: native Win32 headers + POSIX opt-in layer
- Ship tcc-like `<windows.h>` subset (already partly done via import table) and keep POSIX in a separate header layer enabled per translation
  unit. This is a *layering* design that combines with D1/D2 rather than replacing them (see §5).

### D8. Hybrid by family (D2 core + D3 for leaves)
- fd table owns files/pipes/sockets (D2); leaf functions with no fd involvement (getenv, getcwd, time conversions, `_fullpath`) forward to ucrt
  or kernel32, whichever is simpler. Keeps code small.

## 3. Recommendation

**Primary: D2 (forwarder with a Win32 fd table) layered by D7 headers.** Matches the owner's framing, removes macOS syscall risk on arm64,
keeps Linux on syscalls.
**Fallback/contrast: D6 (mingw-subset refusal-first) as the floor** — ship each POSIX family on Windows only after its gate passes; until then
the mingw subset plus named refusals is what users get. Never claim beyond the passing gate.

Order of work, each step gated (gate = probes byte-equal against the reference on every listed target; otherwise the family stays refused):

| Step | Content | Gate |
|---|---|---|
| 0 | Header layering skeleton: `<unistd.h>` on Windows resolves to the posix layer only under the default-on POSIX mode; `<windows.h>` TU without POSIX headers unchanged | existing Windows suites green; a `<windows.h>` program and a POSIX program both build; symbol-clash probe (program defining its own `read`) still links |
| **1 (first gated step)** | **Windows fd table + open/read/write/close/lseek/fstat/stat/unlink, path + UTF-8 translation, Win32 error→errno map** | **probes vs mingw-w64 build on win/x86_64 and win/arm64 in the VM: identical output; errno table probe covers ENOENT/EACCES/EEXIST/EBADF/EISDIR** |
| 2 | macOS forwarder: route file I/O + time to libSystem on osx/arm64 and osx/x86_64; delete pipe2/clock_gettime equivalents | existing macOS POSIX probes byte-equal vs host cc on both ISAs; deterministic output hash unchanged for programs not using these |
| 3 | Windows dirent (FindFirstFileW), getcwd/chdir/mkdir/rmdir/rename, time family | probes vs mingw; directory listing sorted in probe to avoid order dependence |
| 4 | pipes + posix_spawn/waitpid/system/popen via CreateProcessW (argv quoting per MS rules) | spawn/echo/exit-code probes; quoting torture probe vs mingw `_spawnv` |
| 5 | sockets in fd table (ws2_32, WSAStartup lazily), poll via WSAPoll for sockets, pipes via PeekNamedPipe | loopback round-trip probe on Windows; poll on a non-socket non-pipe returns documented error |
| 6 | signals minimal set, isatty/termios subset over console modes, mmap subset | per-function probes; everything else keeps named refusal |
| — | fork/vfork, full termios, ptys, sigaction with SA_RESTART semantics | stay refused on Windows |

## 4. Tempting ideas that conflict with the constraints
- **fork on Windows** (Cosmopolitan/Cygwin): needs address-space copying, a custom loader-like control of the image, and is racy; cannot be
  probe-verified deterministically. Refuse, and point to posix_spawn.
- **Full POSIX layer DLL** (midipix/Cygwin): a prebuilt runtime library + its own loader/process model; violates "small" and "no own loader".
- **Linking msvcrt/ucrt for everything:** fd namespace split from sockets, ANSI path defaults, CRT-owned errno and locale, nondeterministic
  behaviours (e.g. printf formatting of floats differs) — would also break byte-equality of stdio vs the other targets.
- **Runtime errno renumbering (Cosmopolitan's errno variables):** makes `case EINTR:` non-constant; breaks C switch usage and determinism. Fix
  errno per target at compile time.
- **APE-style one binary for all OSes:** not needed; per-target output already exists; adds loader complexity.
- **Signal delivery into blocking ReadFile:** would require alertable I/O rewriting of every call; out of scope.
- **Forwarding Linux to glibc:** no stable choice of libc on the host and breaks static, loader-free binaries; keep syscalls there.
- **Generating the POSIX layer as model weights/tables:** large table growth; this is plain library code, keep it as header bodies.

## 5. Keeping native Windows/macOS programming unaffected
- **Header layering:** POSIX headers (`unistd.h`, `fcntl.h`, `sys/*`, `dirent.h`, `poll.h`, `termios.h`) are a separate layer. `<windows.h>`
  never includes them; including both in one TU must work (test: a probe including both).
- **Namespacing of internals:** all helpers and the fd table use reserved names (`__ux_fd_get`, `__ux_errno_from_win32`); only standard POSIX
  names are public. Bodies are `static`, so a user's own `read` in another TU does not clash at link; in the same TU, user definition wins
  rule documented and probed.
- **No hijacking of Win32 semantics:** HANDLE-level code is untouched; POSIX fds are a parallel world with bridge functions
  (`_get_osfhandle`-like `__ux_handle(fd)` and `__ux_fd_from_handle(h)`; consider mingw-compatible names `_get_osfhandle`/`_open_osfhandle`).
- **Default vs opt-in:** POSIX layer **on by default when a POSIX header is included** (that is the opt-in — no macro needed);
  `-D__UX_NO_POSIX` (or a driver flag) restores today's refusal for users who want strict native mode. Windows-only names like `_open`,
  `_stat` keep mingw/ucrt meaning and are not redefined.
- **macOS frameworks:** forwarding to libSystem makes the POSIX layer *identical* to what framework code sees (same fds, same errno), so
  mixing is naturally safe — another argument for D2 over syscalls on macOS.
- **errno:** on Windows choose one documented numbering (recommend ucrt/mingw values for the shared range so `<errno.h>` matches
  mingw-built objects, with POSIX-only codes like EWOULDBLOCK/ECONNREFUSED taking ucrt's `errno.h` values 100+). Probe against mingw.
- **Main/startup:** UTF-8 argv/environ only when the POSIX layer is in the closure; `WinMain`/`wmain` programs unaffected.

## 6. Open questions to verify (not run)
- Does the Mach-O writer already import libSystem symbols in built (not `-run`) binaries? Step 2 depends on it.
- Is mingw-w64 available for both win/x86_64 and win/arm64 in the VM as the reference (llvm-mingw covers both)?
- Import table growth: ~80–120 Win32 symbols across kernel32/ws2_32/advapi32; confirm the table format has no size cap.

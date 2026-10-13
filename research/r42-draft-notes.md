**Draft, unsigned.** This draft carries the unsigned candidate for review. It is not signed, not the latest release, and not published. Latest remains v0.0.41.

## What changed in the product

- **Host C library forwarding now works under `-run` and the default run on Linux, and the compiler stays a static image.** A program that calls host libc functions is finished as the same dynamic image `-o` writes, placed in an anonymous memory file and executed from it; arguments, environment, standard streams and exit status are the program's own. This needs the host's glibc dynamic loader. Where an anonymous memory file cannot be created or executed, or the loader cannot be opened, the run is refused with a message that says so and points to `-o`. Library mode still refuses host forwarding.
- **Library mode: guest memory is owned by the context.** A guest `mmap`/`munmap` in library mode now goes through the library's own callbacks instead of a raw system call, so mappings are tracked and released with the context. Previously each run leaked a 1 MiB mapping.
- **Library mode: steady memory across repeated compile and run.** The model package and decoded models are cached per context instead of being re-read and re-decoded on every call. Resident size no longer oscillates.
- Reading a directory as an input reports "cannot read" again (a regression introduced and fixed within this cycle).
- The seed builds with GCC 14.

## Known limitations

- Host forwarding under `-run` on Linux needs a glibc dynamic loader on the host. On a system without one, write the program with `-o` and run it where glibc is installed.
- Library mode rejects `MAP_FIXED`, and `munmap` only accepts ranges the context mapped.
- A model package file is treated as immutable while a context that loaded it is alive.

## Evidence for these bytes

`unisacc.com` sha256 `ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f`, sealed from commit `20f3a23c`, release bound to `rc/v0.0.42` at `bee2a3ff`.

- **release-check** run 38426947450 on the rc commit: all six candidate cells (macOS arm64 and x86_64, Linux x86_64 and arm64, Windows x86_64 and arm64), the six-way matrix comparison and the Windows suite succeeded.
- **darwin-gate** run 38387935866: the six suites that need a Darwin host passed on macOS runners against the same bytes.
- **Local gate** (Linux x86_64 cloud host), 722 suites: 632 passed and **no product failure** (no wrong output, compile error or sanitizer report). The run's exit status is 1 because the remaining 90 are not passes: 28 not executed because a predecessor timed out, 39 timeouts recorded against the host baseline (nine of them ruled on 2026-10-13; eight of those nine pass when run alone and one is a suite that is too large for its limit), 2 checks where the host's own reference compiler or assembler rejects the reference source, 7 with memory evidence unknown by design, and 14 that this host cannot verify (six of those are covered by darwin-gate above; eight still need a Lima guest, zig or a macOS linker). The owner accepted a fully classified exit table plus the remote evidence as the basis for opening this draft; details are in `research/c42-7000-final-bucket.md` and `research/c42-ruling-request.md`.

## Not verified

- The new library `mmap` route and the exec-on-demand forwarding path have no dedicated test on macOS or Linux arm64; the six-cell run exercises general programs.
- Behaviour where anonymous memory files are forbidden (`vm.memfd_noexec`) or on kernels without `execveat` was not run.

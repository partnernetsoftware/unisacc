# Ordinary output files commit after a complete write

`exec/c/compiler.c` previously opened every named output with O_TRUNC. Under a
real kernel RLIMIT_FSIZE=1024 and ignored SIGXFSZ, the installed 79fb84e6 correctly
reported a short write but left a truncated image, destroying previous contents.

POSIX ordinary files and missing destinations now use an exclusive sibling
temporary file. Write and close must succeed before rename commits it. Errors
remove only the staging path, never the destination. Temporary names include PID
and a bounded collision retry; creation uses O_EXCL/O_NOFOLLOW/O_CLOEXEC. EINTR
is retried. The existing output timing stamp includes the commit operation.

Filesystem contract:

- New/replaced ordinary files use mode 0755 subject to umask. Replacement creates
  a new inode: other hardlinks retain old contents; old owner/mode/ACL are not
  copied. Directory permissions govern replacement.
- A parent that cannot host a staging file is a named refusal; there is no
  fallback to truncating the destination.
- Symlinks, devices and other non-regular paths retain direct-write behavior.
  Neither they nor stdout are renamed or removed on failure. Symlink writes can
  still truncate their referent on error; this patch does not claim rollback for
  that route. Concurrent external directory-entry changes are not serialized.
- Windows retains its previous direct-write path and has no rollback claim.
- This is atomic visibility after close, not a crash-durability/fsync promise.

`tools/fsize-exec.cx` imposes actual Linux x86_64 kernel sigaction/setrlimit before
exec. Bundled signal() is only an in-process emulation and is deliberately not
used. The launcher was built with installed unisacc, SHA
`4d8d70d97f02e7189aeb8f772963bb2fe9aac9702b78015c943edafd8a3841ef`.
`tools/output-file-cases.cx ROOT PRODUCT LAUNCHER NEW_ABS_OUT` checks fresh and
existing output failure, successful image execution, symlink write-through,
both /dev/null and /dev/full retaining device type, hardlink separation, and no
staging residue. Each compiler invocation is bounded to eight seconds.

Installed baseline: `out/c43-output-before-03.stdout`, 9 assertions / 3 failures
(new file residue, existing file loss, and the new hardlink-separation contract).
Earlier `-01` could not compile because the bundled runtime lacked `link`;
`-02` used a raw `_start: ret` fixture inappropriate for a standalone process.
Both are retained, not qualification evidence. Final fixture is literal C
`int main(void){return 7;}`; its independent expected exit value is 7.

Private product driver built by installed unisacc with
`-O2 -I include -iquote exec/c -b lnx/x86_64 exec/c/asmcompiler.c`:
SHA `3bdcc65e3633df2aedb90ea6687fd1f529929377ab9310d67a28d31fb66fd46f`,
explicit package 7b806ec2. Wrapper `out/c43-output-private.com` only dispatches
that ELF with the package. Result `out/c43-output-private-01.stdout`: **9/0**,
with raw child output in the same-named directory. No installation or foreign
platform qualification; the matrix retains an open product-adoption cell.

Boundary follow-up: appending the staging suffix to a 240-byte destination
basename exceeded the filesystem component limit. The first private driver
therefore failed the expanded suite 1/12 (`out/c43-output-boundary-before-01`).
Staging now uses a short `.unisacc-PID-attempt.tmp` basename in the same directory,
independent of the destination basename. The expanded suite also checks an
unwritable parent with a writable old target (refusal and old bytes preserved)
and a read-only old file in a writable parent (replacement remains executable).
Permission checks refuse to run as root; the suite is Linux x86_64 only.

Self-built private driver
`dcc86125d2e309034ac28d6b4c2c0d2a12c9a9e1930f30bc30146b72fd059c79`
with PC7b806ec2 passes **12/0**, `out/c43-output-boundary-after-01.stdout`.
No new installation or matrix promotion. Collision retries remain bounded and
exclusive; forced collision exhaustion has not yet been exercised.

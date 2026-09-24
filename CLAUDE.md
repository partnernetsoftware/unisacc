# Working rules for this repo

## Testing: local machines, not GitHub Actions

**Do not push in order to test.** The repository is public now, so the hosted
runners are free and `.github/workflows/ci.yml` fires on push again — but CI
is a *safety net*, not the test loop. Get the local suites green first and let
CI be the second opinion on a machine nobody has been editing.

The reason the rule exists: while the repository was private, a push-triggered
matrix of one Linux and two macOS jobs — the macOS ones billing at ten times
the Linux rate — exhausted the organisation's Actions budget in a single
session of pushing after every increment. Batch the work; push once.

All three platforms are here:

| platform | how | covers |
|---|---|---|
| macOS arm64 + x86_64 | `./tests/all.sh`, `./tests/fat.sh` (Rosetta) | osx/arm64, osx/x86_64 |
| Linux | `./tests/linux.sh` — the whole suite inside a local Lima VM. `LIMA_VM=minicon-lnx-x86_64` is x86_64 on this arm64 host, so it is **emulated**: linux.sh notices the architecture mismatch and multiplies the watchdogs by ten, because seven suites once "failed" by timing out while working. It has no repo mount, so the tree is piped in; `gcc` was installed there on 2026-09-23 | lnx/arm64, lnx/x86_64 |
| Windows 11 | `./tests/crossnative.sh` — the local UTM machine, driven by `utmctl` | win/arm64, win/x86_64 |

`tests/crossnative.sh` skips a target whose VM is not up, so start the UTM
machine before relying on the Windows result — and **stop it afterwards**, it
is expensive on CPU.

**Why Linux goes through Lima and not UTM** (measured, 2026-09-21, so nobody
has to re-litigate it): UTM does have `minicon-lnx-arm-64` and
`minicon-lnx-x86-64`, but neither has the QEMU guest agent installed, so
`utmctl exec`, `utmctl file` and `utmctl ip-address` all fail on them with
*"QEMU 客户机代理没有运行或未安装在客户机上"*. Their network is Shared with
no port forwards and their MAC never reaches the host ARP table, so there is
no SSH route either. The Windows VM **does** have the agent, which is why
UTM drives Windows. If the agent gets installed in the Linux machines, move
both `tests/linux.sh` and `crossnative.sh`'s Linux targets over to `utmctl`
and drop the Lima dependency.

The Lima VM mounts the repo **read-only**, so `tests/linux.sh` copies the tree
into the guest (minus `.git` and `corpus/`, with `corpus/` symlinked back —
the suites only read it) before running.

A skipped target is not a passing target: `crossnative.sh` names what it
skipped and `STRICT=1` makes a skip a failure. Every suite also fails when
it checked *nothing* — `closure.sh` with no probes used to print
`identical 0 differ 0` and exit 0.

Green on macOS is not green. `difftest` compares against the *system*
compiler, and glibc and BSD libc disagree about things C only calls
"unspecified" — that is a real class of failure, and `tests/linux.sh` is what
catches it.

## Long commands

Anything that might run for more than a minute goes in the background
(`run_in_background`) or carries an explicit bounded timeout. A foreground
suite once blocked the session for two hours.

## Running the suites

Things that cost time here before they were written down:

- **`all.sh` prints nothing until it finishes.** Each suite writes to its own
  file and the summary comes at the end, so a log of zero bytes means
  *running*, not stuck. Individual suites (`./tests/closure.sh …`) print as
  they go — run those when progress needs to be visible.
- **Do not edit the tree while a suite runs.** A run that overlapped edits to
  `lower.py` reported 59 mismatches from images that were never written, and
  the whole run had to be thrown away. Finish the edit, rebuild, then start.
- **Every inner step gets its own watchdog**, not just the outer command. An
  ablation run with no per-compile timeout hung for twenty minutes on one
  looping compile. macOS has no `timeout`: use
  `perl -e 'alarm N; exec @ARGV' …`.
- **`alarm` binds only the process it execs**, not anything that process
  starts. A test that BUILDS a program and then runs it has two things to
  bound, and the second is the dangerous one: `cc -o p f.c && ./p` leaves
  `./p` unbounded. A generated program that never terminated once span at
  99.8% CPU for the better part of an hour after the harness that started
  it was gone, and the heat was blamed on the suites. When the machine is
  hot, look for a runaway FIRST:
  `ps -eo pid,pcpu,command | awk '$2 > 20'`.
- **`pkill -f` fails under this shell's locale** ("illegal byte sequence").
  Kill by PID (`ps -eo pid,command | grep …`).
- `nativeboot` is the long pole (~4 min): it compiles `unisacc.c` with
  unisacc three times over, then cross-writes five more targets.

## Pushing

Batch the work and push once. Every push used to trigger three billed jobs.

## Building and releasing

**CI tests; it does not build.** Every shipped artifact is cross-compiled in
ONE environment locally -- that is the whole point of a compiler that writes
all six targets itself -- and then moved through a GitHub release (a draft
while it is in flight) as the transfer mechanism. GitHub Actions is only ever
a second opinion on a clean machine, never the thing that produces the
binary. Local cross-machine testing goes through the UTM machines
(`tests/crossnative.sh`, and the `utm-court` helper that lives outside this
repo) and the Lima Linux VM.

v0.0.1 (2026-09-23) was released this way: `python3 -m unisa ape` built
`unisaccrun.com` here, `gh release create` uploaded it.

## The route

Construction, not training. The shipped weights are derived from the gold
tables (`unisa build-weights`, exact by construction, verified by enumeration);
`unisa train` is a control arm and is not on the shipping path. Every
`--drive` default is `built`. Do not run training without being asked — it is
minutes of full-core work and it has overheated this machine before.

# Working rules for this repo

## Testing: local machines, not GitHub Actions

**Do not push in order to test.** The GitHub runners are metered and the two
macOS ones bill at ten times the Linux rate, so a push-triggered matrix spends
real money on work this desk does for free — it once exhausted the
organisation's Actions budget mid-session.

The workflow is **parked**: it sits at `.github/workflows-disabled/ci.yml`,
and GitHub only reads `.github/workflows/`, so nothing can fire it — not a
push, not a pull request, not `gh workflow run`. Move it back only when the
user asks for a clean-room check, and move it out again afterwards:

```bash
git mv .github/workflows-disabled .github/workflows   # arm
gh workflow run ci
git mv .github/workflows .github/workflows-disabled   # park it again
```

All three platforms are here:

| platform | how | covers |
|---|---|---|
| macOS arm64 + x86_64 | `./tests/all.sh`, `./tests/fat.sh` (Rosetta) | osx/arm64, osx/x86_64 |
| Linux | `./tests/linux.sh` — the whole suite inside the local Lima VM | lnx/arm64, lnx/x86_64 |
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

Green on macOS is not green. `difftest` compares against the *system*
compiler, and glibc and BSD libc disagree about things C only calls
"unspecified" — that is a real class of failure, and `tests/linux.sh` is what
catches it.

## Long commands

Anything that might run for more than a minute goes in the background
(`run_in_background`) or carries an explicit bounded timeout. A foreground
suite once blocked the session for two hours.

## Pushing

Batch the work and push once. Every push used to trigger three billed jobs.

## The route

Construction, not training. The shipped weights are derived from the gold
tables (`unisa build-weights`, exact by construction, verified by enumeration);
`unisa train` is a control arm and is not on the shipping path. Every
`--drive` default is `built`. Do not run training without being asked — it is
minutes of full-core work and it has overheated this machine before.

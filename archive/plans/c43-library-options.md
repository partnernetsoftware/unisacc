# Reject invalid compilation options before invalidating a generation

The public library previously rejected null target/invalid optimisation early,
but arbitrary non-null target strings reached compilation after discarding the
old image, tape and late-binding state. This violated the existing
`lib.compile.us_compile.invalid-options` requirement that a rejected option
preserve the previous callable generation.

`us_compile` now validates the exact six target spellings (lnx/osx/win paired
with x86_64/arm64) before any state mutation. This deliberately does not reuse
the narrower POSIX host-binding target set. The existing error is
`invalid compilation options`; optimisation remains 0..2. No language or model
semantics changed.

`tests/cases/library_option_guard.c` is a host ABI fixture linked with a private
runtime and explicit PC7b806ec2. It first compiles and calls a function whose
literal answer is 37. After each invalid option it checks the exact error,
preserved tape bytes, the same exported pointer, and another call returning 37.
It never calls a potentially stale pointer after detecting a changed generation.

Baseline: `out/c43-library-options-before-01.stdout`, 24 assertions / 15 failures.
Empty target and four unknown/malformed names lose the prior generation; null
target and optimisation -1/3 already preserved it. No baseline evidence was
overwritten.

Fixed and expanded: `out/c43-library-options-after-01.stdout`, **45/0**, empty
stderr, exit 0. It additionally tests all six valid target spellings with tape
input, rejects a subsequent unknown target while retaining that tape, and
actually compiles/relocates/calls Linux exports at optimisation 0, 1 and 2.
Foreign target acceptance is an options test, not foreign execution.

Build (host GCC/libffi dependency, each command bounded to 55 seconds):

```
tools/bound 55 /usr/bin/x86_64-linux-gnu-gcc-11 -std=c11 -O2 -w -I . tests/cases/library_option_guard.c exec/c/librarycall_x86_64.S -lffi -ldl -o out/c43-library-options-after.elf
tools/bound 55 out/c43-library-options-after.elf out/c43-fallguard-mixed-pc-01/compiler.pkg
```

This is private-runtime evidence. The matrix records the reproduced product
defect and source repair without granting installed-library or release
qualification. The shared installed APE remains 79fb84e6.

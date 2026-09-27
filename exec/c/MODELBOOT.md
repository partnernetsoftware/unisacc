# Fixed-package model-driver bootstrap

This tests the actual model product on macOS arm64 without changing the default product.
`MODEL_COM` is mandatory and must already exist; the test never builds a reference replacement.
Run each step separately (each owns bounded subprocesses, within a 55-second window):

```sh
export MODEL_COM=/absolute/finished/unisacc-next.com
python3 exec/c/modelboot.py prepare /tmp/my-modelboot
python3 exec/c/modelboot.py bootstrap /tmp/my-modelboot
python3 exec/c/modelboot.py probe /tmp/my-modelboot
```

Prepare requires an empty private directory, validates embedded P2 framing and N-format
model bodies, checks required routes/resources, and snapshots source and package hashes.
Bootstrap compiles `asmcompiler.c` with the model candidate, then compiles the same source
with N1 using exactly the same package. It compares complete N1/N2 Mach-O bytes, including
signatures. Probe runs N2 with empty PATH and no external kernel setting, compares O0/O2
memory/native results to host cc, and checks that a missing explicit package rejects.

The manifest rejects candidate/source/test changes between steps; prepare a new directory
for a changed input. Python is offline orchestration and checking, never a compiler stage.
The package supplies fixed NET models, carried headers, and assembly kernels. This proves
a **model driver fixed point with a fixed package**, not rebuilding the model package,
assembly kernels, complete APE container, or six-platform bootstrap. It does not compile
`unisacc.c` and then silently continue with the classic compiler.

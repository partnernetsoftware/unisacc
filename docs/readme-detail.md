# README 详述(自 README.md 迁出,逐字保留)

本文件是 README 精简时迁出的、prd 系列文档与 docs/ 中查无对应文字的内容,正文逐字保留,不再更新。其中版本号、CI 编号、字节数、测试通过数均是写入当时的事实,不代表当前候选;当前状态以 prd.md 与 prd_release.md 为准。ARCHITECTURE.md、docs/toolchain.md 等处写的"见 README 限制表""见 README"即指本文件的对应小节。

## 一、旧首页简介(已冻结)

macOS：镜像声明最低版本13.0；本次在26.5.1实测，13.0本身未实测。发布镜像仅有ad-hoc签名，Gatekeeper评估未认可；上面的命令行运行成功不代表Gatekeeper信任通过。Safari直连GitHub曾返回504；同SHA官方资产经本机HTTP转存后由Safari下载，天然quarantine标记前后保留，私有缓存下版本及hello运行成功。该测试没有删除隔离标记或关闭系统保护，也不等于Safari直连公开资产成功。

## What is not claimed

- Windows原生自举尚未声明；本机macOS arm64已实测固定包、44根编译器裸片gen2==gen3（见 [prd.md](../prd.md)）。这不是重建NN、宿主工具或完整六片APE的自举证明。
- Windows stdio-crossunit and stream-pipe cover portable subsets; POSIX
  sub-checks were not run and are not counted as passed.
- Full-batch qualification is separate from native CI; see
  [GitHub Releases](https://github.com/partnernetsoftware/unisacc/releases) and [plans/release-notes-0.0.58.md](../plans/release-notes-0.0.58.md) for the session state.

A C99 compiler for six targets -- {Linux, macOS, Windows} x {x86-64,
arm64} -- that writes the executables itself (ELF, Mach-O with an ad-hoc
signature, PE) directly from its own encoders, compiles itself, and ships as one
file, `unisacc.com`. Toolchain status (0.0.46): the c46-3 APE is
6,163,990 bytes, SHA256
`884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf`.
Native CI run [42353236985](https://github.com/partnernetsoftware/unisacc/actions/runs/42353236985)
reports QUALIFIED 42/0 across six targets (identity, version, hello, default
run, stdio-crossunit, stream-pipe and smoke). Windows stdio-crossunit and
stream-pipe cover only the portable subset, not the POSIX sub-checks.
Windows arm64 uses the shipped `slice-win-arm64` with `pkg.bin` and an explicit
`UNISA_CONTAINER`. See [GitHub Releases](https://github.com/partnernetsoftware/unisacc/releases) for
artifact identities, platform limits and the final full-batch session state.
`-o` writes ELF, Mach-O with an ad-hoc signature, or PE32+; the default `-run`
executes in memory on Linux and from a private temporary file on macOS.
Relocatable objects are Linux only. `-c -b lnx/x86_64|lnx/arm64 -o FILE.o` writes an ELF
`ET_REL` object; GNU `ld` links that whole-program object into a runnable static
program on its own, and `cc` cannot (`_start` collides with the crt, and the
calling convention is not SysV or AAPCS64).
当前 main 的开发修复支持 `--target T` / `--target=T`，只选目标，不覆盖 `-E/-S/-c` 模式；
裸 `-c` 默认宿主目标并写 `.o`，可用 `-o` 指定路径。非 Linux 的 `-c` 使用已有 unit 合同：
Mach-O/COFF 容器只携带 unit tape，供 unisacc 自己链接，不是系统链接器对象。
`-c --tapebin` 与 `-S` 保留显式 tape 输出。当前 main 的裸 `-S` 在两种 macOS slice 上按本片平台预定义 C 宏；其余宿主保留 lnx/x86_64 默认。显式目标、已有 tape/tapebin 的来源路由及裸 `-S -o bare.s` 的 tape 输出合同保留。公开 0.0.47 原资产未含此开发修复，身份见
[本地记录](../plans/root-identity.md)与[发布记录](https://github.com/partnernetsoftware/unisacc/releases)。
`-c -b T -funit` 显式选择 unit 对象；`unisacc a.o
b.a [-o prog]` links unit objects and archives, and `unisacc ar rcs|t|x` writes
them (measured: two unit objects plus an archive, linked and run). The product
writes no assembly text: `-S -b lnx/ARCH -o FILE.s` stops with
`NOT_IMPLEMENTED object` (exit 2), the osx/win names are refused as "written for
Linux targets in this version", and bare `-S` writes the tape IR, which
`unisacc as` rejects ("directive not in the subset"). `unisacc nm` and `unisacc
objdump -d` read the ELF objects. The archived reference compiler is what wrote
ELF, Mach-O and COFF objects and GNU assembly for all six targets; assembly text
for Mach-O and COFF, Intel syntax and inline `asm` are not scheduled (moved to
the 0.1.x appendix). The design and the back-end facts behind it are in
[docs/toolchain.md](toolchain.md) and [prd.md](../prd.md). "C99" is ISO/IEC 9899:1999 as amended by TC1-TC3 (the text is WG14
N1256), and how much of it is covered is a number from a clause-by-clause
ledger, not a claim -- see [C99 coverage](#六c99-覆盖). The front end takes
C99 as written in real projects: jsmn, cJSON, miniz, stb, tinyexpr and the
c-testsuite corpus compile and run (`realprog`, `tools`, `corpus` gates), and
since 0.0.18 so does the kilo editor; processes (`fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `getcwd`, `sys/wait.h`) arrived in 0.0.19 on Linux and macOS. Lua is in the `realprog` passing baseline; SQLite still needs `pwd.h` (`sh ./unisacc.com tests/run.cx -- --smoke|--all`). The language-side gaps are complex
types and trigraphs, and the bundled C library is a documented subset;
all are itemised in [the limitations table](#七已知限制) below. unisacc is a
**model-driven compiler (fact tables → generators → VM programs)**。默认产品在通用
VM 上执行生成的转移/动作程序；默认阶段包另含 42 张构造阈值网，经典参考则有下文
所列的 18 张构造事实网，并查询由这些事实网穷举得到的 DENSE 答案表。两者都由
事实与有限决策表确定性构造，不是训练所得，不声称学习或泛化。**NN** 须按上下文
区分这两类构造网，不能作为整个编译器或七流流水线的同义词，也不能把默认产品
笼统描述为学习所得的神经网络推理。
The user supplies C99 and receives ELF, Mach-O or PE output; finite-domain
exactness of the model data is not a claim of complete C99 coverage.

## 二、使用手册(命令、选项、头文件、限制)

`unisacc.com` is a single file; a Unix shell runs it as a script that picks the
slice for the machine (Linux and macOS, x86-64 and arm64). Windows reads it as a
PE, but that slice does not work yet (see the status above). The C headers it
needs travel inside it.
0.0.47 开发版中，完成缓存条目的默认字节上限为 `1073741824`（1GiB），可通过 `UNISACC_E3CACHE_MAX_BYTES` 设置字节上限；达到上限按 mtime 淘汰，缓存异常不影响程序运行。

```bash
./unisacc.com hello.c [args]                # compile and RUN in memory (the default; not cc's a.out)
./unisacc.com hello.c -- --flag -x          # arguments that start with '-' go after --
./unisacc.com -run hello.c [args]          # compile and run in memory, nothing on disk
./unisacc.com hello.c -o hello              # write an executable for this machine
./unisacc.com hello.c -b osx/arm64 -o hello # write an executable for a target
./unisacc.com -O2 a.c b.c -b lnx/x86_64 -o prog
./unisacc.com -E file.c                     # preprocess only
./unisacc.com -c -b lnx/x86_64 -o hello.o   # ELF relocatable object (Linux targets only)
./unisacc.com -c -b lnx/x86_64 -funit -o unit.o # unit object for separate compilation
./unisacc.com hello.c -S --tapebin -o hello.tapebin # write the portable tape container
./unisacc.com -run hello.tapebin            # run its recorded program
./unisacc.com hello.tapebin -b osx/arm64 -o hello
```

- **Default mode is run:** `unisacc file.c [args]` with neither `-run` nor an output flag compiles and runs the program in memory (owner decision 2026-10-01; 0.0.17); this is deliberately unlike cc's silent `a.out`. Writing a file always takes `-o` (or `-b target`, `-S`, `-c`, `-E`). Releases up to 0.0.16 wrote `a.out` like cc.
- **Flags:** `-O0`/`-O`/`-O1`/`-O2`/`-O3`/`-Os` (`-O3` uses the `-O2` pass set; `-O` and `-Os` use `-O1`), `-o`, `-b os/arch`, `-run`, `-I`, `-D`,
  `-include`, `-E`, `-M`/`-MM`/`-MD`/`-MMD`/`-MF`/`-MT`/`-MQ`/`-MP` (gcc's `.d` bytes; `-M` lists what `-MM` lists, the bundled headers are not files), `-nostdinc`, `-ftrim-libc`/`-fno-trim-libc`, `--version`; `-Wall`,
  `-Wextra`, `-g`, `-std=c99` are accepted. A `#!` first line is skipped.
- **Headers:** the standard/compatibility headers (`assert
  ctype dirent errno float inttypes iso646 limits locale math memory signal stdarg stdbool
  stddef stdint stdio stdlib string time wchar`) plus, on Linux and macOS, POSIX
  ones implemented on kernel calls (`unistd fcntl poll termios sys/stat sys/ioctl
  sys/wait sys/time sys/socket sys/select netinet/in netinet/tcp arpa/inet netdb`), and `unisacc_ffi.h`.
  Every carried header compiles when included on its own (checked by `hosthdr`). Ordinary
  library calls use the bundled C implementations, compiled on demand;
  `-ftrim-libc` (**on by default since 0.0.11**; `-fno-trim-libc` keeps every library body; `-libneed` is a compatible alias) conservatively selects library bodies using identifier and dependency closures. On
  macOS, the explicit dl/libffi bridge can call system APIs. Since 0.0.19, `-run` on
  macOS also forwards an external function that has a prototype, no definition and
  no bundled body to the host libc (dlsym + libffi); variadic and struct-by-value
  calls are refused by name. Direct C `-run` and `-o` routes can emit host-forwarding
  stubs for Linux, macOS and Windows targets; symbols are resolved on the system
  executing the program, not on the build host. Object/archive linking and `-S`/`-c`
  do not use this route. A missing forwarded function is diagnosed at runtime
  with exit status 127. See [host interop](host-interop.md) for calling limits.
  System FILE/va_list and allocator families must
  not be mixed with the bundled implementations.
- **Several files, one program:** `unisacc a.c b.c ...` compiles all the units
  of a program in one invocation. Each unit is preprocessed on its own, and a
  later unit's file-scope `static` names are renamed, so two units may each
  keep their own `helper`.
- **尚非 `cc` 替代品：** 当前开发修复的裸 `-c` 写宿主对象；裸 `-S`、`-S -b T`
  （无 `.s` 输出）与 `-c --tapebin` 保留中间 tape。`-S -b lnx/ARCH -o FILE.s`
  asks for GNU assembly, which the model product does not write yet (`NOT_IMPLEMENTED
  object`, exit 2). `unisacc as` takes the subset its own back end defines (plus
  `.text`, `.data`, `.bss`, `.globl`, `.byte/.ascii/.zero`); other instructions, Intel
  syntax, inline `asm` and the tape text are refused by name. `-c -b lnx/ARCH` writes
  an ELF object: a whole-program object links with GNU `ld` on its own, while `cc`
  cannot link it (`_start` collides with the crt and the calling convention is not
  SysV or AAPCS64); 产品的 Mach-O/COFF 对象仅为携带 tape 的 unit 容器，
  原归档参考的机器码对象合同不能迁移到产品。Unit
  objects (`-funit`) are linked by `unisacc` itself, not by the system linker,
  because only unisacc chains every unit's initialisers. `-l`/`-L` are still
  accepted and ignored; pass archives by path. C11 features and GCC extensions (statement
  expressions, `_Generic`, empty structs, inline asm) are outside the subset.
- **Tape origin:** `.tapebin` records the target selected when C was compiled
  to tape. A different `-b` target is rejected unless `--force-origin` is
  given; forcing it does not undo target-specific preprocessing. See the
  [v1 format](tapebin-v1.md). `--tapebin` with `-S`/`-c` writes the
  container; the same file can be read by `-run` or `-b <target>`.


## 三、决策网与七流(经典参考实例,已冻结)

## How the decisions are made

> **Shell = model decoder + VM executor + model data + host bindings.**

The default model product compiles through a **seven-stream byte pipeline**
(below). The **classic seed and reference (decision-network instance)** is a
separate route: the walker, symbol table, relocation arithmetic and image
writers are ordinary code. Its fact nets use cube networks at ask points,
with one kernel for all of them (`embed -> gemv -> ReLU -> gemv -> argmax`,
integers only).

### Byte-stream pipeline (model-driven product)

The default model product runs compilation as **seven stream-stages**.
Each stream-stage runs a generated transition/action program on the generic
VM executor. Stream-stage boundaries are the I/O contract below (aligned with Paper A
§4.2 in [research/unisacc-paper.en.md](../research/unisacc-paper.en.md)). Tests
and model keys should stop at these edges; this is not the same table as the
eighteen classic decision nets later in this section.

| stream | input → output | role |
|---|---|---|
| preprocess | source bytes → preprocessed text | line continuation, comments, directives, macros, conditionals, includes |
| lex | preprocessed text → typed token stream | keywords, identifiers, operators, literals, longest match |
| parse | token stream → tape | declarations, scopes, types, precedence, statements, init, calls |
| opt | tape → tape | peephole / stack-to-reg style rewrites on tape |
| prune | tape → tape | drop unreachable whole functions on image/run routes (public tape out skips prune) |
| lowering | tape → target ops + data | ABI, regmap, syscalls, layout, reloc |
| encode + image | target ops → ELF / Mach-O / PE (or in-memory image) | encoding, branches, reloc, headers, Mach-O ad-hoc signature |

`tape` is the target-independent IR (fixed register set + op vocabulary). The
eighteen-net table below is the **decision-network / classic-reference**
instance (finite key → class at ask points). It does **not** replace the
seven-stream byte-pipeline contract above.

Each stage can be run on its own with `unisacc stream STAGE [--route R] [--source LABEL] [--raw] < in > out`
(`stream frame` and `stream sidecar` join and split the multi-unit and side-car forms). A chain such as
`e2 | e1 | e3 | sidecar | prune | lower | elf` gives the same bytes as `-o`. A stage that rejects exits 1 and
writes nothing, so the next stage sees an empty pipe and exits 1 with `no input`; in a shell pipeline the
first failure is only visible with `set -o pipefail` (or by checking every stage's status). The tape entry
check reads the first line (`_start:`) and the final newline only: a tape cut at a line boundary passes the
entry check and is caught downstream (measured on a stdio program: 10667 of 10670 line cuts are rejected by
`elf` as a branch to an undefined label; the other 3 keep only the first 1-3 lines and do not run).

### Decision nets (classic-reference instance)

<!-- stages:begin -->
| stage | key | out | keys | units |
|---|---|---|---:|---:|
| `pp` | dir(9) x defined(2) | 4 classes | 18 | 6 |
| `lex` | c(12) x peek(12) | 10 classes | 144 | 11 |
| `parse` | nt(5) x tok(68) | 36 classes | 340 | 34 |
| `type` | t1(15) x op(19) x t2(15) | 16 classes | 4,275 | 62 |
| `scope` | ctx(6) x kind(5) | 7 classes | 30 | 9 |
| `irsel` | family(6) x flavor(70) | 70 classes | 420 | 75 |
| `enc` | op(87) x os(3) x arch(2) | 5 classes | 522 | 5 |
| `reloc` | kind(3) x arch(2) | 3 classes | 6 | 3 |
| `regmap` | treg(8) x arch(2) | 16 classes | 16 | 10 |
| `tyinfo` | t(16) | 3 heads | 16 | 6 |
| `pfconv` | conv(9) | 7 classes | 9 | 7 |
| `peep` | a(8) x b(17) x rel(12) | 10 classes | 1,632 | 18 |
| `opinfo` | op(73) | 3 heads | 73 | 16 |
| `prec` | op(19) | 11 classes | 19 | 11 |
| `binsel` | op(16) x sign(2) | 23 classes | 32 | 18 |
| `isel` | op(87) x arch(2) | 2 heads | 174 | 88 |
| `abi` | op(87) x os(3) x arch(2) | 13 heads | 522 | 69 |
| `combo` | op(87) x os(3) x arch(2) | 15 heads | 522 | 121 |
| **total** | | | **8,770** | **569** |

*Frozen text from the historical Python doc generator and legacy doc check. Nothing regenerates or checks this table today.*
<!-- stages:end -->

The last two are a control arm: the lowering asks neither. In this historical
fact-net table, units count constructed hidden units, not trained parameters
or VM instructions. The reference runtime uses the enumerated DENSE answers;
the count is not a measure of NN inference in the default pipeline.

**Exactness is constructed, not searched.** For these fact networks, weights
are derived from the gold decision tables (`weights/gold/*.tsv`): `W1` in
{0,1}, `b1` recoverable from `W1`, `W2` small powers of two. Hidden activation
is always 0 or 1; evaluating that integer network requires no multiply or float.

**Verification is exhaustive, not statistical.** Each net is a total
function on a finite domain, so "the net equals the table" is decided by
enumerating every key (the total is in the table above). That proves the net
equals the *table*; whether the table is right about C is checked separately,
against the system compiler（`difftest` 为经典参考对照术语，不是当前产品开关；现行测试入口为 `sh ./unisacc.com tests/run.cx -- --smoke|--all`）。

## 四、旧测试套件表(0.0.43 前,已退役)

The current entry is `sh ./unisacc.com tests/run.cx -- --smoke|--all|--list`
(registry `tests/batches.tsv`, coverage list `tests/coverage.tsv`); `--smoke`
is local only, and `--all` fails while coverage is incomplete. The rest of
this section describes the historical pre-0.0.43 suites (now retired) and is kept
as background: in that old harness, each suite's verdict was its exit status, and
each got a watchdog so no suite could hang the run. **Testing happens on this
desk, not on GitHub**: macOS natively, Linux in a local Lima VM,
Windows 11 in a local UTM machine.
CI is a second opinion, not the test loop: the local suites run first, and
`.github/workflows/ci.yml` re-checks them on a machine nobody has been
editing. Training is **not** in it:
the weights we ship are constructed, and `unisa acc` verifies those by enumeration
in a twentieth of a second.

| suite | what it checks |
|---|---|
| `vm` | the tape interpreter, on hand-written fixtures |
| `difftest` | unisa vs the system compiler — the only instrument that can see a defect in the reference tables |
| `native` | emitted images actually executed on this host |
| `crossnative` | the *other* targets executed on real machines — local Linux VMs and the Windows 11 machine in UTM; the blind spot where three codegen bugs lived |
| `linux` | the whole suite again, inside the local Lima VM, because glibc and BSD libc disagree about things C only calls unspecified |
| `fat` | one file, both macOS architectures, both slices actually executed |
| `ccrun` | `unisacc` compiles C and the reference VM runs it |
| `stages` | every decision net the Python front end asks, the C front end asks too — the only suite that can see a hand-written rule that happens to agree with the table |
| `selfgap` | how much C `unisacc`'s own front end still refuses that the Python one accepts — a ratchet, because that gap was growing unmeasured |
| `multi` | two translation units compiled into one program, against `cc a.c b.c`.  Both ORDERS are run (`m1 m2` and `m2 m1`), because a file-scope `static` used to be registered when its definition was reached, so a header whose first function calls one defined below it spelled the call without its unit suffix and only the order that reached the definition first worked (`fwd1.c`/`fwd2.c`, and `static1.c`/`static2.c` with same-named statics that must NOT merge) |
| `bootstrap` | `B = C = U`, the self-hosting fixed point |
| `fb12-multi` | the multi-unit fixture directory suite, one case per real-world defect (23 miniz round trip, 27 signature pool, 28 multi-unit static declarator, 30 label naming, 34 arm64 fallback, 35/36 `__has_feature`, 22 multiple `-I`, 24 nested quoted include).  It runs in the gate's `--com` block only: each fixture needs an executable compiler, and the fixtures' expected values come from the host `cc` |
| `closure` | the image `unisacc -b` writes is byte-identical to the Python back end's, for every probe on all six targets (this is a check of the C back end against the seed, not a proof of C semantics) |
| `nativeboot` | `unisacc` builds itself and the result rebuilds itself to the same bytes — no Python anywhere |
| `ablate` | each net's answer is rotated to a wrong one: an image must change, or the compile must be refused — asking a net is not the same as obeying it |
| `run` | `unisacc -run` compiles a file and runs it in memory, against the system `cc` |
| `cli` | the compiler as a tool, from a scratch directory: built-in headers, `-I`, `-D`, shebang, exit status |
| `ccparity` | unisacc next to `cc -std=c99` as a user sees it: a.out, `-o`, exit status, stderr, files left after a failed compile, `-E`/`-D`/`-U`/`-I`, `-Werror`, and the documented `-c` difference |
| `layout` | the data layout **enumerated**, not sampled: every tape of up to three data definitions, each also with one symbol defined twice — 762 tapes, 4,572 image comparisons across all six targets, in twelve seconds |
| `datashape` | generated programs whose globals are declared in one order and allocated in another; the suite asserts the shape is present before it asserts the bytes match |
| `bigclosure` | the closure on the compiler itself — the largest input there is, and the only one ever big enough to break the back end while every probe stayed green |
| `corpus` | [c-testsuite](https://github.com/c-testsuite/c-testsuite) — 220 programs written by other people, for other compilers |
| `realprog` | real PROGRAMS by other people, pinned (kilo, jsmn, cJSON, Lua 5.4.7, SQLite 3.49.1), built by unisacc and by `cc` from the same sources and run the same way; bytes and exit status must agree (2 of 6 passed in 0.0.16, 3 of 6 in 0.0.17 with `unistd.h`, and the baseline names five: kilo, jsmn-simple, jsmn-dump, cjson and lua; SQLite still needs `pwd.h`) |
| `elfobj` | `-c -b lnx/ARCH` writes a relocatable ELF: sections, allowed relocation types, `.text`/`.data` equal to the image outside relocated fields, ld.lld links both architectures, GNU ld links and the arm64 programs run in the Lima VM and print what the image prints |
| `subtract-safety` | nothing in the live tree imports a module or names a path whose only copy is archived (`tools/check-archive-refs.cx`); `AGENTS.md`/`CLAUDE.md` resolve to a non-empty file without a link cycle |
| `tools` | real library code by other people — [crypto-algorithms](https://github.com/B-Con/crypto-algorithms), [tiny-AES-c](https://github.com/kokke/tiny-AES-c), [tiny-regex-c](https://github.com/kokke/tiny-regex-c); eleven entries, several files each, with their own known-answer tests |

## 五、历史语料与库观测

## Historical corpus and library observations

The results below describe earlier classic and reference measurements, retained
for the defects they exposed. They are not the current candidate's acceptance
snapshot (current corpus and library checks run through `sh ./unisacc.com tests/run.cx -- --smoke|--all`).

The historical `corpus` suite ran c-testsuite, 220 single-file C programs this project had
no hand in writing:

```
corpus 220   pass 214   wrong 0   unsupported 0   knownfail 6   slow 0
```

The programs are compiled to a real image for this host and **executed**, not
interpreted: the reference VM is a Python loop, and an eight-queens search a
real CPU finishes instantly takes a quarter of an hour there. `wrong` is the
only failure — a program that compiled and then disagreed.
`unsupported` is the honest coverage gap — the front end refuses the
program — and was **zero** in this snapshot, so the next one to appear was an alarm
rather than another entry in an old backlog. `knownfail` is the list we are
deliberately not chasing: floating point, GCC extensions, `_Generic`, and one
place where this subset evaluates `int` arithmetic at 64 bits on purpose.

But a corpus of single files written *for compilers* is not the same as
library code written *to be used*. The historical `tools` suite ran the other kind —
crypto-algorithms, tiny-AES-c and tiny-regex-c: eleven entries, every one of
them several files, with their own known-answer tests, none of it floating
point:

```
tools 11   pass 11   wrong 0   unsupported 0   skip 0
```

It read `pass 2` the first time. The ten defects behind that are the most
useful thing this project has measured in a while — **seven of them produced a
wrong answer rather than a crash**, and two were invisible to all six `--fold`
targets because the interpreter does not model addressing modes:

- comments were being removed by the *lexer*, not before the directives, so
  `#define N 32  // note` commented out the rest of every line that used N;
- macro parameters were substituted one at a time instead of simultaneously,
  so MD5's `FF(d,a,b,c,…)` rounds wrote the wrong variable;
- arm64's `[fp, #-off]` immediate is a *signed* 9-bit field and we were masking
  it, so every function with more than 256 bytes of frame read the wrong local;
- `uint8_t` was a typedef for signed `char`, so AES printed a screenful of `f`s;
- parentheses destroyed an lvalue, so `(*p)++` was refused outright;
- and a struct passed by value was copied through the registers the *later*
  arguments were still sitting in.

`prd.md` §6 E-45 and E-46 retain the full list. The lesson is not that real code
is harder — it is that real code leans on the type system, the ABI, the
encoder and the library *at the same time*, and a probe we write ourselves
leans on one of them at a time.

## 六、C99 覆盖

<!-- c99-ledger:begin -->
Clause coverage (historical c99 clause ledger, one row per normative subclause of
ISO/IEC 9899:1999 + TC1-TC3, WG14 N1256; headings and informative clauses are excluded):

| Part | Subclauses | Covered | Partial | Unsupported | Covered |
|---|---|---|---|---|---|
| Language (clause 6) | 98 | 95 | 1 | 2 | 96% |
| Environment (clause 5) | 16 | 13 | 2 | 1 | 81% |
| Library (clause 7) | 349 | 242 | 40 | 67 | 69% |
| Annexes F, G | 19 | 0 | 0 | 19 | 0% |
<!-- c99-ledger:end -->

This table is frozen historical text from the pre-0.0.43 clause ledger.
The current active coverage list is [tests/coverage.tsv](../tests/coverage.tsv).

## 七、已知限制

What this compiler does not do, so a reader does not have to find out by
experiment.  Every line was measured against the shipped `unisacc.com`, and the
reproduction is one short program.  The table exists because an external trial
of 0.0.12 reported time lost on exactly these, documented nowhere.

| Limitation | What happens | Reproduce |
|---|---|---|
| 22 classes of C99 diagnostic gaps (A: 12, B: 10) | Eight class-A cases were measured as accepted with wrong output: `duplicate-case`, `case-outside-switch`, `struct-redefinition`, `conflicting-typedef`, `conflicting-function-types`, `enumerator-redeclared`, `float-subscript`, `return-pointer-as-int`. Full list and evidence in [GitHub Releases](https://github.com/partnernetsoftware/unisacc/releases) and `tests/negative-domain-matrix.tsv`. Check constraints separately with `gcc -std=c99 -pedantic-errors` | see `tests/negative-domain-matrix.tsv` |
| `__LINE__` before a line continuation (C99 6.10.8) | A token that sits BEFORE a backslash-newline on the same line reads one line too high (`int m = __LINE__; int \\` then ` n = __LINE__;` gives `3 3` where gcc gives `2 3`); diagnostics at such a token are off by one the same way.  Tokens after the join, and every later line, are right | the two-line example |
| No complex types (C99 6.2.5, 6.3.1.6-7, 7.3, Annex G) | `double _Complex z;` is rejected; the 0.0.20 reference names it (`not covered: complex types`, with an `UNCOVERED` record), released compilers say `expected ';'` or `unknown identifier`; `<complex.h>` is not provided | `double _Complex z = 1.0;` |
| No trigraph replacement (C99 5.2.1.1) | `"a??=b"` stays `a??=b`; C99 requires `??=` to become `#` in translation phase 1 (gcc/clang also skip it unless `-trigraphs`) | `printf("%s", "??=")` |
| ~~No `__LINE__` / `__FILE__`~~ -- **supported as of 0.0.13** | Both work on the reference.  `__LINE__` is the PHYSICAL line of the token -- spliced headers and continuations notwithstanding -- and inside a macro body it is the line of the INVOCATION, which is what gcc and clang report (`9 1010 11 12 15 16`, identical to cc).  `__FILE__` is the path as given on the command line, and reaches the front end through the `\0cli/source` resource when the driver passes no SRCPATH.  This row is kept, struck through, rather than deleted because it was the external trial's most-reported omission | `printf("%d %s\n", __LINE__, __FILE__)` |
| `long double` is `double` | `sizeof(long double) == 8`; no extended precision | `printf("%d", (int)sizeof(long double))` |
| Private calling convention between generated code | Not SysV / AAPCS64: arguments are pushed on the tape stack left to right, `r9` is the frame pointer, the result is in `rax`, the caller pops.  **Unstable** -- it follows the regmap and abi tables and external code must not rely on it.  Interop with outside code goes through libunisacc's carriers, not through this convention.  [prd W-16](../prd.md) | `examples/apps/xgui.c` writes a raw syscall stub by hand and must save `r9`, `rcx` and `r11`, because `r9` doubles as syscall argument 6 and `syscall` clobbers `rcx`/`r11` |
| Windows product host calls remain a named boundary | `include/unisacc_ffi.h` has macOS, Linux and Windows loader branches. The reference Windows back end has conditional imports, but the current product route rejects `.hostaddr`/`.hostcall` by name until its Win64 bridge matches the reference; it must not emit a wrong image | a Windows program using `__hostcall` |
| Missing C library surface | No `regex.h` or `strings.h`; `setjmp.h` is present with compiler intrinsics for `setjmp`/`longjmp`. In 0.0.20 (development): `struct timespec`, `clock_gettime`, `nanosleep`, `sleep`, `usleep`, `execl`/`execlp`/`execle`. 当前候选源码将 macOS `clock_gettime` 绑定至 libSystem，提供真正的 `CLOCK_MONOTONIC`，常量由 `exec/facts/clock-targets.tsv` 生成；两种 Mac 架构的原生收据齐备前不宣称资格通过。Linux 保持系统调用路径，Windows 保持 ABI 适配 shim。 Since 0.0.19 on Linux and macOS: sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h` IPv4, `netdb.h` without DNS, `sys/select.h`), processes, `sys/wait.h`, the full errno set and the time functions. Since 0.0.18 `sys/stat.h`, `fcntl.h`, `poll.h`, `termios.h`, `sys/ioctl.h`, `isatty` and `dirent.h` on macOS exist on Linux and macOS (kernel calls, no host libc); Windows refuses them by name. A minimal `unistd.h` (fd calls) arrived in 0.0.17; `locale.h` and `sys/types.h` in 0.0.13. | `#include <regex.h>` |
| Identifiers: 63 significant characters | C99 6.4.2.1 requires that many, and both front ends keep them.  **The C reference refuses a 64-character name** with `error: identifier too long (max 63 characters)` at its position, rather than truncating it -- truncation merged two distinct names that shared a prefix, and miniz's `tinfl_`/`tdefl_` families are exactly that shape.  The **product accepts arbitrary lengths** (E3 interns names by source position), so a program the reference rejects still compiles on the product; the asymmetry is deliberate, not a bug in either | a 64-character identifier: the reference exits 1 with that message at the name, the product compiles it |
| Struct member count: 256 per struct | A limit separate from the table capacities below: one struct may hold at most 256 members.  Both front ends refuse a 257th, worded differently -- the reference prints `too many members` with no position, the product prints `error: not covered: structure member capacity` at the member | a struct with 257 members, then `s.m0 = 1` |
| Shared table capacities are not deduplicated | The typedef, struct-tag and member tables are shared across translation units and **not deduplicated**: every unit re-registers the headers it includes, so N units of the same headers consume N times the entries.  Counted rather than guessed -- one unit of the miniz headers contributes ~91 typedefs, and three units crossed the old limit of 256.  The limits are now typedefs 1024, struct tags 512, members 4096 | compile the same header from three translation units and watch the typedef count climb |
| Source files per invocation: 128 | One `unisacc.com` run accepts at most 128 source files (multi-unit frame cap `UNIT_MAX` in `exec/facts/limits.tsv`, checked by `tests/check-limits.cx` against every copy); a 129th is refused up front with `unisacc: model driver: too many sources (limit 128)` and no output. The earlier cap was 64 | `main.c` plus 128 one-function files |
| Headers and functions that are declared but absent | `regex.h`, `strings.h`, `utime.h`, `pwd.h`; `system`, `strdup`, `sigaction`. (`unistd.h`, `fcntl.h`, `sys/stat.h`, `getline` and `time()` exist since 0.0.18-0.0.19; `clock_gettime`, `nanosleep`, `sleep`/`usleep` and `execl`/`execlp`/`execle` and `sys/un.h` are in 0.0.20.)  A program that needs one gets a missing-header error, or an implicit declaration and then `undefined function`.  The 0.0.13 additions are `size_t`-style types, `strerror`, the stdio set (`feof`/`ferror`/`ungetc`/`clearerr`/`setvbuf`/`tmpfile`/`vprintf`) and `errno` codes.  The rest is the FX-5 layering decision: L2 (declare and import the host's) is 0.0.14 | `#include <pwd.h>` or a call to `strdup` |
| The private calling convention is not the host's | Code this compiler generates calls code it generated: arguments are pushed on the tape stack, `r9` is the frame pointer and the result is in `rax` (prd W-16).  It is explicitly unstable and external code must not depend on it; interop goes through the libunisacc carrier or the plan's bridge routes.  This is why `#21` above has nowhere to put the callee's address | any two units, or `nativeboot` |
| Unit objects from before 0.0.20 | The 0.0.20 linker refuses a `-funit` object without the `.unit 2` record (`link: object predates .unit 2`): older objects cannot say which global had an initialiser, so two initialised definitions of one name would be silently merged. It also refuses two initialisers for one object across units and an object only ever declared `extern` (`link: undefined reference to NAME`) | recompile the objects with the new compiler |
| `__FILE__` in a SPLICED header; `#line` in a header | The path reported is always the MAIN file's, and inside `#include`d text `__LINE__` is the line of the line as the user sees it in the header -- both matching gcc.  `#line N ["file"]` (C99 6.10.4) is supported in the main file as of 0.0.14; the operand may also be one object-like macro whose body is a digit sequence (c-testsuite 00152); inside an included header it is refused with its position (`not covered: this form of #line`), as are other macro operands and escapes in the file name | `#line 7` inside a header |

Two things that look like limitations and are not.  `-ftrim-libc` is on by
default and keeps library bodies only when a program reaches them by identifier
closure, so a symbol reached solely through a bundled header's macro expansion
can be reported undefined (pass `-fno-trim-libc` to keep every library body).  And the
model compiler refuses to compile rather than falling back to the classic path,
by design.

# Tables as Networks: A Byte-for-Byte Self-Hosting C Compiler Whose Weights Are Constructed, Not Trained

> English text aligned with [`unisacc-paper.md`](unisacc-paper.md). Source `910f1e0a`, `UNISACC_VERSION` 0.0.42.

**arXiv:** primary **cs.AI**; optional cross-list **cs.PL**.

## Abstract

unisacc is a neural-network-based compiler: compilation stages run on integer threshold networks and one generic executor; weights are constructed from a TSV table DSL, not trained. A finite decision is a total truth table. T1 is key-by-key equality on the declared domain (Definition 2), not sample fit. Two layers stay separate. The decision-network instance is 18 decision stages. The network-compiler instance is seven byte-stream stages (pp, lex, parse/parse2, opt, prune, lowering, enc). An out-of-domain key is rejected; there is no fallback. The decision-network instance meets N1 = N2 = N3 on five named targets (unsigned successive-generation bytes, not a semantic self-host). Key identities: op-74 totals 8,509 keys; op-87 totals 8,769 keys; they are not interchangeable before a seal. Source `910f1e0a` / `0.0.42` is not a sealed release measurement. Systematic training comparison is preregistered study A2. Enumeration proves network = table, not table = C.

**Keywords:** neural-network-based compiler; TSV table DSL; weight construction; T1; finite control; N1=N2=N3

## 1 Contributions

1. **Method.** TSV table DSL → constructed weights → exhaustion on the declared domain (T1). The weights are not trained.
2. **Two layers.** The network-compiler contract is seven byte-stream stages: one generic executor E runs finite transitions δ. The decision-network instance is a separate set of 18 decision stages, queried at ask points (or via the DENSE table enumerated from those networks).
3. **Implementation.** unisacc, a six-target C99 subset. The decision-network instance self-hosts byte-for-byte on five targets without Python. Source identity: Appendix A.

Not claimed: table = C99, whole-compiler semantic preservation (T3), or a speed advantage.

## 2 Definitions

**Definition 1 (decision stage).** Stage $s$ has fields $(f_1{:}V_1,\ldots,f_m{:}V_m)$, output vocabulary $Y$, and total $G_s:K_s\to Y$ on the finite product $K_s$. These 18 stages are the decision-network instance, not the seven byte-stream stages.

**Definition 2 (exact / T1).** $N$ is exact for $s$ iff for every $k\in K_s$, $\operatorname{argmax} N(k)$ is unique and equals $G_s(k)$.

**Definition 3 (query discipline).** Use only `ask(s,k)`. Do not read logits. Do not fall back to the table on rejection. $k\notin K_s$ fails.

$$
\mathrm{Compile}(x)=\mathrm{dec}(\mathrm{Run}_{E,\delta}(\mathrm{enc}(x))).
$$

δ takes the control state plus one finite observation and returns the next state and an action-sequence index. E performs storage, stack, and output only. A TSV row `state, observation, next, actions` expands to a finite total $G_s:(q,o)\mapsto(q',a^*)$; the constructor emits $N_s$; the check proves only $N_s=G_s$. DSL syntax and the T2/T3 obligations are in `research/paper-a-notes.md`. T2a, T2b, and T3 are not theorems of this paper.

## 3 Construction

A one-hidden-unit-per-key one-hot construction yields an exact network of width at most $|K|$ (Lean `naive_exact`). Shipped decision stages use a cube cover so the hidden layer is a Boolean conjunction and the output layer an integer vote (Algorithm 1; the 796,490→71,828 parameter count is in the notes). `decision_list_exact` covers only the globally monochrome first-hit case and does not prove the shipped networks. The release check is the integer network with strict argmax on the declared domain; a wrong answer or a tie rejects the build.

Keys of the 18 decision stages are `weights/gold` at this commit. The op-74 total (8,509 keys) and the op-87 total (8,769 keys) are two identities. combo is an isel∪abi control and is off the ship path.

The deployment kernel is integer: one-hot, integer affine, ReLU, integer affine, strict argmax. No softmax, floating point, multiplication, or libm. The same table constructs the same weights. This paper does not quote a weight-byte total.

## 4 Seven byte-stream stages and two instances

The network-compiler contract is the seven stages below. Each is a constructed integer threshold network run by one generic executor.

| Stage | Input → output |
|---|---|
| Preprocess (pp) | source bytes → preprocessed text |
| Lex (lex) | preprocessed text → token stream |
| Parse (parse/parse2) | token stream → tape |
| Opt (opt) | tape → tape |
| Prune (prune) | tape → tape (public tape output skips prune) |
| Lowering | tape → target instructions and data |
| Encode and image (enc) | target instructions → ELF / Mach-O / PE or a memory image |

The tape has eight 64-bit registers and 71 operations in `SHAPE`. The executor has no primitive for C, the tape, an ISA, or an image format.

**Decision-network instance.** Classic recursive descent, lowering, and image writing call `ask` at decision points; no fallback. **Network-compiler instance.** The executor runs the seven stages; a model rejection is an error. This tip has no same-identity size or speed measurement; this paper claims none.

Threshold-prefix evaluation and declared returns do not change network answers: same network, same sum; the network–table check still covers every observation, including returns. Host code still loads packages and retains `src/tapelink.c`, `src/asmtext.c`, and `src/fwdstub.c`. Build-time Python is in the trusted base.

## 5 Self-host, T1, and referees

Decision-network instance, byte equality:

1. Front-end fixed point: A compiles itself to B, B compiles itself to C, B = C, and both match the Python front end.
2. Back-end closure: 94 probes × 6 targets, 564/564; the compiler itself 6/6.
3. No Python: N1 = N2 = N3 on osx/arm64, lnx/arm64, lnx/x86-64, win/arm64, and win/x86-64 (the last under system emulation).

N1 = N2 = N3 asserts only unsigned successive-generation identity. The network compiler's driver self-host is N1 = N2 = N3 on macOS arm64 for a fixed network package. It does not construct that package.

T1: every cube network is run with deployment arithmetic on every key of its declared domain; an error or a tie fails construction. On the normal C path the decision-network instance reads DENSE and has a command that compares table lookup with direct inference.

Six-target fold: the machine model recognises only the (OS, call number) produced by lowering. The six (stdout, exit) pairs agree with each other and with a tape reference VM. Clearing the class bits of macOS system-call numbers must yield 4/6. This tip has no native-versus-emulation matrix; this paper does not fill one.

External referees cover only what they name. Eight of the 18 decision stages have a named referee. The ledger is `research/referee.tsv`. The table is a pointer, not a proof of C.

| Referee | Check |
|---|---|
| Preprocess | `exec/pp/macrocheck.py`, `tests/ccparity.sh`, against `cc -E -P` |
| Parse | `tests/warn.sh`, (line, kind) against `cc -Wall` |
| Type | `_Generic`; 1,400 of 4,275 keys |
| ABI | host system-call headers; actual host OS and architecture only |
| Precedence | 324 pairs; 291 distinguishable and consistent; 33 indistinguishable |
| Type info | sizes of 11 types; signedness and narrowness of 8 integer types |
| Binary-operator selection | `tests/binsel_audit.py`; 7 of 16 operators depend on signedness |
| printf | `tests/pfconv_audit.py`; 9 conversion letters |

Recorded decision-network acceptance checks (not a new seal of this tip): T1 is 1.000 on 18 stages; the type table matches cc on 1,399 of 1,400 keys, with 1 intentional difference; c-testsuite 216/220 pass, 0 errors, 4 out of scope; C99 probes 59/59; differential tests 93/93; three optimisation levels 279/279; two front ends 190/190; known-answer libraries 11/11; back-end closure 564/564 and the compiler 6/6; `%f/%e/%g` matches platform libc bit for bit.

## 6 Boundaries

Systematic construction-versus-training comparison is preregistered study A2. This paper has no training table.

The verification discipline has exposed cross-table combination gaps under one identity. Details are in `archive/research/r20/design-review-2026-10-01.md` and `research/paper-a-notes.md` and are not restated here. T1 asserts only network = table on one table's declared domain.

This tip has no same-identity census of the seven stages, no speed measurement, and no platform matrix. The corpus does not extend to arbitrary C99. Build-time Python is in the trusted base.

Related work, as pointers only: exact network encodings of finite-state machines (Omlin and Giles, 1996; Siegelmann and Sontag, 1995); weights constructed from programs (Weiss et al., 2021; Lindner et al., 2023); table-driven compilers (Johnson, 1975; Graham and Glanville, 1978; Fraser, Hanson, and Proebsting, 1992); trained controllers (Graves et al., 2014); proved semantic preservation (Leroy, 2009; Kumar et al., 2014); diverse double-compiling (Wheeler, 2009). Gradient-method failures are left to A2.

## 7 Conclusion

On table-shaped decisions, exactness is decidable by exhaustion. unisacc runs constructed weights on two layers: 18 decision stages, and seven byte-stream stages. The decision-network instance meets N1 = N2 = N3 on the five targets in §5. Whether a table equals C is answered only by the named referees, inside their coverage.

## Appendix A

| Identity | Content |
|---|---|
| Source tree | main `910f1e0a`; `src/version.h` is `0.0.42`. Not public Latest and not a sealed release measurement. |

Pointers: `research/referee.tsv`, `research/formalization-roadmap.md`, `research/lean/`, `prior-art.md`, `research/paper-a-notes.md`.

```bash
python3 -m unisa build-weights
python3 -m unisa acc
python3 -m unisa run examples/hello.c --fold
./tests/closure.sh examples/*.c tests/c/*.c
./tests/nativeboot.sh
./tests/gate.sh --list --com
```

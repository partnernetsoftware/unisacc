# 2b lower model receipt — actual model tests only

Actual named base gate: `MODEL_COM=/tmp/c43-forward-paths/final/unisacc-next.com ./tests/gate.sh --suite exec-libraryfn-model`, rc0, 2s (warm preparation). Old product is only the dynamic-C observer; no fresh product claim. C/Python constructors compare byte-identical before tbl/net construction.

13 frozen fixtures, both tbl and net actual execution: exact reference fragment, full/fragment span framing, alternate cwd/env replay. The two-label (formerly rc0/264B), one-label and jump-target cases now rc1/empty/named provenance; duplicate call also refuses. Old typed alone and new-then-typed complete output match the frozen pre-fix baselines. Model: 3530 states, 2070 hidden units, 393181 net bytes.

ABI derived from clean HEAD export `/tmp/c43-libraryfn-head-twyo_v2l` plus only the paths named in this commit, by dynamic C `abimanifestcheck.cx --root ... --write .../tests/abi.manifest`: 411 entries. Unfinished library adapter, provider header and packaging edits excluded. This recipe avoids shared-workspace contamination.

Not proved: fresh library API A/B providers, other target execution, old data/typed complete suite qualification, no-address whole lower stream, signature matching, gate result reuse. Those belong to adapter fresh validation; this commit neither removes dependencies nor reopens any historical gate.

## Source/data digests

- `exec/lower/libraryfn-byte.tsv`: 147 bytes, sha256 `3c6dc239a34a2f0e181f2da1ad8754da62e5c1c44da21211ee044cb522c52a6c`
- `exec/lower/libraryfn-result.tsv`: 47922 bytes, sha256 `abfc21dd8cdcea55324c967dcd3ae35b562718a828e81fdb9fe0a7847f8e5cf6`
- `exec/lower/libraryfn-manifest.tsv`: 437 bytes, sha256 `61c57d268c73bfcec6a3f7208ad913c1830e9903e826afc1eb4dc92b31ca333e`
- `exec/lower/libraryfn-template.tsv`: 293 bytes, sha256 `26704a22513b6290844cfb40842480339bf723d18f24fb50ac67694439489c53`
- `exec/lower/libraryfn-observer.c`: 752 bytes, sha256 `a834bb1cfc654f9cad942b5b93a0e48db53d31599ef6cd49feb217580ecc52ee`
- `tests/libraryfncheck.cx`: 3752 bytes, sha256 `0a84552d95a3bd69f24381f6472c307afe71963912413c64cd6b284ac54f16e7`
- `tests/libraryfn-cases.tsv`: 10265 bytes, sha256 `c5da036541c1a990de64bee1baec2998ce71db70088efedf579f74f3c636e1d6`
- `unisa/libraryfnlower.py`: 4312 bytes, sha256 `4d68781769abf2d56f24e561cb6249f5b1ea6c3d443568009b3dfe9da123182a`

Standalone post-fix net SHA256 `18de18f7cc26f4d702612256e107e6757935901628565bbc7ed823a4e89b6636`; actual named suite executes both freshly keyed model formats.

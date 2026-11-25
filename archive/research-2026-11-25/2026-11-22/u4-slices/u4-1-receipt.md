# U4.1 E1 交付

版本分支基线 729c88bae01324ecda324099d5ee7603a2db13e3。私有 `u4-1.patch` 的 SHA256 为 f686297f14618e889ece6916a7ba6956a76e87b8254ccffc9cd6761c9b57f37a。五条均为 A 类，预留路径是 `exec/lex/gen-manifest.tsv`。

| 路径 | 增 | 删 |
| --- | ---: | ---: |
| weights/gold/parse.tsv | 1 | 1 |
| weights/gold/lexword.tsv | 3 | 2 |
| exec/lex/output-manifest.tsv | 4 | 4 |
| exec/lex/f1-template.tsv | 4 | 1 |
| exec/lex/gen-manifest.tsv | 1 | 0 |

先用四表实跑 typed/locations 生成 rc0；随后把左括号覆盖移入 F1 的共用段，在第五表只追加一条共用覆盖安装行。两种拼写在三个模式都输出 `__asm__`，该记号追加为从零计的码位 68，原 68 项逐位相等。实际位置帧上的两个偏移与源字节位置一致。普通 `asm` 标识符、前缀相近的标识符、注释和字符串的带位置输出前后逐字节一致。两种拼写跟 volatile 仍在 ASM.lp 拒绝。

`u4-1/out/four-facts.log` 和最终实跑 `facts-gen --check . lex-gen` 为 SAME，派生表仅在私有副本用生成器重算。真实 `seed/gen.c` 主机版执行正式 E1 配方 `lex`、`lex --locations` 及 supplemental `lex --typed` 三个 rc0，见 `u4-1/generate-final/events.tsv`。旧 plain 两个标签记号数为 0，新为 2，撤去新增安装行的编译变异回到 0，位置与码位共八条全过（`u4-1/verify.log`）。

严格 table-lint 前后均为 rc1。plain 都只报 NT 可能先读警告，typed/locations 都报原有 CF.finish 的 237 个边缺失及三条警告；NT 报告涉及边随两个新增 token 路由增加 2。原行 `u4-1/{token-probes,control-probes}/out/`。未重建产品，ELF 函数发射、裸标签语义拒绝、全链 8 例与 inline 诊断定位由后续片用统一候选验收。

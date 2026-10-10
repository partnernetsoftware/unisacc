# WF2 只读预测（cc，不起窗）
方法：拿 a2 窗后的 history（b28d6b89）里的 ok/lb，做简化排队模拟：span 48.3，4 槽，estimate 公式照现行代码；不模拟 exclusive、heavy、内存闸，也不模拟 entry 的 id/stamp 校验。
- 对照 a2（8 项）：模拟预测会有 4 次 tail；实际是 0 次 tail，前 4 个准入的都是秒级短套件（attemptchain、exittable、gqalive、k2b）。推测原因：本窗 entry 的 id 与 stamp 不匹配，estimate 退回冷先验，大家估计相同，排序就退化了。**模拟和实际不符，所以本预测不可靠**；stamp 我没有复算，记为 UNKNOWN。
- 候选 a3 = a2 的 8 项 + tape-reader、revivedscan、bound、front-bounds、namedrefuse、qprefix（共 14 项）。静态扫描这 6 个脚本，都不含 queue.sh/gatequeue.py/release.sh/QUEUE_STATE/GATE_STATE。按 history 耗时，有 ≥8 项要 5–17 s，多于 4 个槽。所以即使排序退化，3 s 之后仍有套件等着准入，~~结构上会出现 tail~~ **【已被下文更正段替代：只能说“可能，未证”；本段模拟已作废】**
- 排除：gate-infra（queuecheck.py）、script-inventory（inventory.py）静态命中 queue/state 关键词；fresh-order-* 的命令没解析出来，不选。
- 不压缩时限、不放宽预算。如果仍然 0 tail，就照记“未覆盖”。

## 更正（cdx 22:2x）
- “结构上会出现 tail”改为“可能，未证”。模拟没有核 stamp、内存闸和 exclusive，而且 a2 的实际结果已经反证了它。
- 六项不全是轻量的，路线和资源逐项写明：
  - tape-reader：冷 build_ref，加 C harness。
  - front-bounds：build_ref，带 bounds sanitizer。
  - revivedscan：Python，**执行 ROOT/unisacc.com 产品探针**（不是 com 分片，但仍然是产品执行）。建议剔除。
  - bound、namedrefuse、qprefix：未逐项展开，记为待核。
- 建议的 a3 选集：剔除 revivedscan，共 13 项。具名路线和资源交授权人确认。

## cdx2 独核补记（22:2x）
- a2 窗前历史与 state.stamp 对比，8/8 失配；窗后 8/8 有效。所以 a2 开窗时 8 项都用冷先验、估计相同，前几个准入全是 full，这能解释。stamp 为什么变了，没有分清来源，记为 UNKNOWN。
- 我那次模拟用的是窗后历史，也没有校验 entry，属于用了“未来信息”，不能拿来模拟 a2 原窗。上面的模拟作废，不作依据。
- a3 起窗前应先核两件事：当前 stamp 下每项是否有有效历史；14 项多于 4 个槽，也不保证会出现 tail，因为内存、前驱、冷准入都会约束。
- qprefix 带 MODEL_COM，会读产品工件。它是否在获批的 contract 范围内、读了哪些文件，待核。在核清之前，建议 a3 也剔除 qprefix → 12 项。

## 剩余三项的路线（cdx 只读展开，22:3x）
- bound：可能要用宿主 cc 冷构造 native helper；还涉及 Python 和 native 计时器、信号、后台进程清理，以及 Terminal mock。不是纯离线。
- namedrefuse：用假 UA 驱动，但会跑 difftest_o 和宿主 cc 参考探针，并包含多种异常和信号负例；不跑产品。
- qprefix：用宿主 cc -O2 编译两个 run.c（解码 loader 和 runner），读 MODEL_COM 包，逐模型对拍；不执行候选 .com，但会运行 C 解码器，默认不开 sanitize。
供授权人逐项取舍，仍然不跑。

## 待授权人选定的最终清单（选一个，cc 不自行择一）
- 方案 A（13 项）：tools-1 tools-2 tools-3 k2b attemptchain exittable stagelog gqalive tape-reader bound front-bounds namedrefuse qprefix
- 方案 B（12 项，去 qprefix）：tools-1 tools-2 tools-3 k2b attemptchain exittable stagelog gqalive tape-reader bound front-bounds namedrefuse
两个方案都不含 revivedscan。本文件只是计划更正，不代表 a3 已获准，也不代表填尾通过。

# COV1 补覆盖设计便条（R8：机房主任经政委授权裁“补覆盖”；cc 诊断，2026-10-10，只读未改产品）

对象：tests/c/fb12-31-unused-static-refs-undefined.c——未被调用的 `static inline wrapper` 调用从未定义的 `ext_never_defined`。

| 路线 | 行为 |
|---|---|
| gcc / clang | 不发射未用 static，链接通过，打印 `ok` |
| 参考 UA（src/） | tape 中保留 `wrapper:` 块与 `call ext_never_defined`（/tmp/cc39-wf1/fb31.s 第 10176/10189 行）；src/front_parse.c:6556–6740 以 main、__init、非 static 函数为根做逐块可达性，不可达块里的 call 不算引用（:7048），不报未定义；运行 rc0 `ok` |
| 产品 unisacc.com | 三级 `reject: not covered: a branch to an undefined label`——exec/enc/gen-manifest.tsv:105 `x86-procs relax undefined=@rej:…` |

线索：src/tapeprune.c 首行“Conservative original-span reference; the product route uses prune delta”；exec/prune/（check.py、prune-*.tsv）与 unisa/prune.py 标为实验性、默认关闭。

候选方向（均改产品闭包，须 §24 生成器全矩阵预验与各阶段对参考逐字节对拍；未开工）：
1. 产品管线在 enc 前启用/扩展 prune δ，按与参考相同的根集合删去不可达 static 块，使 enc 不见悬空 call。需核：参考 tape 保留该块，而产品删除后各阶段对拍与 image 字节是否仍被允许（对拍口径须先定）。
2. enc 表对“不可达块内的未定义目标”按参考编码器的实际处理编码（需先查参考 back_encode/back_image 对未定义 call 的字节），不删块，保持与参考 tape/镜像一致。

验收：fb12-31 在 com-difftest_o 三级与 cc 一致 → tests/difftest.com.refuse 与 knownfail 行因 REVIVED 失败而删除（namedrefusecheck 已证该机制）；其余分片无新红；§24 矩阵全绿。

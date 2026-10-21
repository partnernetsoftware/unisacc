# 边界 catalogue：待裁决策单（r1）

状态：**待裁，未实施**。本文只写事实、选项与建议，不改产品、不再生 pp-autoinc、不 reseal pp-literals。依据：政委/机房主任裁定（10:11 AWST；(a) 否决，一口 reseal 否决）；cdx 设计底稿 `/home/box/c43/cdx-design-review-0849/REVIEW.md`；边界原型结果 `/home/box/c43/0043-boundary-r1/RESULT.md`；解耦设计 `/home/box/c43/0043-decouple/RESULT.md`。

## 1. 已测事实（measured）

- **耦合来源**：`exec/facts/export.py` 的 `ppautoinc()` 把头内 `static` 体名经 `libneed` 转成 `=keys/=bodies/=roots` 槽位，槽位 = NEEDB + 排序序号。头内插入/移动一个体，后续全部槽位右移。
- **隐藏输入**：`pp-autoinc-gen.tsv` 的 `# input` 行不列 `include/*.h`，但 `libneed.table` 与 `autoinc_map` 实际读取每个头。即头文本是未声明的输入。
- **边界原型（labs）**：把 `labs` 的体移到 provider、声明留头：`pp-autoinc-gen.tsv` 变（keys 446→445，bodies 444→443，217 槽位移位），`k2-gen2.tsv` autonames 变；P 类（pp-layout/pp-gen/pp-standard/pp-autoinc/autoinc-manifest）除 autoinc 外不变；H 类（stdlib.h 字节）变，致 2/93 依赖行失配。结论：**P/M 不变条件不成立**。
- **不可解耦的部分**：7 个封存头（stdio/stdlib/errno/limits/stdarg/stddef/sys/_exit）字节出现在 case-37/71 的 `-E` 输出中，头改动必然使封件失配（与 autoinc 无关）。
- **转发路径**：int/pointer/double 转发已在 Linux 实测可用（b7b87e9e）；转发声明不进 autoinc（已测 closure 不变）。这是当前唯一不触 pp 的库面扩展方式。

## 2. 选项

- **(A) 显式版本化名单目录**：autoinc 的名字来源从"扫描头体"改为"显式目录输入（版本化 catalogue 文件）"，槽位按目录序号而非扫描序号。头体插入不再改槽位。代价：一次性 autoinc 表/清单迁移，需 P 类证据（decision 表不变，只换输入来源）。风险中。
- **(B) 仅转发（当前已在用）**：库面只允许 prototype-only 转发，不再加头内体。代价：`llround` 等 double/long-long 之外的语义（如精确舍入）依赖宿主；不解决已有头内体的耦合，只是不新增。风险低，已验证。
- **(C) 维持现状**：每个头体仍触发 pp 重封。代价：每刀 reseal，违背裁定。不建议。
- **(D) 独立静态运行库（cosmopolitan/tinycc 式）**：库体不进头，进独立对象/运行库，经绑定层选 provider。代价：需要绑定与运行库合同与新产物路径，超出本拍。

## 3. 建议

**采 (B) 为即刻策略，(A) 作为下一个设计刀（需裁）。** 理由：(B) 已实测、不触 pp；(A) 是唯一能在保留头内体的同时解耦槽位的方案，但它需要动 autoinc 的输入来源，属于前级构造路径，必须有独立 P 类证据后才能动，故不由我即刻实施。

## 4. 需要裁决的具体问题

1. 是否授权 (A) 作为下一设计刀（动 `export.py` 的 autoinc 输入来源，不动 decision 表）。
2. 若授权 (A)：谁对"catalogue 版本化"的兼容期负责（旧封件是否允许一次性失配并一次性 reseal，或要求先证明 P 类不变）。
3. 封存头（stdio/stdlib 等）的字节依赖：是否接受"封存头改动 = 一次性 reseal（另立一次边界拆分合同）"，还是先冻结封存头库面（如 `_Exit` 保持 pending）。
4. 有限 POSIX C99 面的冻结口径：采用 `research/c99-surface-freeze-r1.md`（`79d5c67d`）作为冻结底稿是否可接受，之后新增库面只走 (B)。

## 5. 不做的事（本拍）

不改 `export.py`，不再生 `pp-autoinc-gen.tsv`，不 reseal `exec-pp-literals`，不动 `stdlib.h`/`stdio.h` 等封存头，不做 `_Exit`，不做 `long double`/`FILE` 转发。

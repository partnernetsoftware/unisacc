# W5 受影响批次清单复核（只读，2026-11-02 omp-ds）

冻结期只读复核：`plans/w5-capacity-2026-11-02.md` 第 90 行只写了"候选 + 受影响批次集"，没有落清单。
本文件补清单并说明为什么不能直接拿 `tools/affected.cx` 的输出当清单。

## 一、affected.cx 会给出"假完整"（scanned=355，只回 3 条）

```
sh ./unisacc.com tools/affected.cx --files cx/cx0/bundle.cx exec/lower/codeend-manifest.tsv \
  exec/parse2/units-result.tsv exec/pp/include-location-result.tsv exec/pp/macro-byte.tsv
AFFECTED	cx0-bundle	why=cx/cx0/bundle.cx	via=use
AFFECTED	cx0-e2	why=cx/cx0/bundle.cx	via=use
AFFECTED	cx-layers	why=cx/cx0/bundle.cx	via=literal
AFFECTED_SUMMARY	changed=5	batches=3	scanned=355
```

它只按 `tests/batches.tsv` 的 `source` 列匹配 `.cx`：**四个事实表和一个 `.tsv` 一个都没映射**，两份文档也没有。
所以 W5 的受影响面不能用这 3 条代表；事实表改动会让模型/内核/路由整体换身份，凡依赖产品编译的批次都算受影响。

## 二、三个补丁的实际文件与必跑批次

| 补丁 | 文件 | affected.cx | 必须补跑的批次 |
|---|---|---|---|
| 01 | `exec/lower/codeend-manifest.tsv`（`rows code-shell exit`：`facts -→lower-code`、`bind -→TXT=TXT`） | 无 | `stream-lower-independent`、`stream-lower-domain-format`、`-layout-plain`、`-layout-redef`、`-refusals`、`-input`、`stream-lower-target-lnx-arm64`、`-osx-x86_64`、`-osx-arm64`、`-win-x86_64`、`-win-arm64`（11 行，全部） |
| 02 | `exec/parse2/units-result.tsv`（64→128）、`cx/cx0/bundle.cx`（`:34/:71/:85`） | `cx0-bundle`、`cx0-e2`、`cx-layers` | 另加 `cx-unit-graph`、`cx0-unit`、`cx-funit-host`、`objects-old-unit-version`，以及 `objects-*` 全 9 行（`objects-link`、`objects-mixed-target`、`-missing-definition`、`-duplicate-definition`、`-old-unit-version`、`-unsupported-osx-x86_64`、`-unsupported-osx-arm64`、`-unsupported-win-x86_64`、`-unsupported-win-arm64`） |
| 02（文档） | `exec/pipeline/formats.md:259`、`exec/c/PACKAGE.md:252`（写死 64 的地方） | 无 | 相关批次：`formats-objects`、`formats-elf-ape`、`formats-macho-pe`、`formats-archive-ar`、`formats-object-ld`、`interfaces-lnx-x86_64`、`stream-e2-domain-positive`、`stream-e2-domain-refusal`。**更正（2026-11-02 二次核实）**：这些程序只在 `/* */` 注释里点名两份文档（`tests/formats-elf-ape.cx:15`、`tests/interfaces-lnx-x86_64.cx:15`、`tests/stream-e2-domain.cx:37`、`tests/stream-lower-domain.cx:17` 等），**不读文件内容**；所以手册 64→128 的改动不会让它们红。列这 8 行是回归（上限 128 后行为没变），不是文档一致性强制。 |
| 03 | `exec/pp/macro-byte.tsv`、`exec/pp/include-location-result.tsv` | 无 | `pp-negative`（现 8/10 红；补丁意图是 `#define 3x 1` 与未闭合 `#include <stdio.h` 两例转具名拒绝 → 期望 10/10） |

## 三、纪元级必跑（不属于任何补丁映射，但不能漏）

1. **完整新清单**：事实表→facts-gen→p2ini→kernel 换身份后，`tests/run.cx -- --all` 是新候选的发布资格口径；子集只算开发验证。
2. **九棘轮**：`tools/ratchets.cx`。预期 `cxapis` 319 calls / `coreapi` 26 模块 193 导出不变（`bundle.cx` 只改常量）；`cxinclude` 的 units 计数随 `units-result.tsv` 的 64→128 **可能变**，变了要在提交说明单列，不能当"不变"。
3. **门禁对照**（文档已写，这里提醒判据）：65 单元整程序 `-o` `rc1 reject malformed unit frame` → `rc0`；129 单元仍 `rc1` 具名拒绝；osx/win whole-o 三片 67 单元 `rc1`→`rc0`。这三条要在**同一候选**上一次性取证。
4. **落尾/磁带回归**：`tools/tape-fallthrough-cases.cx`（7/7）与 `tools/check-run-tape.cx`、`check-tapebin`（`BYTE-IDENTICAL`）——W5 改的是单元帧与上限，磁带/落尾路径必须证明没被带翻。

## 四、给 W5 文档的落点

本文件可作 `plans/w5-capacity-2026-11-02.md` 第 90 行"受影响批次集"的附节引用；该文件此刻有 bdy 的 staged 大改，
复核行不直接写进去，合并时引本文件即可。

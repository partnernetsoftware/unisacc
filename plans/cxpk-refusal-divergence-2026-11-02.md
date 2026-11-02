# cxpk_decode 与产品拒绝原因分歧：调查结论（2026-11-02，只读）

来源：6d 窗口前一次 census 的 `cx-package-refusals` 红项
（`out/census-83343ec1/log/cx-package-refusals.log`）：

```
FAIL version cx=missing model number live=0 c_rc=2 c_err=run: unknown package version
```

## 结论

**不是产品缺陷，也不存在「库层 vs 产品」分歧。** 是 `1bf6f7e8c3`（SIZE-01，P4 资源压缩）把包版本上限从 3 提到 4 之后，两条 10 月的断言没有跟着更新。

分歧双方其实是**当前树里的库层**与**归档的参考 C**（`archive/reference-c`），不是已装产品：

| 路径 | 位置 | 对 `P 4 1 1\n` 的反应 | 时间 |
|---|---|---|---|
| 新链库层 `cxpk_decode` | `cx/runtime/package_decode.cx:96` `if(ver<1\|\|ver>4)` | 接受 ver=4 → 按 ver≥2 读第 4 个整数 → 输入到结尾 → `missing model number`（`cxmn_number` rc=7，`package_decode.cx:30`） | 11-02 `1bf6f7e8c3` 改 `>3`→`>4` |
| 归档参考 C | `archive/reference-c/exec/c/run.c:665` `version != 1 && != 2 && != 3` | `unknown package version` | 冻结，未随 P4 更新 |
| 结构审计器 `packagestruct` | `cx/runtime/packagestruct.cx:88` `'1'..'4'`、`:90` 字段数 4/5 | 返回 2（字段数不足），**按 ver=4 受理** | 11-02 同提交 |
| 当前树重建的产品 | `src/packmodel.cx:135` 直接用 `cxpk_decode` | 与库层一致 | — |

census 那一条里 `live=0` 是**内存分配计数**（`static int live;` 由 `grow`/`drop` 维护，`ok` 要求无泄漏），不是「实跑产品」标志；该批 args 为 `-`，`argc==2`，根本没跑 live 产品。`c_err` 来自 gcc 编出的参考 C 二进制。我此前把 `c_err` 当成了产品输出，此处更正。

## 哪边对

库层对。`1bf6f7e8c3` 的提交说明写明「version accepted up to 4 everywhere」，并同步改了 `packagestruct`、`rebuildcontext`、`streampack`、`package-refresh`、`release-build`；`plans/v0.0.44.md` 的裁定 2 也是「读方同时接受 3 和 4」。参考 C 在 `archive/reference-c`，按规则不因新链演进改写，所以它的拒绝是**预期行为**，不是待修 bug。

## 期望值在哪、应怎么改（未实施，留给 cc 派工）

1. `tools/check-package-decode-refusals.cx:27`
   `{"version","P 4 1 1\n","unknown package version"}` —— 输入本身在 P4 之后不再非法。该表逐条比较「库层 why」与「参考 C stderr」，参考 C 只能拒绝到 3，所以这条永远不可能同时成立。
   建议：输入改为 `P 5 1 1\n`（明天仍非法；库层 ver>4、参考 C 都报 `unknown package version`）。`P 0 1 1\n` 亦可，但方向性差。
2. `tools/check-packagestruct.cx:54`
   `audit("P 4 1 1\nD route stage in out 0\nM 1\nZ", 2, 0, "version-rejected")` —— 现在仍返回 2（因为 ver=4 要求 5 个头部字段而只给了 4 个），断言**碰巧通过但标签是假的**：它现在拒绝的理由是字段数，不是版本。建议改标签（如 `short-header`），或换成 `P 5 …` 保持「版本拒绝」语义。
3. 附带缺口：`1bf6f7e8c3` 改了 12 个文件、**没动任何测试**（仅 `tests/srcshape.tsv` 计数）。全仓 `grep 'P 4 '` 只命中上面两条拒绝断言和 `release-build.cx`，**P4 受理没有正向覆盖**。同理 P4 写侧（`--compress-resources`）也没有定值用例。建议单开一条覆盖项，而不是只修断言。

## 复现（只读，不需要重跑）

- 证据日志：`out/census-83343ec1/log/cx-package-refusals.log`
- 源码：`cx/runtime/package_decode.cx:96`、`:30`；`archive/reference-c/exec/c/run.c:665`；`cx/runtime/packagestruct.cx:88`；`src/packmodel.cx:135`
- 历史：`git log -1 -S'ver<1||ver>4' -- cx/runtime/package_decode.cx` → `1bf6f7e8c3`（11-02 02:18，仅改 `cx/runtime/package_decode.cx` 等 12 个实现文件）；断言行 `git log -1 -S'{"version","P 4 1 1\n"' -- tools/check-package-decode-refusals.cx` → `765e8a4192`（10-29）

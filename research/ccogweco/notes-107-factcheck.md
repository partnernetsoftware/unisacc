# cdx 的 notes-0.0.107-draft.md 事实核对(只读)

对象:~/.local/share/unisacc/research/cdx/notes-0.0.107-draft.md(87 行)。核对基线:工作树 .claude/worktrees/v0.0.107 头 d750ad123c 及其历史;各页与日志在仓内或 cdx 研究目录。命令均为只读。

## 1 逐条核对(提交号/批次数/实测原行)

| 草稿位置 | 草稿说法 | 核对命令与结果 | 判定 |
|---|---|---|---|
| 补丁清单 | CXM_MAXU 前置 = 9199f8dbbf | `git log -1 --format=%s 9199f8dbbf` = "v0.0.107 前置: 合入 CXM_MAXU 256 补丁 (38ea37666a)" | 正确 |
| 补丁清单 | F1 与负控覆盖、B1 2/3 与 3/3 完整重放 = ac483247c7 | 该提交存在且信息相符;`git show --stat ac483247c7` 含 cx/compiler/fwdstub.cx、fwdmark.cx、fwdincrement.cx、routepipe.cx、tests/check-fwd-cap.cx、tests/check-tentative-array-incomplete.cx、tests/check-array-param-typedef.cx、exec/parse2/control-result.tsv | 正确 |
| 补丁清单 | module-units 三片与断言计数 = a7b2dd86c5 | 提交存在且信息相符;`grep "^module-units" tests/batches.tsv` = module-units、module-units-forward、module-units-limits 三行 | 正确 |
| 补丁清单 | cxmod-tri 两片与断言计数 = 351be964ef | 提交存在且信息相符;批表两行 cxmod-tri 与 cxmod-tri-exports | 正确 |
| 补丁清单 | 不动点证据工具 v4 = d750ad123c | 提交存在;`git ls-tree -r d750ad123c` 含 tools/fixedpoint-evidence.cx、tools/check-fixedpoint-evidence.cx、tests/manifests/fixedpoint-evidence.tsv | 正确 |
| 产品改动 (1) | 具名拒绝文本含 `forward stub buffer exceeded (forwarded symbols …, limit bytes 131072)` | cx/compiler/fwdstub.cx:58-68 的 fwd_refusal 逐字拼接;FWD_STUB_BYTES=131072 | 正确(草稿省略号是占位,原文 N 为数字) |
| 产品改动 (1) | 数字、标记和桩写入都走边界检查,恰好装满允许继续 | fwdstub.cx:30-47:fwd_n 对 n > FWD_STUB_BYTES - nfwdsrc 才拒;fwd_s/fwd_d 走 fwd_n;注释"All or nothing" | 正确 |
| 产品改动 (2) | t10 对照只比较标准输出 | tests/check-array-param-typedef.cx:110 只核 result.out == "8\n",不读 err | 正确 |
| 产品改动 (2) | 既有 extern 夹具保留原断言数并迁移 AN1/AN4 预期 | G45 审阅原行:A0_SUMMARY total=14 不变,AN1/AN4 expect_rc0 由 0 改 1 | 正确 |
| 已有实测 | F1 私有原版夹具 22 项过;移除 -5 返回 4 项红;跳过调用方分支 1 项红 | cdx/f1-minus5/unit-green.log 22 行 PASS;mut-fk.log failed=4;mut-route.log failed=1;两个 patch 的 diff 正是 remove `if (fwd_overflowed()) return -5;` 与 `if (k == -5)` 改 -6 | 正确 |
| 已有实测 | B1 旧产品:暂定数组 2 过 8 红、数组形参 5 过 5 红、extern 12 过 2 红 | G45 审阅原行 TENTARR passed=2 failed=8、ARRPARAM 5/5、A0 12/2 | 正确 |
| 已有实测 | 不动点证据工具 v4 真实提交门 PRECOMMIT staged_cx=2 blocked=0 | cdx/fixedpoint-evidence-tool-logs 内 PRECOMMIT 原行 | 正确 |
| 已有实测 | 封版准入工具私有夹具 5 过 0 红;移除开放 P0 判据后 1 项红 | cdx/seal-fixture.log CHECK_SEAL_PRECHECK ok=5 failed=0;seal-mutant.log ok=4 failed=1 | 正确 |
| 已知缺口 (3) | 旧版 cx-booter-ape 与 tools-default-regression 为 UNKNOWN(no-producer) | 0.0.105 cx-booter-ape-deferred.tsv 与 v0.0.100 strict 行各为 UNKNOWN rc=3;notes 0.0.79/0.0.85/0.0.86 记 old-checker/booter-raw-elf 无生产者 | 正确 |
| 补丁清单 | P0 chain-step、list-cap、选择器复用、U08、WP4、两个生产者、seal-precheck 均为待合入 | 107 树中 tools/seal-precheck.cx 与 plans/seal-precheck.md 不存在;check-slice-migration-gen.cx 的 source-missing 命中 0;U08/list-cap 均未见入树 | 正确 |

## 2 需要改的地方

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| 1 | 验证与工具改动 (1) "以各片真实断言并集和同候选冷态回执复核原覆盖" | 把未测写成已测:同一草稿的"已有实测"表对 module-units 与 cxmod-tri 写的是 UNMEASURED(待归档各片断言与耗时),仓内也还没有 107 候选上的冷态回执;目前只有 ompgpt 的私有分片证据 | 改为"待在本版候选上按各片真实断言并集与冷态回执复核原覆盖",或在候选实跑后再写成已完成 |
| 2 | 产品改动 (2) "GCC 默认也接受,但本机对照会发补齐警告" | 无证据:tests/check-tentative-array-incomplete.cx 里没有任何 gcc 对照(grep gcc 0 命中),G45 只记录了"gcc 默认接受"的说明,没有实跑本机对照;把它写成既有事实属于把未测写成已测 | 删去"本机对照会发补齐警告",或改成"按 G45 记录,gcc 默认接受(本仓未实跑宿主对照)" |
| 3 | 补丁清单 title "F1 与负控覆盖、B1 2/3 与 3/3 完整重放" | 与事实相符,但完整重放同时依赖 a183bd4e5d(2/3 与 3/3 引擎行)与 ac483247c7(夹具与 v2 增量);只列 ac483 会让人以为 a183 不存在 | 可保留 ac483 作为最终落点;若写"完整重放",补一句引擎行源于 a183bd4e5d |

## 3 结论

草稿里的提交号(5 个)、批次数(module-units 3、cxmod-tri 2)、F1 的 22/4/1、B1 的 2/8、5/5、12/2、封版工具 5/0 与变异 1 红、不动点 v4 的 PRECOMMIT 原行,全部与实际一致;没有发现提交号错。两处需要改:验证段 (1) 把 module-units/cxmod-tri 写成已复核(实为 UNMEASURED),产品改动 (2) 的"本机对照会发补齐警告"无证据;另建议注明 B1 引擎行来自 a183bd4e5d。

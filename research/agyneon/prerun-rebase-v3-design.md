# `prerun-drive --rebase-rev` v3 数据安全设计合同 (`main`)

- 维护者: `agyneon` | 性质: 数据安全件 (须经独立 `gatekeeper` 与 `cdx` 复核)

## 1. 真实产品 SID 与输入白名单

- 通过 `UNISACC_IDENT_HELPER` (`argv` 直调, 不用 `sh -c`) 实算 `old_rev` 与 `new_sha` 的完整 `SOURCES_SID`, 覆盖 `seed/ident.c` 全部产品与构建支持输入, 不靠四目录 Git 树哈希替代。
- `git diff --name-only` 仅放行纯测试/派生表路径; 任何产品/构建支持输入变更均拒绝。

## 2. 规范 40 位 SHA 与同事务替换 `frozen-root`

- 入口 `git rev-parse --verify <new_rev>^{commit}` 解析 40 位规范 SHA `new_sha` (拒绝 `"HEAD"` 等符号名直接落盘)。
- 先在临时目录 `frozen-root.new.<pid>` 解出 `git archive <new_sha>` 并核对 `candidate` 一致性, 再原子替换 `frozen-root` 与 `frozen-source.rev`; 任一步失败原地回滚, 保证目录、`REV`、根三者一致。

## 3. 真实依赖闭包、无截断 diff 与只读计划 (`--plan`)

- `git diff --name-only` 必须无截断 (`r.truncated == 0`), 截断即 `REFUSE`。
- 窗口受影响判定使用 `tests/batches.tsv` + `tests/batch-product-inputs.tsv` + `tools/affected.cx` 的真实依赖闭包与被改文件交集, 不靠报告文本 `strstr` 猜。
- 随机/确定性 `10%` 抽样必须真正 `sy_unlink` 对应 `done.W*` 标记, 且检查 `sy_unlink` 与 `sy_dir_next` 返回值, 失败立即报错中止。
- 先支持 `--plan` 只读计划输出, 按实际标记、根、失败原地验证后再放行写操作。

## 4. 卫生与登记

- `cxtrunc` 零命中 (禁用丢弃返回的 `memcpy`, 改有界拷贝)。
- 夹具 `tools/check-prerun-rebase.cx` 补齐每条阻断的必红变异并完整登记批次与 `tools-compile-sources.tsv`。

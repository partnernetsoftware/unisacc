# c42 fresh env-pin 定点闸分桶回执（2026-10-12 SGT，只读分桶）

性质：分桶记账，**不是验收通过**。acceptance FAILED，release NOT ready。不改验收措辞，不为过线改口径。

## 1. 运行身份

| 项 | 值 |
|---|---|
| GATE | `/tmp/c42-1640/cand.queue`（fresh start，未 resume） |
| 候选 | `unisacc-next.com` sha256 前缀 `e221516a`，build.json commit `ffbc9e11` |
| 源闭包 | `sources_sha256` `23e09a89…`，与当前树闭包一致 |
| env-pin | `3dffcca1…`，vars 名：HOME, LANG, LC_ALL, LOGNAME, PATH, SHELL, TERM, USER（只记名，不记值） |
| H1 | INHERIT（`archive/plans/v0.0.38.md`，由 `release/tools/h1source.py` 读出） |
| 结证 | 721/721 completed，88 failed，6 unverified，`queue.sh` final rc=1 |
| 旧对照 | `/tmp/c42-1424-envpin`（cand e221516a @ e016ecc1，721 项）仅作史证 |

## 2. rc 分布

| rc | 数量 | 旧 721 | 记账 |
|---|---:|---:|---|
| 0 | 627 | 628 | PASS |
| 1 | 48 | 53 | 见第 3 节 |
| 2 | 7 | 7 | RESOURCE_UNKNOWN（缺内存证据） |
| 77 | 6 | 6 | UNVERIFIED_HOST（宿主不适用，覆盖义务保留） |
| 142 | 33 | 27 | HOST_TIMEOUT / 超时记账，不当产品红 |

## 3. rc1 分栏（48）

**a. 前驱连锁（25）**：`rowcov-parse2-*` 全部为 `predecessor rowcov-parse2-build failed`，耗时 0.00s，未执行。根因在 `rowcov-parse2-build`（rc142，超时），不计为独立红。

**b. 独立 rc1（23）**，逐类记账：

- **真红 3（裁定面 / 构建形态，与 41 同构，记债/INHERIT）**：
  - `lib-lifecycle`：驻留量阈值，属验收裁定面。
  - `com-multi`、`com-forward-multi`：`-run` 转发需要动态编译器镜像，候选为静态链接（`ldd`：not a dynamic executable），默认运行也失败，`-o` 通过。
- **文档 / 台账 / 冻结**：`ledgercheck`（carry into v0.0.42.md mismatches 1）、`freezecheck`（closure commits without fix: 2）。
- **新出现，待分诊**：`seedmemory`（FAILED failures=2）。
- **宿主工具**：`com-elfobj`、`elfobj`（宿主 ld 不支持 aarch64elf 仿真，疑似环境）；`windows-resolver-host`（缺 zig）；`syscall6`（ok2 wrong0 skipped1，待看）。
- **元套件（撞车或测试脚本问题，非产品）**：`queuestart-launch`（另一 queue.sh 在跑）、`queuetimeout`（`$2` 未绑定）、`queuetimer`、`observation`。

## 4. 其它分类

- **rc142 超时（33）**：`closure-c1..c4`、`exec-e5`、`exec-f1-attributes`、`exec-macself`、`lib-callable-variadic-model`、`shared-e2-plain`、`stages-1..3`、`seedgen-ppconsumer-1`、`exec-warningdriver-ua-Wall` 等，按超时记账，不当产品红。
- **rc2（7）**：`seedparse2-1/-2`、`seedgen`、`seedgen-2..4`、`com-seedgen` 等，缺内存证据，需单 owner 单项测量后入证。
- **rc77（6）**：`exec-bootstrap-osxarm`、`exec-bootstrap-osxx86`、`lib-stack-arm`、`lib-stack-x86`、`lib-stack-x86-hostabi`、`lib-windows-gp-native`，宿主不适用，覆盖义务保留，不是 PASS。

## 5. 本版不挡 Draft 材料的依据

按机房主任代裁：真红 3 与 41 同构，记债/INHERIT，不挡 Draft 材料；不改 acceptance 措辞；不为过线硬啃 lifecycle 配额或动态链接设计面。

## 6. 未完成的材料项（阻塞）

- **candidate 封存（GHCR）未完成**：`release/tools/seal_candidate.sh` 会把候选推送到 GHCR 公共 registry，权限层拒绝了这一动作（Create Public Surface），本回执不绕过。
- 因此 `release/candidate.json` 未更新，`rc/v0.0.42` 未打标签，release-check 的 candidate 作业无法绑定封存字节，unsigned-receipt 与 Draft 的 run id/attempt 也还没有。
- `gh release create --draft` 尚未执行。Draft、Latest、签名、公开都没有做。

## 7. 下一步

1. 由有权限的人确认是否允许封存到 GHCR（公共 surface）。允许后再按 `RELEASE-PIPELINE.md` §9 第 4–5 步执行。
2. 逐条分诊 `seedmemory`、`freezecheck`、`ledgercheck`、`syscall6`，并补 rc2 的内存证据。
3. 真红 3 按裁定面记债，不硬改。

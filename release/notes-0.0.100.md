# unisacc 0.0.100 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | b348b6f9fdebf5b08929af34cb4854f018a98122 |
| 发布 APE SHA256(unisacc.com) | e173defd0cccf70cf4790f7317db11c29b999fb2a6349268b4d8b5e830186bf9 |
| 源快照 SID | 52031be564cf11c4db3da54cc33718227021e1182fb7e2b77a70404d8129922a |
| pkg.bin | 43a20658d8cf57328c0d0d0ec28128678b7e83cb497b1ef8a6ecabdba9dff4b8 |
| slice-win-arm64 | e1a5edaaa6ad0e1bac4097c1ba41f01359db37e84a9c748d034bc7b487019166 |
| SHA256SUMS | 91e3b71c3a97d3564efe52d30241ee11d0002e30836e9895fe2638ae10db155d |

## 严格证据

466 批:PASS 463、FAIL 1、UNKNOWN 2、INVALID 0

## 产品改动

### 前端 A 两项 C99 修复(0.0.100 主线)

- (1) `c99-extern-array-incomplete`(14/14 PASS):修复文件域不完整 `extern` 数组声明前瞻早退问题与两单元链接行为,14 例正负控回归全过。
- (2) `c99-paren-lvalue-postinc`(17/17 PASS):修复括号内可修改左值后缀 `++`/`--` 与外层解引用解析,GCC C99 双 oracle 17 例正负控回归全过。
- (3) 两代自举固定点(gen1 190/190 全绿 `obj_same=1506 obj_diff=0 old_fail=0`, 完整 499/499 链产物打包并经全批严格预跑验证)。

### 所含 0.0.99 已合入内容(合并随 0.0.100 公开)

- (1) `run-cache` `__FILE__` 相对路径键绑定与 `UNISACC_RUN_CACHE_KEY_OLD=1` 兼容回退开关:消除跨目录同源字节同相对参数复用错误内嵌路径对象的问题。
- (2) `cx/sys/fs.cx` 新增 `sy_statvfs` 接口与跨平台系统层支持。
- (3) `mutation-gate` 临时工作区自清与门禁稳定性改进。

### 关于 0.0.99 记为未公开的说明

- `0.0.99` 记为未公开,不打 tag、不发 GitHub Release,其整链(`44d1a68dbd`)与 191 窗严格预跑(`461 PASS / 1 FAIL / 2 UNKNOWN`)证据完整保留备查。
- 未公开原因:静默窗解禁后 `main` 合入 `0.0.100` 期 `cx/sys/fs.cx` 改动导致 `main` 源快照 `SID` 前移;现行发布工具链硬编码 `origin/main` 门禁,而分支直发改造补丁按审阅结论退回拆小、不在发布途中首次投产;经裁定(方案丁)一次性冻结 `main` 并直接发布包含 `0.0.99` 全量已合入内容的 `0.0.100`。

## 已知缺口

- (1) 整体裁决 `NOT_QUALIFIED`:全套 `s9` `daily`/`release` 长跑未执行,原生 CI 仅覆盖冒烟子集,不宣称全批发布资格。
- (2) 严格预跑结果(466 批:PASS 463、FAIL 1、UNKNOWN 2、INVALID 0):
  - `FAIL=1`:`precommit-coverage-root`(`W0189`, 首轮高负载超时触发串行 `W0189.z05` 得出 `count=4/1`, 空载整窗重跑 `W0189` 干净产出 `count=4/1`)已登记于 `release/evidence/0.0.100/expected-fail.tsv`(原因为 `672506c776 -> e594d28f9b -> 5bdaffce61` 恢复 `tests/run.cx:76` `realpath(__FILE__)` 致 `rc_raw` 绑定工作树路径,次私有目录覆盖率求值非冷态,跟踪至 `0.0.101`, `expires=0.0.101`)。
  - `UNKNOWN=2`:`cx-booter-ape` 与 `tools-default-regression`(`W0011`, `no-BATCH_RUN rc=3`)均为既有无生产者挂账项,已按 `expected-unmeasured.tsv` 与对应 `*-deferred.tsv` 见证登记。
- (3) 严格预跑超时重试与外部负载披露:
  - `W0083`、`W0084`、`W0092`、`W0096` 首轮 `rc=124` 超时,次轮 `attempt=2` 全部通过;同期有外部造负载(`omp-sonnet` 三波 16/24/24 个空转进程加三路冷编与首轮超时时间接近,可能相关)。
  - `W0000`(`tools-compile-021`)与 `W0001`(`lane-key-probes`)首轮因 `frozen-root` 首次解包未预热、冷建 layout 与冷编工具贴 `PD_CHILD_SECONDS=58s` 上限超时(`rc=124`),清理半截残留后经 `--resume` 补跑一次通过(`W0000` `wall_ms=6181` `PASS`, `W0001` `wall_ms=47177` `PASS`)。
- (4) 非 Linux 平台实机验证由 GitHub Actions `candidate-native.yml` 覆盖:首轮 CI `45283344177` 中 `osx/x86_64` 格 `FAIL`(`stdio-crossunit` `rc=-1` `ms=55082` 55s 超时、`stream-pipe` 四段 `STREAM_PART` `rc=-1` `ms=55025-55033` 55s 超时,`CANDIDATE_CELL osx/x86_64 FAIL`)加 `upload-artifact` `ENOTFOUND`;按 `--redispatch` 重派 `45285851131` 6/6 全绿通过(详见 `release/evidence/0.0.100/INDEX.tsv` 与 `ci-45285851131.{json,log}`)。

## 回滚方案

公开后如需撤回,按 `release/rollback.txt` 执行:(1) 将 `v0.0.100` release 转回 draft 并把 Latest 移回 `v0.0.98` 后读回;(2) 在 `main` 按逆序前向 `git revert` Y2/Y1/X 及相关提交并推送,绝不删 tag 或 release;(3) 使用 `tools/install-ape.cx` 从既有 `0.0.98` 备份回装本机 Root 并核对版本。

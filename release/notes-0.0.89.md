# unisacc 0.0.89 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 74f541c6cc5b5c9808a3324f849c8982ffc93648 |
| 候选提交 | f77e64a25b3ad63e9bc9ca55247c8a89dfefdc15 |
| 发布 APE SHA256(unisacc.com) | 384e115227525d21c2392bc03aa251586592d411c685d9db041867b63d7ff3a7 |
| 源快照 SID | 66bb32dd67791c68dcef148dadb662b2a08639c534ac685cec78c6f21b6362be |
| pkg.bin | caed773f60bd05860ad0fcf74c38d16d925c2b0a3d02bacd1295637c4a6ca8bc |
| slice-win-arm64 | a04aedbcd984c4bc016ed8438ed6bce447d0d4f631e27cb0c74e21f78d4a74ac |
| SHA256SUMS | d772a97dc4a7de5a6246cd843fd5fb90b2a2c1c25754944fb8bc68bb8dd2c42f |

## 严格证据

442 批:PASS 440、FAIL 0、UNKNOWN 2、INVALID 0

## 产品改动

- 批 A(三元回扫根修): 修复 `exec/parse2/{return-result,return-stack,errors-template}.tsv` 三元表达式回扫类型丢失与浮点/字符串分支问题。
- 批 B(cx 后缀判据放宽与单段名诊断): 放宽无 `.cx` 后缀可执行脚本运行判据(`cxu_iscxunit` 与驱动 6 处), 补全单段模块名诊断文案并同步三源等价表 `build/cx0-three-source-diff.tsv`。
- 批 C(A1a 内建核心 U0-U3): 实现未实现 `__builtin_*` 具名拒绝(U0)、`__builtin_{clz,ctz,popcount,parity,ffs}[l|ll]` 常量折叠与运行期展开(U1, 含 `b08` 常量折叠与 `b15` 取址拒绝)、`__builtin_bswap{16,32,64}`(U2)、`__builtin_unreachable` 与 `__builtin_trap`(U3, 含 `ACV.voidsrc` 禁止 `void` 当值); 全部裸立即数收敛至事实表 `exec/facts/bi-const.tsv`。注: 按 `plans/v0.0.89.md` 已定口径, `__builtin_trap` 采取零地址写触发 `SIGSEGV`(rc 139, 非 `SIGILL` rc 132), `tests/check-a1a-u3.cx` 判据已同步对齐。
- 批 D(`ds-attr-leak`): 修复 `src/drivestream.cx` 流属性泄漏。
- 批 E(`safedel-tmpshadow`): 修复 `include/unisa_safedel.h` 在 `$TMPDIR` 根目录存在外部瞬时 `.git` 时 `cx_rmtree` 误判拒删问题, 同步 4 项派生物(`include/cx-flat.h`、`exec/facts/pp-autoinc-gen.tsv`、`build/lane-inputs.tsv`、`build/libc-decls.tsv`)。

## 构造过程说明

- 候选首链 `chain1`(`0b9461450f`) 在 `464/499`(`g2-lnx-arm64-closure` attempt 133) 撞上外部瞬时 `/tmp/.git` 触发 `0.0.88` Root 旧 `cx_rmtree` 误判(`BATCH_STAGE_WITNESS /tmp/.git lstat=0`, 正是本版批 E `safedel-tmpshadow` 根治的触发场景, 装上 `0.0.89` Root 后免疫); 后续续跑因未预置 `--base/src` 及 `chain-to-snap` 清空共享快照 `out/chain/` 判定作废并保留完整证据。
- 合入 `origin/main`(`43a7e56462`)得候选点 `T2=f77e64a25b` 后预置 `--base/src` 启动 `chain2`, 31 分钟零中断一次完成 `499/499 DONE` + `PROMOTE_CHECK OK`(与 `chain1` 同 `SID=66bb32dd`); `ci-early` 六格原生 CI 一次性 `6/6 PASS`(`run_id=44926567999`, 6.7 分钟); 严格预跑 `prerun2`(与 `0.0.88` 同口径 442 批) 完成 `440 PASS / 0 FAIL / 2 UNKNOWN`(2 个已知无生产者批 `cx-booter-ape`、`tools-default-regression`)。

## 已知缺口

- 全套 s9 尚未运行, 整体裁决保持 `NOT_QUALIFIED`;
- `U4`(asm 标签)已拆为独立版本 `0.0.90`;
- 2 个无生产者批(`cx-booter-ape`、`tools-default-regression`)在严格预跑中为 `UNKNOWN(no-producer)`;
- `tests/stream-lower-domain.cx`(`daily` 批 `stream-lower-domain-format`)存在历史未声明函数 `rt_tail` 缺陷(实测 `0.0.88` 公开 Root 同挂, 非 `0.0.89` 回归), 以及 `prerun-drive` 799 批撞顶 `PD_MAX_WINS=512`(已记 `R74` 排 `0.0.90` 前修复);
- `tier=daily` 批 818 个中 490 个自 `v0.0.86` 起未被冷编, 全量冷编扫 117 个唯一入口(383 批)在 `0.0.88` 产品下 115 通过 2 失败(`stream-lower-domain`、`cx-host-run`, 均为测试源码缺前向声明, 发布后修);
- 本机严格预跑只在 `lnx/x86_64` 执行; `osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 与 `lnx/arm64` 原生仅由 `ci-early` 六格冒烟子集覆盖, 未跑全量(第 4 步全量覆盖为 `UNKNOWN`)。

## A1a 语言行为差异

- `__builtin_trap`: 采取零地址写触发 `SIGSEGV`, 退出码为 139 而非 `SIGILL`(退出码 132);
- `__builtin_bswap{16,32,64}`: 常量表达式位置(`case` 标签、枚举值、数组维度、静态初值等)不可用, 仅支持运行期求值;
- 常量位置浮点实参: 内建函数在常量表达式位置传入浮点实参时直接被拒(GCC 接受并作浮点转整折叠);
- `__builtin_clz(0)` 与 `__builtin_ctz(0)`: 输入 0 时常量折叠与运行期统一得目标位宽值(32 或 64);
- 取内建函数地址: 对 `__builtin_*` 取地址的拒绝文案与 GCC 不同, 但同为编译期拒绝(`b15`);
- `__builtin_unreachable` 后 `-Wall` 告警: 非 `void` 函数在 `__builtin_unreachable()` 后无显式 `return` 时, 开启 `-Wall` 仍有缺 `return` 告警现象;
- 跨平台验证范围: `osx` 与 `win` 目标仅 `ci-early` 六格原生冒烟覆盖, 未跑全量批次。

## 回滚办法

- 候选级(未公开前): 保持草稿 release(`v0.0.89`)私有不公开; 若需换候选重跑 CI, 执行 `sh ./unisacc.com tools/release-chain.cx -- retry-ci --version 0.0.89 --run-id <RUN_ID> --evidence-dir release/evidence/0.0.89 --candidate release/candidate.tsv` 归档失败 CI 证据并删除陈旧草稿; 本机若已按候选模式试装 Root, 执行 `sh ./unisacc.com tools/install-ape.cx -- . --rollback --write` 恢复 `.unisacc-root-backups` 中的 `0.0.88` 备份并核 `--version` 为 `unisacc 0.0.88`。
- 公开后(适用窗口: `0.0.89` 公开后至下一版本公开并完成读回, 回退目标 `0.0.88` 公开 APE `0e028e7de8dc1c4a5e6a5b3120e137daf8b2d3a8b3068d5af7f971a3c3a46854`, 详见 `release/evidence/0.0.89/rollback.txt`):
  - (1) 前置与 release 撤回: 保留 `0.0.88` 公开资产与 `release/evidence/0.0.89/` 全部回执; 先将 `v0.0.89` 转回草稿并把 `Latest` 移回 `v0.0.88`, 随后读回 `isDraft`、`tagName`、资产及 `Latest`:
    `gh -R partnernetsoftware/unisacc release edit v0.0.89 --draft=true`
    `gh -R partnernetsoftware/unisacc release edit v0.0.88 --latest`
    `gh -R partnernetsoftware/unisacc release view v0.0.89 --json tagName,isDraft,assets`
    `gh -R partnernetsoftware/unisacc release view v0.0.88 --json tagName,isDraft,assets`
    `gh api repos/partnernetsoftware/unisacc/releases/latest --jq .tag_name`
    若确需删除 release 与 tag(前置: `Latest` 已切回 `v0.0.88` 且确认无外部引用): 执行 `gh -R partnernetsoftware/unisacc release delete v0.0.89 --yes` 与 `git push origin :refs/tags/v0.0.89 && git tag -d v0.0.89`。
  - (2) 共享 `main` 发布记录前向撤销: 从新到旧执行 `git revert --no-edit 57de2b4ddd`(Y2)、`git revert --no-edit db00056f10`(Y1)、`git revert --no-edit aaf5e424b2`(X) 并 `git push origin HEAD:main`, 不回退冻结后他人代码, 禁止 `stash`/`reset`/`clean`/`checkout`。
  - (3) 本机装机回退: 有备份槽时执行 `sh ./unisacc.com tools/install-ape.cx -- . --rollback --write`; 无备份槽时下载 `0.0.88` 公开 `unisacc.com` 核对 `sha256=0e028e7de8dc1c4a5e6a5b3120e137daf8b2d3a8b3068d5af7f971a3c3a46854` 后执行 `sh ./unisacc.com tools/install-ape.cx -- . <APE_PATH> 0e028e7de8dc1c4a5e6a5b3120e137daf8b2d3a8b3068d5af7f971a3c3a46854 --write`, 随后核验 `sh ./unisacc.com --version` 输出 `unisacc 0.0.88`。

本说明首版缺项,已于 2026-11-22 追加

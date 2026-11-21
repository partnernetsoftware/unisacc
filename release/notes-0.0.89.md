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
- `tests/stream-lower-domain.cx`(`daily` 批 `stream-lower-domain-format`)存在历史未声明函数 `rt_tail` 缺陷(实测 `0.0.88` 公开 Root 同挂, 非 `0.0.89` 回归), 以及 `prerun-drive` 799 批撞顶 `PD_MAX_WINS=512`(已记 `R74` 排 `0.0.90` 前修复)。

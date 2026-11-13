# unisacc 0.0.59 -- 发布说明(草稿)

状态:草稿,链与尾部结束前不是最终状态。以下带 [待填] 的项目尚无事实,不得当作已通过;未实跑的项一律写未跑。

- 冻结提交 `fbdc99ca8921514fca927781a85ec93fbb5a82ec`(标签 v0.0.59-src2;父提交 v0.0.58-src `2a46e1a427`,加六个 P59R 文件与 release/candidate.tsv 的版本列;账本 C3-START5-059、C3-C-UNIT-COMPAT)。
- 提交 X:[待填: 只含 release/candidate.tsv 的提交哈希,来自发布尾部]。
- 提交 Y:[待填: tag v0.0.59 所指提交,来自发布尾部;本文件随 Y 提交]。
- 候选 APE `unisacc.com` sha256 与字节数:[待填: 来自 out/b1/fbdc99ca8921514fca927781a85ec93fbb5a82ec 的 pack 结果],`--version` 实测:[待填]。
- git 范围 `v0.0.58..Y`。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.59 的产品改动类别是「路线上行」(route upstream),且在本候选内缩小了范围,见下。上一发布版见 `plans/release-notes-0.0.58.md`。

## 行为变化(产品输入)

本候选包含:

(1) 链接路线经 cx.stream 管道(routepipe)。
(2) tl_dce:链接期丢弃不可达函数。
(3) fwdsig 共享。

本候选撤下,推迟到 0.0.60:

- 对象路线上行,即 run-forward 前段给单元对象加前推标记(fk_mark)。
- 撤下原因(账本 C3-GEN1-FIX、C3-C-UNIT-COMPAT):首个候选的 gen1 编译器拒编 cx/stream/item.cx,报 `not covered: library signature resource or duplicate definition`;修正后 C 单元调用别的单元里签名含按值结构体的函数仍被拒,0.0.58 同命令返回 0。取缩范围方案,只改 cx/compiler/routepipe.cx(c70f2c8002)。

行为后果:

- 分离编译、之后在链接期才由宿主前推的情形,与 0.0.58 一样仍被拒,典型报错 `link: undefined reference to pread`。本版不修复。
- 其余 stream 命令行为设计预期不变;[待填: 同候选 CI 与字节等价检查对此的实测结论]。

## 随发布带出的工具与规则变化(非产品输入)

- 限值表 build/limits.tsv 与生成器 limits-gen;链运行身份不含限值。
- 宽度回升(C1):chain-run 降宽规则改为可回升。0.0.58 链诊断:264 次尝试,墙钟 7985 秒,同时只有 1 次尝试在跑的占 92.5%(账本 C1-PARALLEL)。本链实测效果:[待填: 来自 chain-run.tsv 的并行度测算]。
- 表驱动与钉住:whole-front、preflight、release-build、release-ape 按输出目录钉住(CHAIN-PIN-REST);续跑与残留计数改为表驱动规则(RESUME-RULE,build/chain-residue.tsv);release-oneshot 的身份绑定。
- s9 重型域宽度 2,分片归属 v2;预提交加登记配对门;tlib 批拆分;check-reaper 的 nproc 上限修正。
- 链调用统一由 cxlib 的 chaincall 单元决定并校验(账本 C3-CALL-PLAN),起因是本轮前三次起链都停在两边不一致上。
- cursor-fleet 派工器(只是工具,不进产品)。
- tlib 包的预稳定策略:build/tlib-policy.tsv 中 stable 为 no,版本升级上限到 Z 位。std-surface v2 与 stable-eval v0 已有;没有任何包被宣布稳定。

## 整链与预演经过(如实)

本版共起链 5 次:

(1) 第 1 次:snap 相 preflight 规则钉了字面量 55000,与限值表改动冲突,链 0/498 停(修正 b9da262839,账本 C3-PREFLIGHT-FIX)。
(2) 第 2 次:分组相的段内限时被拒,10/498 停(修正 65b5ce2565,账本 C3-STOP-059)。
(3) 第 3 次:分组相段尾调用被拒,10/498 停(chaincall.cx,0223ddf096、9ffe37c948,账本 C3-STOP3-059)。其后先用真表干跑再起链(C3-DRY-059)。
(4) 第 4 次:到 p3(329/498)后,在 gen2 的 468/498 因产品缺陷停(g2-lnx-x86_64-closure first-failure,账本 C3-PRODUCT-STOP-059),即上文的 gen1 拒编问题。
(5) 第 5 次:起于 fbdc99ca8921;已记录到 p3(332/498,无 KILL、WIDEN、RESUME、BLOCKER)。最终结果:[待填: 来自 chain-run.tsv 与 chain-run 收尾行]。

链工具取自工作检出,冻结的是产品输入,不是链工具。

## 身份与证据

| 对象 | 结果 | 证据 |
|---|---|---|
| 整链 | [待填: N/498 到 pack、PROMOTE_CHECK、compare 结果] | out/b1/fbdc99ca8921514fca927781a85ec93fbb5a82ec |
| 链墙钟与各相 | [待填: 总墙钟、各相秒数、并发度] | 同上 chain-run.tsv |
| kernels-g2、kernels-g3 | [待填: 实测秒数,对照 90 与 120 秒时限] | 同上 |
| 打包 | [待填: release-run --pack 结果、源 SID] | [待填: 路径] |
| 七资产 | [待填: 草稿读回逐件同字节、SHA256SUMS、release-layout --check] | [待填: 路径] |
| 同候选原生 CI | [待填: run id、verdict、pass/fail/not_run/invalid] | [待填: 路径] |
| s9 commit 档 | [待填: 一行结论] | [待填: 路径] |
| release-decide | [待填: 一行结论,需含 unknown 与 fail 计数] | [待填: 路径] |
| 公开后读回 | [待填: 资产与发布页正文读回] | [待填: 路径] |

win 与 osx 的真实可执行体只由同候选 CI 判定,本机为 linux 宿主。

## 已跑与未跑

已跑:[待填: 链内批、s9 commit 档、候选专属批的实际结果]。

未跑:

- s9 的 daily 档与 release 档未跑。
- osx 批在 linux 宿主上 NOT_APPLICABLE。
- 仅对候选有意义的 PLANNED 批 cand-unit-compile、check-link-dce、check-route-fwd、route-equiv,对链产出候选的运行结果:[待填: 逐批结果,未跑则写未跑]。

## 临时放宽(如实)

- LIMIT-90-TEMP:链相位与批子进程时限 55 秒放宽到 90 秒,由项目所有者 2026-11-12 授权,临时覆盖「不放宽时限」铁律,尚未回收。
- LIMIT-120-KERNELS-TEMP:仅 kernels-* 相位 90 秒放宽到 120 秒,到期 2026-12-31;依据 0.0.58 中 kernels-g2、g3 实测 80 至 88 秒逼近 90 秒限。到期前须回收或重新裁定。

## 已知缺口(如实)

- DEBT-C-STATIC-ASSERT:前端对含 `__builtin_offsetof` 的 `_Static_assert` 与数组界报 `not covered: nonconstant bound`,未修。
- 18 个参考 C 夹具需要外部 -I 目录,不是回归,未处理。
- check-model-decode-real 自 C 参考归档(e16028ffa1)起损坏,未修。
- 链工具中仍未钉住的:entry-build、source-job-wave、net-deps。
- 对象路线上行与 C 单元按值结构体兼容推迟到 0.0.60。
- 文档整并不属于本版。
- 整版超 2 小时铁律:[待填: 本链实测整体时长与结论]。
- 沿 0.0.58 未处理项:BASE6 字节 oracle、p3 release-ctor 材料、全域矩阵 release 档红格。

## 撤回与回滚

撤回公开:`gh release edit v0.0.59 --draft=true -R partnernetsoftware/unisacc`;彻底撤回再删 tag:`git push origin :refs/tags/v0.0.59` 与 `git tag -d v0.0.59`,依运行手册。本机根回退到 0.0.58:`sh ./unisacc.com tools/install-ape.cx -- ROOT <v0.0.58 公开 APE> 3744eacfa6b2472de8b18ef3d64ffad6f6a47f956d62e4a43760dee6e768632e --write`。候选表回退到 0.0.58 记录:[待填: 0.0.58 发布提交 Y 的哈希,取其 release/candidate.tsv]。

规则:发布页正文必须与本文件逐字相同,公开后读回核对(DEBT-RELEASE-BODY-READBACK)。

## 发布前须填清单(fill before publish)

- [ ] 提交 X 哈希
- [ ] 提交 Y 哈希
- [ ] 候选 APE sha256、字节数、`--version`
- [ ] stream 行为的 CI 与字节等价结论
- [ ] C1 宽度回升的实测效果
- [ ] 第 5 次链的最终结果
- [ ] 身份表全部 [待填]:整链、墙钟与各相与并发度、kernels-g2/g3 秒数对 90/120、打包、七资产、CI run id 与 verdict、s9 结论、release-decide 一行、公开后读回
- [ ] 已跑项与候选专属 PLANNED 批结果
- [ ] 整版时长结论
- [ ] 0.0.58 发布提交 Y 哈希(候选表回退用)
- [ ] 发布页正文与本文件读回一致

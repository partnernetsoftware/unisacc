# plans/pending —— 未落地的半成品（2026-10-23 停工时收集）

这里的东西**都没有进代码树**，也都**没有经过完整验证 + 落地**。接手者逐项：在 scratch 导出里应用 → 跑对应闸 → 用落地配方（`plans/HANDOFF.md` §3）提交。
`.cx` 文件加了 `.pending` 后缀，免得 cxinclude / glossarycheck / zeropython 扫到它们；落地时去掉后缀放回原路径。

| 目录 | 内容 | 状态 | 基准 |
|---|---|---|---|
| `f1/` | 0.0.43 F1 产品修复批：6 刀自带 libc + `enum E;` 回归 | 已构建候选并 `ident` 通过；未做语料差分、未重封、未落 | 09e29be6（`git am` 0001–0007） |
| `detach-w2/` | unisa 剥离 W2 的 A、C、D1、E、F 组补丁（B、D2、D3 未产出） | 各组代理在自己的导出里验证过；未落 | 约 1c0aab8f（逐个 `git apply --check`） |
| `delgov-fix3/` | 删除守卫第 3 轮：core（fd 遍历、TMPDIR 祖先、天花板、清除策略）、scanner（删除写法全覆盖 + 豁免行哈希；`scanner-new-sites.tsv` 是新扫出的点）、js（JS/Makefile 点位）、triage（受影响闸分拣） | 补丁完成；**未落、未经对抗验证**；core 改 include/，需重建产品 | dbd14bda |
| `flownet-w5/files/` | flownet W5：覆盖三态（47 程序：覆盖 39 / 拒绝 8 / 未测 24）+ `flownet/README.md` + `docs/flownet-migration.md` | 实现并在 scratch 验证；未落；另需 gate.sh 两个 job（flownet-w5-cover、flownet-w5-oracle）+ gatelayers/stagemap/INVENTORY 登记 | 约 92e0ec07 |
| `py2cx-w7-unlanded/` | 第 7 波未落的 5 个移植：exec/c 的 librarystackcheck、netcheck；exec/parse2 的 diagnosticcheck、errorcheck、warnprep（文件名中 `_` 代表 `/`） | 未知（该波落地者没落它们）；需重跑与 .py 的逐字节对拍 | 约 e3c4b123 |
| `tapebin-selfheal/` | tapebin 参考缓存自愈（坏条目移开 + 私有构建 + 原子改名） | **半成品**补丁 | 1c0aab8f |

另：仓库根目录的 `true`（某闸误写出的 ELF）和 `unisacc.com.prev-*`（旧产品备份）是未跟踪杂物，不提交；产品备份在作者本机 `/home/devali/backups/unisacc/roots/`。

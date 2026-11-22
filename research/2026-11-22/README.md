# research/2026-11-22 设计与评审底稿归档

方法学 6.6:设计与评审产物入仓。本目录收 2026-11-22 前后各车道在个人 job 临时目录里写成、又被 `plans/v0.0.9*.md` 与 `prd_mempalace.md` 引用的底稿。只复制,内容除路径外不改(已核不含家目录绝对路径);各文件互相引用的 job 目录路径,凡指向本目录已有文件的,已改为本目录相对路径。原件仍在各 job 目录,以本目录为准。`plans/v0.0.90.md`、`plans/v0.0.91.md`、`prd_mempalace.md` 里指向这些底稿的路径同步改为本目录相对路径。

本目录只有被点名归档的文件;原目录里的探针、夹具、补丁、日志、`.cx` 脚本和符号链接不在此(未复制,按需从 job 目录取或重做)。

| 路径 | 内容 | 关联台账 |
|---|---|---|
| r76-design/design.md | R76 明文 HTTP/1.1 服务端(ext/net B 类库)设计稿 v2 | R76 |
| r76-review2/review.md | R76 第二轮守门人纸面评审(第一轮评审稿原目录里不存在,未归档) | R76 |
| r78-v090-named/summary.md | 0.0.90 关键补丁集带 --expect-fail 具名杀重跑与 release-chain 存活变异补强汇总 | R78 |
| r78-v090-named/receipt.md | release-chain.cx sy_realpath 修复与软链拒绝先红后绿收据 | R78 |
| r69-6-shadow/report.md | R69.6 零断言守卫影子阶段盘点与评审落点报告 | R69 |
| r69-report/report.md | R69 普查综合报告 | R69 |
| zero-assert/plan.md、v089-shadow.tsv | R69 零断言守卫送审方案;0.0.89 影子批次盘点 | R69 |
| u4-slices/plan.md | 0.0.90 U4 汇编标签拆片 | U4 |
| u4-slices/wire-form.md | U4 记号与标签绑定 | U4 |
| u4-slices/archive-decisions.md | U4 裁定与 GCC 边界实测 | U4 |
| u4-slices/release-notes-diffs.md | 汇编标签与 GCC 有意差异(发布说明素材) | U4 |
| u4-slices/u4-1-receipt.md、u4-3-and-5-receipt.md | U4.1 交付收据;U4.3 与 U4.5 回执 | U4 |
| csr/tsv-lint-F/ 下三个文件 | tsv-lint F 类逐表分布、高频唯一项与常量 top5 | tsv-lint |
| csr/cold-ladder-closure.tsv、cold-iso-method.txt | 冷编译阶梯(逐模块加入的冷编译毫秒与闭包规模);冷测隔离方法 | 冷编译 |
| replay/replay-report.tsv | 遗留补丁重放 apply-check 报告 | 0.0.91 |
| sy-retval-census/report.md | 全仓 sy_ 返回正数或非负描述符函数被误作 == 0 比较的缺陷普查 | 库面 |
| r81-census/report.md | R81 永不失败的阈值断言与文本存在性自测盘点 | R81 |
| utf8-audit/report.md | tools 字节级切分的 UTF-8 边界审计 | UTF-8 |
| flow-next/v0.0.90-draft.md | 0.0.90 计划草案:U4 asm 标签加 R72 临时头年龄清理 | R72、U4 |
| v091-draft/plan-draft.md | 0.0.91 计划草案:身份与构建边界(L.1 加 Z5 加 B3) | 0.0.91 |
| r70-review/review.md | R70 cx 热启动提案的守门人纸面评审结论 | R70 |
| litlen/real-bugs-41.tsv | 字面量长度缺陷 41 条真缺陷逐条判定清单 | 字面量长度 |
| litlen-089/handoff.md | 字面量长度修复交接:发布执行两处提交与 29 条断言漏登 | 字面量长度 |
| r72-product/receipt.md | R72 临时编译缓存回收收据(补丁已落库,补丁文件不入仓) | R72 |
| r69-register/rt-shared-fix/receipt.md | rt-shared-gen 大块分配状态补登记回执(补丁已落库,补丁文件不入仓) | R69 |

第二批(按价值分级后续入,同一目录):设计稿、评审、判定清单与回执必须入仓;探针原始输出和已落库的补丁文件不入仓(补丁内容已在对应提交里,引用保持原样)。原目录已清理或不在本次范围的引用(`v090-ready/evidence.md`、`v091-ready/review.md`、`v094-ready/review.md` 等)没有归档,引用保持原样;r74 重放目录原目录里不存在。

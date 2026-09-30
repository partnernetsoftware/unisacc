# prd 归档（第四轮，2026-09-30，0.0.15 R15-0）：版本沿革与旧版本发布记录

prd.md 在 0.0.15 重写为“当前与将来”；下面是移出的历史段落，原样保留。

### v0.0.12（2026-09-30 发布，源 `c4d667e`）

- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.12 ；签后 `unisacc.com` fb607af5…（1,170,368 B），未签候选 df8cc9b4…（1,154,589 B）；最终树全量 350/350；验收 `research/r12-release-acceptance.json`。
- **多架构结果**：六托管 runner（lnx/osx/win × x86_64/arm64）对同一候选跑演示套件六格全绿（`research/r12-demo-matrix-c4d667e.json`）；全套件 ci 矩阵 lnx/x86_64、lnx/arm64、osx/arm64 全绿，osx/x86_64（Intel）163/181（18 项为 Python 参考路径的时间预算）；本机六目标 8/8 与 Windows 双自举。
- **[v]**：R12-0 测试债（清点/入门禁/usage/exec-formats/linuxbridgecheck/ua_ref 竞态/--list/bindingcheck 拆分）、R12-1 ① BANK 可执行部分（两 ISA 19 夹具）、R12-3 ③ 矩阵、R12-4 ③④（external 8/18、计数单一来源）、R12-5 ④ FX-6 量化负结果。**[-] 顺延 0.0.13**：BANK 表接线与宿主、R12-2、Windows 全套件 runner、网络裁判登记、R12-6 文档/规格/论文余项、FX-5。回执全文 `plans/v0.0.12.md`。


## 7. 附录

### 7.1 版本沿革

v1 到 v3.4 的逐版钉死条目，连同 14 阶段之前的模型总表，已归档到
[`archive/prd-history.md`](archive/prd-history.md)：那些数字记录的是当时的口径
（`OPS` 38、9 头、TYS 8→9、selfgap 31/73 等），与今天的代码不符，留在正文里只会
被当成现状读。

**此后的变化**按实验条目记在 §6：浮点 [E-54]、自举闭环 [E-55]、编译即运行与单文件
打包 [E-56]，以及表形决策的对齐（`regmap`/`tyinfo`/`pfconv` 三个新阶段、`isel`
退出 lowering、`abi` 去 `tls` 加 `nrreg`/`arg3..5`）。


**v0.0.10（2026-09-29 发布，tag ae6d512）**
- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.10 ；签后 `unisacc.com` f6e8e090…（1,168,488 B），未签候选 4ba24140…（1,152,711 B）；本地 332/332；验收 `research/r10-release-acceptance.json`。
- **闭合项**：干净机器 CI（后改为 release-check 快链 + 每周全量）、Windows 企业签名（qualification→company）、Apple 公证、共享 E2/24 网、源码/权重 include 分离、六单目标私有构建、进程内库主 API/线程/生命周期/回调/USLCALL3 别名、`-ftrim-libc` 改名、GHCR 候选封存。
- **顺延到 0.0.11**（已在 v0.0.11 节逐项回执）：V3 variadic 跨来源、pointee 身份、packed / 宽 FP / BANK、Windows SEH、六平台生命周期矩阵、公共 origin0 refinement、Paper A 定稿。
- v0.0.9（`a606ff4`）及 R9 状态、v0.0.10 全计划见归档索引。


**v0.0.11（2026-09-29 发布，tag 8b5abc9）**
- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.11 ；签后 `unisacc.com` e86cc61c…（1,170,384 B），未签候选 6a3dfce2…（1,154,605 B）；本地 338/338；验收 `research/r11-release-acceptance.json`。
- **[v]**：V3 variadic 跨来源、一层 pointee 身份、long double（IEEE64 profile）、packed 外部布局（AAPCS64/Win64）—— `research/r11-{variadic-import,pointee-identity,longdouble,packed}-evidence.json`；`-ftrim-libc` 默认（住在 E2 网络，`-fno-trim-libc` 退出；calc -run 188.7→142.7 ms）—— `r11-trim-default-evidence.json`；关系账 —— `r11-package-relations.json`；Windows 双目标编译器自举与六目标 crossnative；GHCR 每片重封。
- **[-] 顺延 0.0.12（理由在 R12 计划树对应项）**：general BANK（设计 `research/r11-bank-design.md`）、Windows SEH/六平台生命周期矩阵、us_eval/us_reload/us_opt_verify、网络裁判登记、目录职责梳理、论文定稿。
- 逐项回执全文见 [归档](archive/prd-history-20260929.md#v0-0-11)。

**v0.0.9 / v0.0.8 身份行（自 README 迁入的快照表）**

| 版本 | 身份 |
|---|---|
| v0.0.9 artifact | Synchronized reference/network prune; the build sidecar records the actual source-content closure separately from its build-time base HEAD; `.com` 1,233,236 B; SHA-256 `d4f7d3022a373fb71ad46dde23c72fe450beefad045727122383c85da17c678b` |
| Published v0.0.8 | `10672e3`; unsigned `.com` 5,388,402 B; SHA-256 `948232f00028170d2090983375fbca2a3829ef8f73235baada5deb9db174d737` |

v0.0.9 时代的说明（单次绑定计时、平台范围 d4f7d302、Windows 签名顺延句、有限域检查边界）已随 README 迁入块移至 [archive/prd-history-20260929.md](archive/prd-history-20260929.md#readme-migrated)。


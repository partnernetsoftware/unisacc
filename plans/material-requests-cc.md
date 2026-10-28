# cc 材料请求（2026-10-28 17:4x；fable 裁定 D：只写 plans/，由材料所有者执行）

## 1. O1 单流容器（关 e4.in.o1-model、e4.pos.o1-save-restore-to-mov）

- 事实：riroot 源包 `out/c43-source-riroot-full-pc-03/compiler.pkg`（4d9b8a3a）已含 `lnx/x86_64/image/O1` 路线，e4 流单元模型索引 6（O2 路线为 7），其余单元与 O2 同索引。即 O1 模型已在包内，只缺打包路线。
- 阻塞：`tools/cxlib/streampack.cx` 第 27 行硬编码 `route = "%s/image/O2"`，只打 O2。
- 2026-10-28 18:2x 更新：fable 裁定 streampack 不在 cx 结构化预留内且请求单超时，cc 已自行实施（可选第 5 参 LEVEL，默认 O2 字节等价：七容器 sha 回归一致；O1 容器已打成 out/c43-streams-cc-o1-01，e4 模型索引 6），待 cdx 复核。原请求：给 `stream_pack` 加可选第 5 参 LEVEL（`O0|O1|O2`，默认 O2，拼入 route），`NAMES` 不变；O1 路线无 `tapecheck` 要求差异（包内 O1 同样含 tapecheck，可照旧跳过）。输出到新目录 `out/c43-streams-riroot-o1-01/`，receipt `selected_route` 为 `lnx/x86_64/image/O1`。
- 输入身份：BASE_COM = 与 riroot-01 相同的 stream-runtime 基座（receipt base_sha256 03d00cfc…），CANDIDATE_PKG = 4d9b8a3a（或当前准入候选内嵌 P3，若换则两份身份都记）。
- 验收：`tools/stream-model-identity.cx <新目录> <包> all` 全 SAME；stream4(O1) 对 `imm r2, 0 / add64 r0, r1, r2` 不折叠（O1 不含 O2 折叠）而对软件栈 save/restore 折叠为 mov；oracle = REF `-O1 -S` tape（tools/o2-cases-gen.cx 可扩一组 `o1`）。deadline：下一慢车道纪元前；每条命令 ≤55 s。

## 2. 库路线材料（关 prune.pos.library-exports、prune.ref.library-signatures）

- 事实：prune 库模式由资源 `\0library/signatures`（E3 的 USLSIG1/USLSIG2 字节）触发（exec/prune/libraryroots.md:3），无资源时输出不变（:21）；拒绝文本 `not covered: malformed library signatures or missing public label`、`not covered: typed library signature`（exec/prune/prune-result.tsv:1014、:407）。
- 阻塞：流容器 CLI（tools/stream-runtime.c）只接 INPUT/INC，无注入资源的入口；产品侧库模式入口拼写未在 cc 侧核实。
- 请求（所有者：stream-runtime / 产品 CLI 主人）：(a) 容器增加 `UNISA_RESOURCE_FILE=<path>` 或第 4 参，把文件字节作为 `\0library/signatures` 资源供给；或 (b) 给出产品的库模式拼写（`-flib`? `-shared`?）与一份最小 USLSIG1 夹具（含 1 个公共标签、1 个类型化签名、1 个畸形头）。
- 验收：三条夹具经 stream5 输出分别为"导出根保留（dead 不剪）"、"typed library signature 拒绝"、"malformed… 拒绝"，rc≠0 且 stdout 空；预期文本以上述两行为准。cc 收到材料后在 tools/prune-cases-gen.cx 加 `library` 组，一个窗口槽 ≤40 s。

### 2. 库路线 — 18:3x 事实补充（只读核对，未实跑）

- 产品 CLI 不暴露库模式（exec/c/compiler.c 无 `library/symbols`/`library/signatures`/USLTAPE1 字样）；run.c 的 env 钩子只有 UNISA_ATTRIBUTES / UNISA_MAXSTEPS / UNISA_CONTAINER / UNISACC_RF_CAPTURE / UNISACC_RR_TIME，无通用资源注入。
- 链路（exec/parse2/libraryexports.md、exec/c/librarysignature-v2.md）：E3 在资源 `\0library/symbols`=LE64 1 时输出 `USLTAPE1`（tape + 模型生成的 USLSIG2）；prune 在资源 `\0library/signatures` 下保留公共定义为根。故库材料 = stream3 带 symbols 资源 → 拆 USLTAPE1 → stream5 带 signatures 资源。
- 所需改动（超出 cc 轻活槽，列为请求）：tools/stream-runtime.c 加 `UNISA_STREAM_RESOURCE_<NAME>=<path>`（或两个专用 env）填 ri[4..5]；一个 USLTAPE1 拆分小工具（.cx，按 v2 文档 LE64 框架）；畸形签名夹具按 libraryroots.md 规则构造（截断、defined≠1、重名）。cc 可在所有者准予后一次窗口槽完成（基座重编 7 s + 重打包 + ≤40 s 用例）。

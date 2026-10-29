# csih C→CX 短结（csr /goal，2026-10-29）

路径：`apps/csih/docs/c-to-cx-report.md`  
结论：产品树 0 个 `.c`；上层只 `#pragma unisa module`/`use`，下层 libc/POSIX 才 `#include`；`check.cx -- modules|loop|probes-{tui,libc,net,e2e}` 与 `check.sh` 本机绿。冷 `-run csih.cx` 每次约 47s 且 run-cache 不命中，e2e/日常自测必须落盘 ELF（`CSIH_BIN`/`out/csih-e2e-bin/csih`）才进得了 60s。未重打产品 `.com`，未 Draft/Latest/签/公开。

## 1）经验

好使：

- 广度优先：先根入口 `csih.cx` + 分层合同，再 TDD 分桶（modules / loop / probes-*），避免一次啃整棵 TUI。
- 下层独占 `#include`，上层只 `use`；CXIF 导出类型与非 static 函数（枚举进导出结构，不靠宏）。
- `setjmp` 必须落在守卫帧（`rc_guard`），包一层 `rc_setjmp` 会 UB/segfault。
- 探针与驱动一律 `unisacc.com -run ENTRY.cx --`；套件分桶后单桶可进 60s。
- e2e 证据：`sh $UNISACC csih.cx -o out/csih-e2e-bin/csih`，再 `CSIH_BIN=…` 跑 `probes/tui_e2e.cx`（首帧毫秒级）。

踩坑：

- 冷 `-run` 大图每次全编；`UNISACC_RUN_CACHE` 对 CXIF 临时头路径 `lookup_hit=0`，暖机无效。嵌套 e2e（check 再 `-run` 探针再 `-run` csih）必超时。
- CXIF 禁止条件导出：类型不能包在 `#ifndef HEADER` 里给 cc fuzz 复用；cc 路径要分 TU（库用 `cx.h` 前奏，驱动 `-include *.h`）。
- `use` 闭包有上限（约 63）；切模块时要数边，勿把大杂烩塞进一个 stem。
- 探针字符串/argv 表、文档里残留 `.c` 路径会假红；迁移要连夹具表一起改。
- 删除走统一 guard（`tools/cxbin` + `tools/safedel.cx`）；旧 `tests/cxbin` 已归档，硬编码会炸。
- 环境变量污染（如 `UNISACC_RR_TIME=1`）会把 RR_TIME 行灌进断言输出，假失败。

范式 / 边界怎么切：

- stem = 模块名；`use` 相对消费者；库无无 main，CLI/`_cli.cx` 或根 `csih.cx` 持 `main`。
- I/O、终端、socket、safedel 等留下层可 include；渲染/会话/agent/TUI 只 use。
- 检查器分相：modules（单模块 selftest）→ loop（组合）→ probes 按成本分桶；重活（e2e）独立桶并要求预构建产物。

## 2）缺／该改进的工具与脚手架（优先级）

P0：

- CX `-run` 跨进程复用：内容寻址的单元/图缓存，键勿绑 `/tmp/ua-cxif-*` 临时路径；否则大应用永远冷启动。
- 正式 `CSIH_BIN`／准备步：源摘要变则重建 `out/…/csih`，check 缺产物时具名拒绝或单次有界构建，避免隐式 47s。

P1：

- 迁移脚手架：`.c`→`.cx` 批量改名、`#include`→`use` 候选、残留 `.c` 字符串扫描、下层 include 白名单审计一条命令出报告。
- `check.cx` 门禁：与 `tests/gate.sh` 风格对齐的滚动队列（分桶、输入哈希、禁止复用旧绿）；嵌套超时与直跑证据分流记账。
- cc fuzz 与 CX 双轨：一键「库 TU + cx.h / 驱动 TU + API.h」配方，写入文档，避免再踩条件导出。

P2：

- CXIF／检查器：导出面 diff（类型、枚举、非 static）、`use` 闭包计数与超限诊断、禁止上层 include 的静态闸。
- e2e 减 Python 依赖：pty 见证逐步 CX 化或最小宿主助手；safedel 调用统一从 apps 侧封装。
- 模块图可视：`-cx-plan` 输出进可读摘要（边数、下层/上层着色），方便切边界。

P3：

- 测套加速：热点模块的 native 缓存与产品 `-run` 合同分离（测用 ELF，产品仍默认 `-run`）。
- 文档合同：`self-iter.md` 等仍写旧 `.c` 路径处随手清；不扩 scope 的话可挂债。

## 本机可日用的前置（2026-10-29 落地，P0 最小集）

- 准备：`sh apps/csih/csih.sh prepare`。源（`*.cx *.h`）摘要写在 `out/csih-e2e-bin/csih.src`；摘要未变秒返，变了才有界（300s）重建 `out/csih-e2e-bin/csih`，失败看同目录 `build.err`。
- 日常：`sh apps/csih/csih.sh`（默认 agent）或 `csih.sh run` 直接用已备且摘要匹配的 ELF，秒开 TUI。产物缺或陈旧时具名报错并提示 prepare，不再静默冷 `-run` 约 47s；确要冷跑设 `CSIH_COLD=1`。`CSIH_BIN` 仍可覆盖。
- 密钥合同：`DEEPSEEK_API_KEY` 优先；否则读 `~/env.jsonl` 中含 `"deepseek"` 的首行的 `"api_key"`，没有则 `"key"`。`csih.sh keyprobe` 只报长度与来源，不打印密钥。模型默认 `deepseek-chat`，`CSIH_MODEL` 覆盖。
- 首次 agent 自建 `~/.cdsh/`（之前缺目录时报 cannot write transcript）。
- Ctrl-C：有输入清输入；输入空且不忙时同时清结果/错误行，提示为「清空输入/结果」。
- 实窗证据：tmux 中 agent 真打 DeepSeek，write→read→sh(cat) 三步成功。遗留：旧式工具完成缺结构化 claims，本轮仍判「未验证」（门禁合同，不在本轮改）。工作目录为 `apps/csih`（csih.sh 会 cd 过去）。

# cc 定时巡检（每 5 分钟一轮）

定时器每次只发一句话："按 plans/cron-cc.md 执行一轮"。要改巡检内容就改这个文件，不改定时器。

## 我是谁、管什么

- cc 负责三件事：指导 grk 开发 unisacc.com 0.0.43；维护 L0（cx/sys、cx/mem、cx/text）和开发工具（tools/）；清理、归档和梳理代码与文档。
- 别人的车道，只发信封，不动文件：
  - **grk**：编译器和产品，即 src/、cx/compiler、cx/driver、cx/core、cx/runtime、cx/cx0、cx/workspace；用 `mux envelope 1:grk`。
  - **bdy**：用 Rust 的思路改进 cx 架构，即 mem.grow、text.build、plans/cx-layers.md；用 `mux envelope bdy`。
  - **csih**：apps/csih/ 下的全部文件；用 SendMessage 发给 unisacc-00。
  - **ujs/**：属于论文 B，unisacc 这边不碰。

## 每轮步骤

1. **看变化**：`git log -6 --format='%h %ar %s'`，再用 `mux capture 1:grk` 看 grk 最后几行。
2. **跑棘轮**：`sh ./unisacc.com tools/ratchets.cx`，五项约 1 秒。
   - 超基线的行如果来自 HEAD：发信封给这个文件所在车道的主人。
   - 如果只出现在工作区（别人还没提交的改动）：不管。
3. **判断要不要出手**，只在以下情况动作：
   - grk 空闲，或者停在某一刀上：给它下一刀，并附上数据和该用的工具。
   - 有人违规，比如越车道改文件、提交前没实编、棘轮上升：指出来，请它撤回或修正。
   - 有人请求 cc 做事，比如补 L0 接口或复核：做，或者派子代理去做。
   - 发现重复的人工劳动：造一个工具，见下面"造工具"。
4. **没有新事实就收尾**：没有新提交、没有新阻塞、也没有可推进的动作时，只回一句话，不重复检查，不重复汇报。

## 当前背景（随进展更新）

- **产品层**：src/ 和 cx/ 里剩下的裸 libc 迁移卡在 closure-once 上：每个产品对象都 use mem.heap 时，`mm_*` 会重复定义。这是 grk 的事。
- **等下一次慢车道安装**（每周一次，或主人点名）：模块去重、每个单元 48 条 use、间接原型报错。验收用 `sh ./unisacc.com tools/check-install-fixes.cx -- 候选路径`，4 个用例。装好以后：
  1. 用 `tools/register.cx` 把这个检查登记成批次；
  2. 通知 csih 和 bdy；
  3. 重跑 refactor 第 5 轮，处理 tools/cxlib 那 204 处。
- **目标状态**：
  - 范围内 py 和 mjs 都是 0；
  - 只有底层用 #include（cxinclude 为 0）；
  - 上层直接调用 libc/POSIX 的次数只降不升（cxlibc，基线 23462）。

## 规则

- **提交**：只提交自己的文件。多人同改的文件（tests/batches.tsv、tests/coverage.tsv、prd.md 等）用 `tools/commit-own.cx --keyed`，提交前可以先用 `tools/own-diff.cx` 看一眼。
- **技术细节**：自己裁定，不问主人。只有对外发布、删除别人的成果、越出车道这几类事才问。
- **造工具**：同一类事要做第二遍，或者涉及十处以上时，就用 `.cx` 造一个工具。工具放进 `tools/`，配 `check-*.cx`，用 `tools/register.cx` 登记批次，再在 prd.md 能力速查里补一行。
- **独立复核**：在干净快照里跑，用 `tools/snapshot-run.cx`，不在共享工作区里跑。
- **期限**：每条命令不超过 60 秒；长任务放后台或交给子代理。

## 常用工具（详见 prd.md 能力速查）

- 查找：`refactor.cx sites`；`symindex.cx`，支持 calls、def、uses、users、undeclared、external。
- 改写：`refactor.cx`，支持 find、apply、sweep、rename；规则文件在 `tools/refactor/*.tsv`。
- 影响面：`affected.cx --files F --run -j 3`。
- 守卫：`ratchets.cx`、`own-diff.cx`、`commit-own.cx`、`doc-links.cx`、`tape-gap.cx`。
- 登记与复核：`register.cx`、`snapshot-run.cx`、`check-install-fixes.cx`。

# cc 每 5 分钟巡检：督导 grk 把 0.0.43 推到可发布

定时器只发一句"按 plans/cron-cc.md 执行一轮"。要改巡检内容，就改这个文件。

## 目标

grk 主刀，尽快做出可发布的 0.0.43 `unisacc.com`。计划、链路和进度以 [release-0.0.43.md](release-0.0.43.md) 为准。

发布链当前的步骤：hostnative 变薄 → `tools/release-build.cx`（P3、五片、两代逐字节）→ 慢车道安装（`tools/check-install-fixes.cx` 四项全绿）→ 同一候选在 CI 上跑 win/osx/arm64 → `tests/run.cx -- --all` 为 QUALIFIED → 主人授权。

## 每轮

1. **看 grk**：
   - 看 `git log -6`，看 grk 暂存了什么、窗口里在做什么。
   - 它在不在发布链的当前步骤上？卡在哪？
2. **推 grk**：
   - 不在当前步骤上，当轮拉回来。
   - 卡住了，cc 当轮动手疏通（L0、工具、测试缺口）。
   - 两轮没进展，就在它的窗口里要具体结果：哪一步、哪个提交、什么时候。
3. **审提交**：grk 的每个新提交都按范式审：
   - 差异写成模型步（表、网、流），不在驱动或宿主代码里写特判；
   - 平台差异只放进 L0 或表里；
   - 不回退成传统 C 结构；
   - 棘轮（`tools/ratchets.cx`）不变红。
   发现问题，当轮告诉 grk。
4. **量**：跑 `tools/release-board.cx`，只看 RELEASE 行和 OWNER 行的变化。
5. **报**：四件事，各一行：
   - grk 推进到哪一步；
   - 本轮新提交；
   - 卡在哪；
   - RELEASE 数字。

有空时，cc 补自己名下的格（ri、resources、negative、library、interfaces、properties），但不能挡住上面这几步。

## 清理与归档（督导之外的空档做，每轮最多一小件）

目的是让仓库只剩"事实表 → 生成器（tools/）→ weights/、kernel/ → 产品（src/ + cx/）"这条主线，少些干扰。

- **找对象**：
  - 用 `tools/stale.cx` 找没有读者的文件。
  - 用 `tools/symindex.cx`、`refactor.cx sites` 查引用。
  - 用 `tools/doc-links.cx` 查断链。
  - 不凭印象删东西。
- **当前清单**（做完一项就划掉并写上提交号）：
  1. 矩阵"合同来源"列和 check 工具里引用 `exec/c/...` 的地方，批量改指到 `cx/runtime`、`src` 里的现行实现。全部改完以后，把 `exec/c` 挪进 `archive/`。
  2. `exec/` 顶层的 `model*-manifest/result/byte/fresh.tsv`、`layoutprovenance-*` 这类旧门禁产物，确认没有读者后归档。
  3. 按现状重写 `exec/README.md`：它还在描述已经不存在的 gen.py/check.py。
  4. `plans/` 里过期的计划，用 `plans/README.md` 判断哪些是当前的，过期的归到 `archive/`。
- **怎么做**：
  - 用 `git mv` 原路径挪进 `archive/`，不直接删。
  - 挪完跑 `doc-links --fix-moved`，但只修活跃区，不改 archive 里的旧文档。
  - 跑 `tools/ratchets.cx`，再跑 affected 批次，全绿再提交。
  - 一件事一个提交，提交说明写清依据，比如"0 个读者"。
- **不碰**：
  - 别的车道：src/ 和产品层归 grk，cx/* 归 bdy，apps/csih/ 归 csih，ujs/ 不碰。
  - 正在构造或测试中用到的输入。
  - 还被矩阵证据引用的文件：先改引用，再挪文件。
  - 别的车道里该清理的东西，发信封给它的主人。

## 底线

- 只提交自己的文件。多人共改的文件用 `tools/commit-own.cx --keyed` 提交。
- 别的车道发信封，不动它们的文件：
  - grk 负责 src/ 和 cx 产品层；
  - bdy 专做 cx 核心底层和上层封装，不在发布板上，不派它发布格；
  - csih 负责 apps/csih/；
  - ujs/ 不碰。
- 技术细节自己定，不问主人。工期以实测为准。
- 被阻塞时说一次就停，不重复报同一个状态。
- 每条命令 60 秒以内。工具清单见 prd.md 能力速查。

# /tmp 与 ~/.cache/unisacc 遗留目录及疑似挂死进程盘点(2026-11-24)

采样时刻: 2026-11-24 18:13-18:17 +08:00。只读测量(未删任何目录或文件、未终止任何进程)。
主机磁盘现状(`df -h`): `/dev/vda2` 总计 250G,已用 201G,可用 50G(使用率 81%)。`/tmp` 整目录 `du` 总计约 24G(`24559672 KiB`),`~/.cache/unisacc` 整目录总计约 47G(`48502552 KiB`)。

注:部分 `git clone --local` 临时目录之间共享硬链接,整组去重(`du -skc`)与逐目录独立统计(`du -sh`)会存在差异;表内列出独立 `du -sh` 大小并附去重说明,汇总同时给出两套数字。

## 1. /tmp 下前 30 大目录

| 序号 | 目录 | 大小(du -sh) | 独立KiB | 最后修改时间 | 属主窗口猜测(依据) | 进程占用(lsof +D / ps) | 判断 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `/tmp/unisacc-session-check-prerun-drive` | 2.1G | 2187104 | 2026-11-24 18:14:21 | agy-flow / mutation-gate(`check-prerun-drive` 会话) | lsof 采样 NONE;ps 实见 PID 1439380/1439565 正在写入 | 保留 |
| 2 | `/tmp/mg-g24-noret` | 1.9G | 1938904 | 2026-11-24 18:07:21 | agy-flow(`mutation-gate --out /tmp/mg-g24-noret`) | lsof 采样 NONE;ps 实见 PID 1190808 及 8 个 worker 运行中 | 保留 |
| 3 | `/tmp/grk47redo` | 1.4G(独1.7G) | 1754208 | 2026-11-24 18:13:27 | ccogweco / omp-grk(`omp-grk47` 重放工作树) | IN_USE(bash:1411066,tail:1427285,timeout:1427284,unisacc-9:1427286) | 保留 |
| 4 | `/tmp/unisacc-session-cdx-r24-x7W1PK` | 1.3G | 1280784 | 2026-11-24 17:21:31 | cdx(R24 取证会话,见 `prd_mempalace.md` R24 条) | NONE | 可删 |
| 5 | `/tmp/dvgit-68ea2464-ab` | 985M | 1007668 | 2026-11-21 20:31:46 | agyneon / cc(`tools/check-devsvc-git.cx` 夹具残留) | NONE | 可删 |
| 6 | `/tmp/commit-own-968568b4c968e332` | 665M | 681404 | 2026-11-24 18:06:41 | devsvc / `commit-own.cx`(本仓私有索引工作树缓存) | NONE(18:06 刚被提交触发刷新) | 保留 |
| 7 | `/tmp/cdx-cxlib043-O3mpZKcp` | 491M | 502072 | 2026-11-24 13:59:37 | cdx(`cxlib` 0.4.3 检查临时树) | NONE | 可删 |
| 8 | `/tmp/unisacc-session-release-flow` | 487M | 497988 | 2026-11-24 15:06:02 | cc / 发布车道(`release-flow.cx`,0.0.105 发布时段) | NONE | 待确认 |
| 9 | `/tmp/unisacc-premerge-union` | 338M | 345136 | 2026-11-24 18:10:41 | agy-flow / 集成列车(`premerge-union.cx` 复用树) | NONE(18:10 刚刷新) | 保留 |
| 10 | `/tmp/cdx-fwdstub-edit-RQFl9fvb` | 321M | 328324 | 2026-11-24 14:54:37 | cdx(0.0.122 转发桩编辑临时树) | NONE | 可删 |
| 11 | `/tmp/check-commit-own-341972` | 1.1G | 1089328 | 2026-11-24 17:13:23 | agy-flow / devsvc(`check-commit-own` 夹具克隆残留) | NONE | 可删 |
| 12 | `/tmp/unisacc-session-cdx-s1-LA14YQ` | 292M | 298692 | 2026-11-24 18:13:28 | cdx(S1 `register` 私有索引验证会话) | NONE(18:13 刚修改) | 待确认 |
| 13 | `/tmp/check-commit-own-371208` | 292M | 298048 | 2026-11-24 17:16:10 | agy-flow / devsvc(`check-commit-own` 夹具残留) | NONE | 可删 |
| 14 | `/tmp/check-commit-own-2091480` | 291M | 297720 | 2026-11-24 14:05:36 | agy-flow / devsvc(`check-commit-own` 夹具残留) | NONE | 可删 |
| 15 | `/tmp/unisacc-qualify-934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1-414a024ea5e52462b22e7ed9d11f6c44b1d6c2e8` | 291M | 297172 | 2026-11-24 15:10:49 | agy-flow / dvqualify(0.0.105 冻结 `414a024ea5` 资格树) | NONE | 可删 |
| 16 | `/tmp/unisacc-qualify-934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1-21c6f644419dfa1ee775d654ea7969a9dcce8d6e` | 291M | 297104 | 2026-11-24 14:47:25 | agy-flow / dvqualify(历史提交资格检查树) | NONE | 可删 |
| 17 | `/tmp/unisacc-qualify-934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1-8e7f71d8c653f8c7bde1aa5c191f6b21f3e800c1` | 291M | 297088 | 2026-11-24 14:37:43 | agy-flow / dvqualify(历史提交资格检查树) | NONE | 可删 |
| 18 | `/tmp/check-commit-own-2106938` | 290M | 296900 | 2026-11-24 14:07:56 | agy-flow / devsvc(`check-commit-own` 夹具残留) | NONE | 可删 |
| 19 | `/tmp/pcfck-fx-trunc` | 284M | 290640 | 2026-11-24 17:35:52 | agy-flow / ccogweco(`check-precommit-flow.cx` 夹具) | NONE | 可删 |
| 20 | `/tmp/pcfck-fx-r2` | 284M | 290640 | 2026-11-24 17:35:52 | agy-flow / ccogweco(`check-precommit-flow.cx` 夹具) | NONE | 可删 |
| 21 | `/tmp/pcfck-fx-r1` | 284M | 290636 | 2026-11-24 17:35:52 | agy-flow / ccogweco(`check-precommit-flow.cx` 夹具) | NONE | 可删 |
| 22 | `/tmp/pcfck-fx-pass` | 284M | 290624 | 2026-11-24 17:35:52 | agy-flow / ccogweco(`check-precommit-flow.cx` 夹具) | NONE | 可删 |
| 23 | `/tmp/fxdbg3` | 284M | 290544 | 2026-11-24 17:35:05 | agy-flow / ccogweco(precommit-flow 调试克隆副本) | NONE | 可删 |
| 24 | `/tmp/fxdbg` | 284M | 290468 | 2026-11-24 17:29:59 | agy-flow / ccogweco(precommit-flow 调试克隆副本) | NONE | 可删 |
| 25 | `/tmp/fxdbg2` | 284M | 290452 | 2026-11-24 17:34:41 | agy-flow / ccogweco(precommit-flow 调试克隆副本) | NONE | 可删 |
| 26 | `/tmp/headchk` | 284M | 290332 | 2026-11-24 15:53:50 | cc / gatekeeper(HEAD 私有校验克隆副本) | NONE | 可删 |
| 27 | `/tmp/unisacc-privrepo-pmuq1f5u` | 283M | 288972 | 2026-11-24 14:50:07 | agy-flow(`privrepo.cx` / `premerge-selfcheck` 残留) | NONE | 可删 |
| 28 | `/tmp/e7api` | 282M | 288636 | 2026-11-24 14:40:08 | agyneon / cc(E7 API 私有校验克隆副本) | NONE | 可删 |
| 29 | `/tmp/dvgit-5aecbc72-f` | 282M | 287848 | 2026-11-24 13:49:25 | agyneon(`tools/check-devsvc-git.cx` 夹具残留) | NONE | 可删 |
| 30 | `/tmp/dvgit-868b6a1b-a` | 281M | 287660 | 2026-11-24 14:03:02 | agyneon(`tools/check-devsvc-git.cx` 夹具残留) | NONE | 可删 |

补充说明(前 30 名外被点名模式): `/tmp/r24`、`/tmp/r26`、`/tmp/s0dir` 当前均已不存在; `/tmp/mg-*` 除第 2 名 `mg-g24-noret` 外,另有已结束或小体积残留 `mg-r15b`(236M)、`mg-g24-final`(856K)、`mg-enospc-r3`(824K)、`mg-g24`(808K)、`mg-v102-v2`(276K)、`mg-r15`(236K),不在前 30 统计内。

## 2. ~/.cache/unisacc 下前 15 大目录

| 序号 | 目录 | 大小(du -sh) | 大小(KiB) | 最后修改时间 | 属主窗口猜测(依据) | 进程占用(lsof +D) | 判断 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `~/.cache/unisacc/chains` | 17G | 17087648 | 2026-11-24 17:25:12 | cc / 发布列车(含 `v0.0.105` 1.1G、`bg-b1` 13G、`snap` 2.8G) | NONE | 待确认 |
| 2 | `~/.cache/unisacc/symindex` | 9.0G | 9321428 | 2026-11-24 18:08:41 | devsvc / `symindex.cx`(共用符号索引缓存) | NONE(18:08 刚更新) | 保留 |
| 3 | `~/.cache/unisacc/check-devsvc-core` | 7.5G | 7797212 | 2026-11-24 14:19:29 | agyneon(`check-devsvc-core.cx` 测试工作树缓存) | NONE | 可删 |
| 4 | `~/.cache/unisacc/tlib` | 6.3G | 6559880 | 2026-11-22 00:55:59 | 发布/构建(tlib 历史产物缓存,11-22 起未更新) | NONE | 待确认 |
| 5 | `~/.cache/unisacc/e3cache` | 3.7G | 3882384 | 2026-11-24 18:15:05 | unisacc 前端 e3 热缓存(全仓共用) | NONE(18:15 刚更新) | 保留 |
| 6 | `~/.cache/unisacc/check-precommit-gates` | 2.5G | 2608936 | 2026-11-24 16:31:45 | agy-flow(`check-precommit-gates.cx` 夹具缓存) | NONE | 可删 |
| 7 | `~/.cache/unisacc/devsvc` | 548M | 561132 | 2026-11-24 18:02:12 | devsvc(常驻服务 PID 805320 状态、事件与 commit jobs) | NONE(服务活跃) | 保留 |
| 8 | `~/.cache/unisacc/release-v0105-state` | 340M | 347428 | 2026-11-24 15:22:44 | cc / 发布列车(0.0.105 发布状态,今日刚公开须留 7 天) | NONE | 保留 |
| 9 | `~/.cache/unisacc/derived` | 225M | 230100 | 2026-11-22 00:11:56 | `derive.cx` 旧派生缓存(现已用 `/tmp/unisacc-derive-cache`) | NONE | 可删 |
| 10 | `~/.cache/unisacc/prep-cache` | 57M | 58112 | 2026-11-24 17:58:31 | agy-flow / `release-prep`(含 0.0.101 `old-product-cache`) | NONE(记忆宫殿登记) | 保留 |
| 11 | `~/.cache/unisacc/pf-snap-tools` | 12M | 11640 | 2026-11-24 16:12:36 | agy-flow(`chain-preflight` 工具快照) | NONE | 保留 |
| 12 | `~/.cache/unisacc/cx-dev` | 2.4M | 2356 | 2026-11-24 07:51:46 | `cx-dev`(本地开发重建缓存) | NONE | 可删 |
| 13 | `~/.cache/unisacc/.managers` | 504K | 504 | 2026-11-21 09:46:26 | 管理器锁与状态目录 | NONE | 待确认 |
| 14 | `~/.cache/unisacc/auto-continue` | 72K | 72 | 2026-11-24 18:15:09 | `auto-continue` 状态(18:15 刚更新) | NONE | 保留 |
| 15 | `~/.cache/unisacc/prerun` | 0 | 0 | 2026-11-24 18:00:16 | `prerun-drive` 空目录占位 | NONE | 保留 |

注: `~/.cache/unisacc/chains` 按 `prd_mempalace.md` 规则须保留 `v0.0.105`(1.1G, 发布后留 7 天);其余 `bg-b1`(13G) 与 `snap`(2.8G, 记忆宫殿要求 Root 升级后移走旧快照) 确认后可释放约 15.8G。

## 3. 运行超过 2 小时且 CPU 低于 1% 的疑似挂死/遗留进程

筛选条件: `ps -eo pid,ppid,pgid,etimes,pcpu,user,stat,args`, `USER=devali`, `etimes > 7200`, `%CPU < 1.0`,排除正常交互壳(`-bash`/`tmux`/`mosh-server`)、代理本身(`claude`/`codex`/`hermes`)、系统基础设施(`systemd`/`dbus`/`rclone`/`podman`/`mux`/`mapp-http`/`ndtsv`/`gost`/`llm-proxy`/`procguard`/`cxsa-engine`)。

### (1) unisacc 测试直接挂死进程(共 12 个)

| PID | PPID(父进程) | PGID | 年龄(etimes / 时长) | %CPU | STAT | 命令 | 判断 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2098657 | 1 | 2098657 | 108037s (30.0h) | 0.0 | Ss | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` | 可杀(挂死) |
| 2098658 | 2098657 | 2098657 | 108037s (30.0h) | 0.0 | S | `tools/check-l0-capture.cx` | 可杀(挂死) |
| 2099254 | 2098658 | 2099254 | 108032s (30.0h) | 0.0 | Z | `[sh] <defunct>` | 随父清理 |
| 3028004 | 1 | 3028004 | 101225s (28.1h) | 0.0 | Ss | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` | 可杀(挂死) |
| 3028005 | 3028004 | 3028004 | 101225s (28.1h) | 0.0 | S | `tools/check-l0-capture.cx` | 可杀(挂死) |
| 3028808 | 3028005 | 3028808 | 101220s (28.1h) | 0.0 | Z | `[sh] <defunct>` | 随父清理 |
| 3254849 | 1 | 3254849 | 100261s (27.8h) | 0.0 | Ss | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` | 可杀(挂死) |
| 3254853 | 3254849 | 3254849 | 100261s (27.8h) | 0.0 | S | `tools/check-l0-capture.cx` | 可杀(挂死) |
| 3255368 | 3254853 | 3255368 | 100257s (27.8h) | 0.0 | S | `sh -c head -c 262144 /dev/zero >&2; printf 'out\n'` | 可杀(挂死) |
| 3255369 | 3255368 | 3255368 | 100257s (27.8h) | 0.0 | S | `head -c 262144 /dev/zero` | 可杀(挂死) |
| 2417764 | 1 | 2417762 | 13766s (3.8h) | 0.0 | SN | `script -qec ... tools/check-cx-sys-net.cx -- child tty /tmp/cx-sys-net-zhoiir/t.tty /dev/null` | 可杀(挂死) |
| 2417765 | 2417764 | 2417765 | 13766s (3.8h) | 0.0 | ZNs | `[check-cx-sys-ne] <defunct>` | 随父清理 |

### (2) 其它跨会话/ujs/moltbaby 遗留或挂死进程(共 96 个)

| 类别与数量 | PID / PGID 范围 | PPID(父进程) | 年龄(etimes / 时长) | %CPU | 命令摘要 | 判断 |
| --- | --- | --- | --- | --- | --- | --- |
| `revise-oracle` 服务 (18 个) | 2321472..2321526 (PGID 2321452..2321504) | 1 | 1737083s (20.1d) | 0.0 | `bun /tmp/revise-oracle-XXXXXX/server.ts` | 待确认(高概率可杀) |
| `ujs/practice/uxe` HTTP 服务 (65 个) | 2925615..4189588, 1574..228554 (PGID=PID) | 1 | 598044..607256s (6.9-7.0d) | 0.0 | `python3 -m http.server 8766..8834 --directory .../ujs/practice/uxe` | 待确认(先通知 ujs 车道) |
| `127.0.0.1` 临时 HTTP 服务 (2 个) | 484589, 871200 (PGID=PID) | 1001460 | 593985..596361s (6.9d) | 0.0 | `python3 -m http.server 8765/8876 --bind 127.0.0.1` | 待确认 |
| `probe-strip` 暂停进程 (1 个) | 587132 (PGID 587132) | 1001460 | 1609470s (18.6d) | 0.0 | `bun /tmp/probe-strip/tui-provider4.ts` (STAT=Tsl) | 可杀(已暂停遗留) |
| `ohmy` mock 进程 (1 个) | 706370 (PGID 706370) | 969550 | 1607976s (18.6d) | 0.5 | `bun /tmp/ohmy/.../cli.js --model localmock/flaky-model` | 待确认 |
| `omp-workflow` 探针 (3 个) | 3728805, 3728815, 1265987 | 969550 / 3728805 | 1022728..1039690s (11.8-12.0d) | 0.0-0.6 | `bun omp-workflow/cli.ts` / `/tmp/wfprobe/...` / `/tmp/wf-cursor-probe` | 待确认 |
| `omp-python-runner` 遗留 (1 个) | 3094422 (PGID 3094422) | 3436433 | 100960s (28.0h) | 0.0 | `/usr/bin/python3 -u /tmp/omp-python-runner-1001/runner-3b4uxubuapgfi.py` | 待确认 |
| `port-pids-probe.cx` 遗留 (3 个) | 1270176, 1270177, 2078330 | 1 | 237384..267487s (65.9-74.3h) | 0.0 | `moltbaby/bin/lib/port-pids-probe.cx` 及 `head -2` | 待确认 |
| 孤立僵尸进程 (2 个) | 3365359, 1069520 | 3359865, 2031154 | 19658..546958s | 0.0 | `[chrome] <defunct>`, `[git] <defunct>` | 不可单杀(须由父进程回收) |

## 4. 汇总

(1) 确定 `可删` 目录合计:
- `/tmp` 前 30 大中的 23 项 `可删` 目录: 去重实际可释放 `8.29 GiB` (`8691920 KiB`, 独立累加 `9.03 GiB` / `9469660 KiB`)。
- `~/.cache/unisacc` 前 15 大中的 4 项 `可删` 目录: `10.15 GiB` (`10638604 KiB`)。
- 两处确定 `可删` 合计: 去重实省 `18.44 GiB` (`19330524 KiB`, 独立累加 `19.18 GiB` / `20108264 KiB`)。

(2) `待确认` 潜在可释放空间:
- `/tmp` 待确认 2 项(`unisacc-session-release-flow` 487M + `unisacc-session-cdx-s1-LA14YQ` 292M): 约 `779 MiB`。
- `~/.cache/unisacc/chains` 中除 `v0.0.105`(1.1G 须留 7 天)外的 `bg-b1`(13G) 与 `snap`(2.8G): 约 `15.8 GiB`。
- `~/.cache/unisacc/tlib`: `6.3 GiB`。
- 若全部确认清理, 还可再释放约 `22.9 GiB`。

(3) 疑似挂死/遗留进程数:
- `unisacc` 测试直接挂死进程: `12` 个(`check-l0-capture` 3 组共 10 个 + `check-cx-sys-net` 1 组 2 个)。
- 全用户(`devali`)范围超 2 小时低 CPU 遗留/挂死进程合计: `108` 个(含上述 12 个 + `http.server` 67 个 + `revise-oracle` 18 个 + 其它 `/tmp` 探针与僵尸 11 个)。

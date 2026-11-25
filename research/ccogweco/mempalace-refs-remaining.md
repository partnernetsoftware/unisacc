# prd_mempalace.md 剩余失效引用清单(2026-11-24 复核,只列)

检索: `grep -n '会话临时目录\|\.claude/jobs\|CLAUDE_JOB_DIR' prd_mempalace.md`
上一版: archive/research-2026-11-25/2026-11-24/mempalace-ref-fixlist.md(当时 8 行)。现剩 7 行(43、44 已不再命中)。

| 行 | 现文引用 | 状态 | 建议 |
|---|---|---|---|
| 91 | "证据在会话临时目录,待归档(R5 路线、R4 结构评审)" | 两侧源都已不存在(`~/.claude/jobs/7f46e032/tmp/R4/structure.md`、`R5/devsvc-roadmap.md` 均无,盘上未寻到副本) | 改文为"源已佚(2026-11-24 复核)",不再写"待归档" |
| 99 | "一次性分析脚本仍用 .cx,留会话临时目录不入库" | 规则条文,不是证据指向 | 不改 |
| 108 | "CLAUDE_JOB_DIR/tmp/system-cleanup.patch" | 该作业目录已不存在,research/ 与 plans/ 均无同名归档(只有 archive/research-2026-11-25/2026-11-24/tmp-cleanup-receipt.md,内容是临时目录盘点,不是该补丁) | 改文为"源已佚";若所有者还要该补丁,须从 moltbaby 侧或旧备份另寻 |
| 113 | "证据在会话临时目录,待归档(E4 评审)" | 源不存在(`~/.claude/jobs/7f46e032/tmp/E4` 已无) | 改文为"源已佚" |
| 115 | "候选清单在会话临时目录,待归档(E9 zero-candidates)" | 源不存在(`E9` 已无) | 改文为"源已佚" |
| 135 | "证据 research 待归档,会话临时目录 R24/R26" | R26 已归档 `archive/research-2026-11-25/2026-11-24/R26-ruling.md`;R24 源已不存在(`~/.claude/jobs/7f46e032/tmp/R24/design.md` 无) | 拆开写:R26 指向 research 文件;R24 改"源已佚" |
| 149 | "D7 报告在会话临时目录,待归档" | D7 源不存在;后续修订版已归档 `archive/research-2026-11-25/2026-11-24/D8-report.txt` | 改指 D8-report.txt,并注明 D7 原报告源已佚 |

## 仍可补拷但尚未归档(上一版清单遗留,本次复核)

- `~/.local/share/unisacc/research/cdx/r24-l2/` 11 个文件仍在盘(entry-probe.cx、l2-entry.log、l2-entry-repaired.cov/log、l2-struct.cov、l2-struct.sim.log、module-import.cx、module-tape-resources.tsv、normal-entry.md、struct-call.c、struct-return.c),prd_mempalace 第 44 行已不再命中,但仓内仍无归档;是否补拷待编排者定。

## 已不再命中

- 第 43 行、第 44 行的旧命中(会话临时目录 R25/R27/R30/R31 等)已被现有文字改成 research 文件指向,复核确认无残留。

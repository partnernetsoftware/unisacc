# S5 v3:cdx 二轮阻断三项修复对照(已复跑)

复跑环境:当前 v0.1.3 HEAD 的 git archive 私有树,/tmp/s6/v4;所有命令 `./tools/bound 55` 包装、taskset 12-13;load1 3.58。
补丁:/tmp/ccogweco-docs/research/ccogweco/s5-bare-proc-ratchet-v3.patch(7 文件,不含基线;scrub 0 命中)。
基线不入补丁,入库时:`sh ./unisacc.com tools/check-bare-proc.cx -- --write-baseline --bootstrap $(git rev-parse HEAD)`(首落父基线=HEAD,写入 `# base-parent=<sha>`)。

复跑原行:
- 夹具:`CHECK_CHECK_BARE_PROC	PASS	fail=0`(13 断言)
- ratchets:`RATCHETS	ok=20	fail=0	elapsed_ms=20159`
- 首落 bootstrap:`check-bare-proc: ok: files=37 calls=137 base-parent=9dc43192... baseline written`
- 变异 M8(仅暂存基线上调 ext/sysx/trace.cx 1->2):check `check-bare-proc: ext/sysx/trace.cx has baseline 2, previous 1`;precommit `precommit: BLOCKED ratchet-baseline-raised table=build/bare-proc-baseline.tsv key=ext/sysx/trace.cx col=1 head=1 staged=2 (ratchet baselines only go down)`,`PRECOMMIT staged_cx=0 blocked=2`
- 变异 M9(cx/sys/mem.cx 加一行裸 waitpid):`check-bare-proc: cx/sys/mem.cx has 1 uses and no baseline row`
- 变异 M7(`#define WAIT_RAW wait ## pid` + 调用):探针 files=1 calls=1(修复前 0)

| cdx 阻断 | 修复位置 | 夹具断言 | 变异/实测 |
|---|---|---|---|
| (1) 暂存基线上调不阻塞;清空基线可重走 bootstrap;首落无固定父基线 | `write_baseline()`:文件不存在时才 bootstrap,必须 `--bootstrap COMMIT`(commit 存在且为 HEAD 祖先),写 `# base-parent`;文件存在但无父只允许"非空 + --bootstrap"一次性迁移;注释-only(0 行)拒绝;存在时任何新行/上调 REFUSE。`main()`:check 要求 parent 存在且祖先;HEAD 无基线时要求 parent==HEAD(首落),否则 UNMEASURED 红;precommit 的 `build/*-baseline.tsv` 通用只降门同时生效 | `--write-baseline refuses the increase`;`baseline raised above HEAD fails`;`comment-only baseline cannot re-bootstrap`;`first landing without --bootstrap is refused`;`first landing with --bootstrap HEAD writes the parent` | M8 见原行 |
| (2) `##` 宏别名漏检 | `lx_preproc()`:替换体按"标识符 + ## 拼接"重建名字(wait ## pid -> waitpid)后登记别名;`line_has_call_macro()`:先跳过宏名与参数表,再对 ## 拼接名判 `(` 调用 | `comments/strings skipped, split/alias/##-alias calls, macro and pointer counted`(calls=3 indirect=1 macro=1) | M7 见原行 |
| (3) 丢 sy_dir_next 负值/路径格式化失败;基线坏数与重复行不拒 | `walk()`/`scan_root()`:sy_dir_next()<0 报错且 err_n++,所有 cx_fmt 失败计 err_n,err_n>0 即 FAIL;`parse_rows()`:严格两列(单一 TAB、数字字段 <=9 位)、重复键拒绝、控制字符拒绝;`prev_baseline()` 区分缺失(-1)与损坏(-2) | `malformed baseline is red`;`duplicate baseline row is red` | M11 坏数/重复行均 rc1 |

范围:`tools/cxlib`、`ext`、`cx` 与顶层 `tools/*.cx|.c|.h`(顶层 `check-*.cx` 夹具明确 UNCOVERED,各有批次),精确排除 `cx/sys/proc.cx`。

## 复跑补充(02:12,load1 1.24)

- 新工具过 lint:`CX_LINT files=2 findings=0 elapsed_ms=9`(私有归档树无 git 索引,按 cc 指示单跑;R1/R2/R4 均已处理:argv 前缀 `./tools/bound`,git 调用 bound 10,`has()` 改手写子串扫描避免 R4)。
- 应用补丁的新私有树(v5)全链:夹具 13/13 PASS;`--write-baseline --bootstrap HEAD` 成功写入 base-parent;`RATCHETS ok=20 fail=0 elapsed_ms=21278`。
- 本轮补丁基于当前 v0.1.3 HEAD d45a45894 生成,7 文件,基线不入补丁。

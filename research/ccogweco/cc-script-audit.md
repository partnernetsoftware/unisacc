# cc 起链脚本演练式自审(参数是否被现行源码接受;2026-11-25 ccogweco)

对象:research/ccogweco/cc-chain-day-script.md 的每条命令,以及 pre-chain-crosscheck.md 的门 5/22 条目。
方法:逐条读源码的参数解析与 REFUSE 行,不执行。结论:两条写法与源码不符,其余通过;下面给正确原行。

## 1 不符项(必须改)

### (A) bg-candidate start:`--parallel 4` 位置错

- 现页写法:`bg-candidate.cx -- start --rev R --deadline-s 5400 --parallel 4 --root "$PWD" --base ...`
- 源码:bg-candidate.cx:814-853 只识别 start/status/promote/prune 自己的选项;`--` 之后整体作为
  `extra[]` 原样传给链(819 行 `if (!strcmp(a,"--")) { extra = argv+i+1; ... }`,590-595 行拼进 chain-run argv)。
  `--parallel` 是 chain-run 的选项(chain-run.cx:855,范围 1..4),不是 bg-candidate 的;放在 `--` 前会走
  未知选项/usage 拒绝。
- 正确原行:

      DEVSVC_OWNER=cc sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$(git rev-parse HEAD)" \
          --deadline-s 5400 --root "$PWD" --base "$HOME/.cache/unisacc/chains/bg-b1" \
          -- --parallel 4

- 依据:bg-candidate.cx:814-853、:590-595;chain-run.cx:855。

### (B) chain-ready 不能用多个 --only 串一条命令

- 现页(crosscheck 事实 5):`--only 11 --only 12 --only 12p --only 14 --only 17`
- 源码:chain-ready.cx:1692 `only = argv[++a]`,single 变量,后一个覆盖前一个;结果只跑最后一个门。
- 正确原行:逐门分开跑(或直接跑全量):

      for g in 11 12 12p 14 17; do
        DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$PWD" --root "$PWD" \
            --rev "$REV" --version 0.0.107 --only "$g"
      done

- 依据:chain-ready.cx:1692。

### (C) 空文件时 devsvc f edit 不可用(预防项)

- 现页给了"空文件则用空 old 文件"的备选。f edit 要求旧文本恰好出现一次(devsvc f edit 用法:
  `--all` 才替换多处,默认 exactly one);空 old 在空文件里零宽匹配次数不唯一,会被拒。
- 正确做法:文件为空时改用整文件写:

      DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- f write build/freeze.tsv --from /tmp/freeze-new.txt --expect-sha <空文件 sha>

- 依据:devsvc `f --help` 输出(edit 默认 exactly one;write 支持 --from 与 --expect-sha)。

## 2 通过项(已核源码接受)

| 命令片段 | 源码依据 | 说明 |
|---|---|---|
| `devsvc f edit PATH --old-file A --new-file B` | devsvc f 用法行 | 需 old 恰一次;建议加 `--expect-sha` 防并发改写 |
| `devsvc lock acquire freeze --owner cc --ttl 5400` | devsvc 用法行:`lock acquire NAME --owner O [--ttl S]` | NAME=freeze 与 owner 均被接受 |
| `commit-own -- -m MSG --whole build/freeze.tsv` | commit-own.cx 用法 | 只提交该路径;不推 |
| `devsvc git push FULL_SHA --cwd .` | devsvc 用法行:`git push FULL_SHA... [--cwd WORKTREE]` | 发布列出的提交到当前分支;注意只接受 40 位 |
| `devsvc git commit ... [--push]`(备选) | devsvc git commit 用法行 | 若想一步提交并推,可用它替代 commit-own+push;两者择一 |
| `chain-ready -- --repo --root --rev --version` | chain-ready.cx 主解析 | 全量不加 --fast/--only |
| `--only 22-premerge-union` 单门 | chain-ready.cx:1692 | 单次 --only 合法 |
| `bg-candidate start --rev HEX40 --deadline-s N` | bg-candidate.cx 用法与 :843-853 | --rev 必须 40 位小写 hex;--deadline-s 正整数 |

## 3 演练中还要注意(不改写法,但要在执行时核)

- `devsvc git push` 只推列出的 SHA;冻结行提交的 SHA 必须是当前分支 HEAD,且 main 与 origin/main 在推后同 SHA。
- freeze 锁的 TTL 5400 秒(90 分钟);链止到公开若超过,锁会先过期,建议在 05:30 再 acquire 一次或把 TTL 设到
  覆盖到公开(不改变冻结行 deadline)。
- commit-own 与 devsvc git commit 都受 prepush 冻结检查:107 冻结行已提交后,只有 owner=cc 能推 main;
  非 cc 窗口的 push 会被 PREPUSH_REFUSE 拒,这是预期。

## 4 结论

- 必须改的两条:(A) `--parallel 4` 移到 `--` 之后;(B) 五个 `--only` 拆成五次单门或跑全量。
- 预防项:(C) 空 freeze.tsv 用 f write。
- 其余参数写法与源码一致。修正后请 cc 用第 1 节的原行替换页内对应行;本页不代替执行。

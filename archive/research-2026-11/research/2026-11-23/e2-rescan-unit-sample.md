# 0.0.95 e2 重扫修复:单元层真对拍样本清单(只读,未跑)

清单与账本的持久位置:`~/.local/share/unisacc/research/v095-rescan-review/uec-113/`(与 oracle 包同目录),不依赖 /tmp。
状态:样本已定,未实跑。等负载低于 12 再跑。

## 工具

`tools/unit-e2-compare.cx`(b36a22129e 入库)。同一已装 APE、两个模型包,对每个单元跑
`stream e2 --level 1 --models PKG --target lnx/x86_64 --source F < F`,比 UNIPP2 字节。
判定 SAME / REFUSE_SAME / DIFF / UNMEASURED;清单 sha 入每条账本行,可续跑;`--jobs N` 每 worker 一个分片。

两个 oracle 包(本机,不进仓):

| 包 | sha256 | 字节 |
|---|---|---|
| oracle-original.pkg | 50bf450063e8bd1acc186957c11de23b6a0174dd776e5afce558fc1d553667f8 | 2904461 |
| oracle-patched.pkg | 4ac1180a9fb9cd204222382947e24e510b4ef4cf5b24c289b08699b5295a4b82 | 2904810 |

位置:`~/.local/share/unisacc/research/v095-rescan-review/`。

## 选取规则(确定性)

各层配额:tools 40(仅 tools 顶层 .cx)、cx 25、ext 20、examples 8、tests 7,合计 100。

层内打分 `score = 3*include/use 行 + 3*#define 行 + 5*续行(行尾反斜杠) + 40*含 NUL + 40*末行无换行 + 字节/8192`,
按分数降序取配额的 60%(高分角例),其余 40% 从剩余按等步长跨取(覆盖长尾,不只挑巨文件)。
分数口径对应 e2 重扫最可能触及的四类:深层 #include/use 解析、宏展开量、续行拼接、NUL 与末行无换行边界。

另加 13 个 `tests/pp-equiv-corners/*.c` 全量(include 边角组:backslash-include、continued-include、macro-include、
noeol-case、nul、missing、unreadable、error、depth-overflow、guard-once、cross-header-if、deep-valid、file-path),
它们在 100 之外单列 stratum=corners,合计 113 行。

生成命令(临时脚本,不入库):

```
sh ./unisacc.com <selector.cx> -- . /tmp/uec-100.tsv          # 100 行,分层配额
sh ./unisacc.com tools/unit-e2-compare.cx -- manifest --root . --strata corners --out /tmp/uec-corners13.tsv
cat /tmp/uec-100.tsv /tmp/uec-corners13.tsv | sort > /tmp/uec-113.tsv
```

## 清单身份

| 清单 | 行数 | sha256 |
|---|---|---|
| 100 行(五层配额) | 100 | 93d39e0449906cb2c4898bde0c6f7cad8e7261bc4ddb1e281f3baf17b241c5e8 |
| + corners 13 | 113 | 18de657ebca61bd047be957922e4bf6961985d0882d805c3b3261003a4c51480 |

分层行数:tools 40、cx 25、ext 20、examples 8、tests 7、corners 13。

## 角例覆盖实测(100 行内)

| 特征 | 计数 |
|---|---|
| include/use 行合计 | 792 |
| #define 行合计 | 472 |
| 续行(行尾反斜杠)合计 | 165 |
| 含 NUL 字节的文件 | 0(仓内 .cx 无 NUL;NUL 边界由 corners 的 `nul.c` 覆盖) |
| 末行无换行 | 3(examples/win/posix.c、vmem.c、procsnap.c) |

续行最集中的单元:tools/commit-own.cx 100 处、tools/precommit.cx 27 处、tools/check-cx-fmt.cx 15 处、
tools/check-derive.cx 8 处、tools/check-objabi.cx 6 处、tools/facts-gen.cx 5 处、tools/check-flow-txn.cx 3 处。
include/use 最密集:tools/check-cx-surface.cx 与 tools/check-cx-runtime-surface.cx 各 29 行。
宏最密集:tools/devsvc-core.cx 27 处、tools/check-xunit-fnptr.cx 18 处、tools/facts-gen.cx 17 处、
tools/prerun-drive.cx 16 处。

## 跑法(待负载 < 12)

```
sh ./unisacc.com tools/unit-e2-compare.cx -- run \
  --ape ./unisacc.com \
  --models-a <orig.pkg> --models-b <patched.pkg> \
  --manifest /tmp/uec-113.tsv --ledger /tmp/uec-led \
  --jobs 2 --side-secs 20 --budget-ms 45000
sh ./unisacc.com tools/unit-e2-compare.cx -- status \
  --manifest /tmp/uec-113.tsv --ledger /tmp/uec-led \
  --models-a <orig.pkg> --models-b <patched.pkg>
```

`--jobs 2` 是两条 worker,每条自己按 `--budget-ms` 截断;两片各自回一行 `UEC_RUN ... done= skipped= budget_stop=`,
外层回 `UEC_RUN workers=2 rc=`。总账由 `status` 给 total/same/refuse_same/diff/unmeasured/pending 与 COMPLETE。
DIFF 非 0 立即停,贴首个 DIFF 行的 path 与 detail(rc/out/err/sha)。

单单元 e2 实测约 303ms(小单元);大单元更慢,`--side-secs 20` 是每侧上限。
113 行两路约 113*2*0.3s/2 ≈ 34s 起,大单元拉长,按 55 秒一片切。

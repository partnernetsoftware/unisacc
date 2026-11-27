# 预跑后阅读

未实跑. 行号来自本工作树 tools/prerun-drive.cx. 运行项 UNMEASURED.

## strict-W*.tsv

窗文件名: `<out>/strict-<窗号>.tsv` (tools/prerun-drive.cx:1485). 例: strict-W0000.tsv. 完成戳: `<out>/done.<窗号>` (同文件 1484).

表头 (同文件 1065):

```
# devsvc qualify
batch	domain	status	rc	candidate	ms	load1	retries	note
```

读行规则 (同文件 908-919, 928-940):

(1) 以 `#` 开头的行跳过.
(2) `RAW\t`, `SUMMARY\t`, `batch\t` 跳过.
(3) 数据行至少三列. 第 1 列是批次号, 第 2 列是 domain, 第 3 列是 status. 第 4 列是 rc. 第 9 列是 note.
(4) 同一批次后出现的行覆盖前面的 status. 先 FAIL 再 PASS 时 superseded=1 (同文件 937).

## 哪些状态算 FAIL / UNKNOWN / INVALID

merge 只认第 3 列的字面量 (同文件 2352-2360):

(1) `PASS` 计 PASS.
(2) `FAIL` 计 FAIL. 这是 FAIL.
(3) 既不是 PASS 也不是 FAIL 的, 包括字面量 `UNKNOWN`, 全部进 UNKNOWN 计数. 这是 UNKNOWN.
(4) 源码没有单独的 INVALID 计数. merge 重写的 SUMMARY 把 `INVALID=0` 写死 (同文件 2392). 行内 status 若是 INVALID, 仍进 UNKNOWN, 不进一个 INVALID 桶. 不要把 SUMMARY 里的 INVALID=0 读成 "没有非法行" 的实测.

缺证据的窗另打 `PRERUN_INCOMPLETE` (同文件 2124), 不是 strict 行里的 status 列.

## merge 成功行

```
sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir <跑目录> --out <合并目录> [--version VER]
```

用法: 文件头第 8 行. 读入 `strict-*.tsv`, 排除 `strict-final.tsv` (同文件 2278-2280).

写出三份, 不删入目录:

(1) `<合并目录>/strict-final.tsv` (同文件 2403)
(2) `<合并目录>/expected-fail.draft.tsv`, 表头 `batch\texpected\treason` (同文件 2345, 2404)
(3) `<合并目录>/expected-unmeasured.draft.tsv`, 表头 `batch\treason\tevidence` (同文件 2346, 2405)

成功行 (同文件 2415), 退出 0 (同文件 2417):

```
PRERUN_MERGE	windows=<文件数>	batches=<N>	PASS=<P>	FAIL=<F>	UNKNOWN=<U>	SUPERSEDED=<S>	out=<strict-final.tsv>
```

退出 0 只表示写出成功. FAIL 或 UNKNOWN 非 0 仍不是资格通过.

## archive 成功行

```
sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir <跑目录> --repo <仓> --version <V>
```

用法: 文件头第 9 行. 只复制 `windows.tsv` 与 `supervise.log` 到 `<仓>/release/evidence/<V>/prerun-windows.tsv` 与 `prerun-supervise.log` (同文件 2199, 2205-2209). 目标已存在则拒绝 `archive-destination-exists` (同文件 2242), 不覆盖.

成功行 (同文件 2267), 退出 0:

```
PRERUN_ARCHIVE	version=<V>	windows_bytes=<N>	log_bytes=<N>	home_paths_redacted=<N>
```

日志里本账号 HOME 前缀换成 `~` (同文件 2219-2229). 表或日志仍含 `/home/` 则拒绝 `archive-unresolved-home-path` (同文件 2233-2235).

跑完且无 INCOMPLETE 时, run 自己的成功行是同文件 2175 或 2178 的 `PRERUN_RUN\tstatus=DONE`. 无测量窗时带 `outer_overhead=UNMEASURED`.

## INCOMPLETE 窗的续跑

例: 任务所述 bpidecl 的 W0000 在第一次跑里超时两次. 本说明不核那次日志, 不删证据, 不换 --out.

超时缺证据时打印 (同文件 2124, 2159), 退出 1, 且该分支在写 done 之前 break (同文件 2128, done 写在 2137):

```
PRERUN_INCOMPLETE	batch=<id>	window=W0000	raw=timed-window-missing-evidence rc=<rc>
PRERUN_RUN	status=INCOMPLETE	ran=<N>	skipped=<N>	missing_windows=<N>	out=<原目录>
```

续跑, 同一 --out, 加上 --resume (文件头第 5 行):

```
sh ./unisacc.com tools/prerun-drive.cx -- run --root <根> --candidate <候选> --out <原目录> --resume
```

不要删, 不要覆盖:

(1) 不删 `<out>/strict-W0000.tsv`, `<out>/windows.tsv`, `<out>/supervise.log`, 已有的 `done.*`.
(2) 不换 --out. 新目录不是续跑.
(3) 不跑 archive 到已有目标. 目标已存在会被拒, 不是覆盖许可.

--resume 行为:

(1) `done.<窗号>` 已存在则整窗跳过 (同文件 1848-1850). 超时缺证据的窗没有这枚戳, 所以会再入.
(2) `strict-<窗号>.tsv` 已存在则先收割已完成的 PASS 行, 不丢 (同文件 964-965, 1953-1958). 只跑剩余批次.
(3) 剩余批次走 `strict-<窗号>.zNN.tsv`, 不改原窗文件名 (同文件 1214-1216). 串行恢复在重试前会 unlink 自己的 z 文件 (同文件 1221), 不 unlink 原 `strict-W0000.tsv`.
(4) 若 `done.W0000` 已被别的路径写上, --resume 会跳过整窗, 不会重试. 那时停手, 不要删戳来强行重跑.

# 0.0.107 起链前 `cc` 最后一遍口令表 (`02:35..02:50`)

编写窗口: `agy-h` (引用 [car2-run-timeline.md](car2-run-timeline.md)、[prerun-watch-card.md](prerun-watch-card.md) 与 `pre-chain-timings.md`, 6 条正文均可直接复制发信, 每条均不超过 `300` 字)

---

## 信件 1: 致全体 (`02:35` 群发 — 冻结生效与总时标)

```text
0.0.107 车分支于 02:46 冻结生效:build/freeze.tsv 写入后,main 只收集成者与 release-chain 正式提交,各窗口严禁动 .claude/worktrees/v0.0.107 源码、tests/ 与 build/,research/ 只写主仓且不提前写 PASS。全员按 car2-run-timeline.md 对表 02:50 起链(T+0m):链 ~28m,2-lane 预跑 ~50-75m,CI 与公开 ~20m;第 30 分 done<110 切保守档并守 0.1.0 最迟线 13:40。
```

## 信件 2: 致 `agy-flow` (`02:38` — F 封口确认与起链主线)

```text
确认 v0.0.107 于 48f9e054ad(13 份入车+F)完成 4A 封口。交完 car2-parallel-prep.md 与门 19 预备后切净会话再起链。依 car2-run-timeline.md:T+0m(02:50) bg-candidate start --parallel 4;T+28m 链止;T+28..31m P1-P3、S2 预热及 2-lane 开跑;T+46/51m P4/P5,尾段 P6+--warm;完后 4m merge/pack/dry/X 推 Step 2-7。
```

## 信件 3: 致 `cdx` (`02:40` — `chain-ready` 全量守时与分诊)

```text
4A-4C 热态 284.8s 贴 360s 窗。请于 02:46 前核门 5 双 --prepare 与门 19 两条预备,4B 门 22 同 TMPDIR+F sha,4C 拿同 F 全量 READY=yes failed=none 放行 02:50 起链。后依 car2-run-timeline.md:T+28..31m 三件齐核 MATERIAL/PREPARED/PREBUILD;红停 2m 内分诊 strict-Wxxxx.tsv;Step 3 盯 6 格 CI(瞬态用 G4 --redispatch)。
```

## 信件 4: 致 `agy-h` (`02:42` — 命令卡采样与证伪对表)

```text
依 car2-run-timeline.md 与 prerun-watch-card.md 只读计量:T+0m 与 T+28m 锚 owner/exit 入 chain-to-public.tsv;T+31m 起每 10 分跑命令卡三条只读命令(禁 --append-rate)报 7 字段;10 分核 done<14 复核 lanes=2;30 分核 done<110 证伪点与 13:40 线;被动采 S3 reused 行;CI 至 Y2 逐笔对账。
```

## 信件 5: 致 `ccogweco` (`02:44` — 压缩接力与证据核对)

```text
当前 ctx 64%,交完 car2-final-patchlist.md 即 /compact 或主仓根新会话(勿留车树),以 chain-day-roster.md 与 incident-playbook-107.md 接手。依 car2-run-timeline.md:T+29m 核 P1 对齐 seal-0.1.0-checklist;30 分记 done<110 证伪台账;尾段核 P6 notes 只缺 CI_RUN_ID;Step 3-7 依 INDEX.tsv 双核后封存 107。
```

## 信件 6: 致 `agyneon` 与 `ompgpt` (`02:45` — 预热闭环与解冻待命)

```text
致 ompgpt:02:50 后依 S2 盯预跑前材料与首窗 fwd-cap(防 55s 超时),P6 尾段协核 --warm 与 Step 5-6 读回;过 70% 或预跑稳后开新会话续 prerun-prewarm-plan.md。致 agyneon:107 期间于 48f9e054ad 只读待命不动代码;CI 16m 空窗备解冻派生表检查命令,待 release-post 解冻按 car2-108-candidates.md 收口。
```

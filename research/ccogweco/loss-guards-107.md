# 107 公开日的 106 三类损耗挡板(2026-11-25 ccogweco)

范围:0.0.106 实测的三类损耗,各写"流程里的挡板":时点、命令原行与判据、负责人、失败最短处置。
与 agy-h 的 car2-prewarm-list.md(预热清单)与 car2-chain-launch.md(起链清单)互引,不重复其步骤。

## 损耗一:CI 瞬态 404 导致整轮重派(21.1 分钟)

- 106 实测:首轮草稿读回与 ci-45442318936 派发 00:53:27;windows-11-arm 于 16:55:43Z 因草稿资产 404
  失败(fetch 任务的 gh release download 撞最终一致性,被 workflow `|| echo warning` 吞掉),重派
  ci-45446325698 到 01:14:30,净间隔 21.1 分钟。输出链:car2-timeline-v2.md:22、:121(ci-45442318936.json:2、
  :187-236、:587);notes-0.0.106 已知缺口 (192)。
- 挡板时点:草稿创建后、派 CI 之前(Step 6 起点)。
- 命令与判据(原行可执行):

      TAG=v0.0.107
      gh release view "$TAG" --json assets --jq '.assets[].name' | sort
      # 判据:五个资产名齐全(unisacc.com, unisacc.com.build.json, build.json, pkg.bin, slice-win-arm64, SHA256SUMS 中属草稿的五个)
      gh release download "$TAG" -p slice-win-arm64 -D /tmp/guard-107 --clobber
      # 判据:rc=0,且文件 sha256 等于候选表 slice-win-arm64_sha256

- 负责人:agy-flow(派 CI 前自检);cc 在派发记录上复核 rc 与 sha。
- 失败最短处置:本地 download 404 时等 30 秒重试一次;仍 404 则**不派 CI**,先查草稿资产上传;
  已派出的 run 若仅 fetch 任务失败:**不要用 `gh run rerun --failed`**(release-exec step3 的 run 级 JSON 与日志无 attempt 绑定,重跑会覆盖同名);发布路径用 `release-exec --redispatch`,新 run 号保留旧 run 记录。依据:research/cdx/car2-16-ci-run-attempt.md。
- 边界:重试不改 workflow、不跳过失败格、不把 warning 当通过。

## 损耗二:读回冷缓存超时回滚(6.1 分钟,且首次公开已对外)

- 106 实测:首次 exec-5-publish 01:36:31,读回 smoke 45 秒冷编超时,01:37:39 回滚 Draft,01:42:35 重公开,
  多耗 6.1 分钟。输出链:car2-timeline-v2.md:26-27、:122(INDEX.tsv:20-24);prewarm-list.md:18、:78-79。
- 挡板时点:pack 完成到 exec-5-publish 之前(Step 5 末与 Step 8 前各一次)。
- 命令与判据:

      sh ./unisacc.com tools/release-exec.cx -- --warm <pack>/candidate.tsv --layout <LAYOUT> --out <STATE>/warm
      # 第二次同 argv 复跑:判据 READY 且耗时毫秒级(106 实测 1452 ms),收据 WARM 键与候选一致

- 负责人:agy-flow;cc 公开前看 warm 收据。
- 失败最短处置:再 warm 一次;仍非 READY 就停,不公开(已公开则按 106 的回滚路径把 release 转 Draft 并读回,
  保留两次 INDEX 行)。
- 边界:45 秒时限不动;预热是重复执行,不是跳过读回;不得预写读回通过。

## 损耗三:预跑 3 lane 重窗口 rc=124 与 DVQUALIFY_NATIVE_RUN_BATCH 误设

- 106 实测:预跑实际 122.6 分钟(链起 157.2),其中 W0080 出现 rc=124 x2 与红停;3 lane 跑 heavy 窗全 rc=124,
  属负载不可行;预跑说明里把 DVQUALIFY_NATIVE_RUN_BATCH 当默认的表述已作废(该开关 opt-in,默认 0)。
  输出链:car2-timeline-v2.md:18、:112;notes-0.0.106? 相关行;记忆宫殿 2 车道与开关更正两条;
  tools/dvqualify.cx:624、tools/dvledger.cx:167。
- 挡板时点:预跑启动前(Step 4 起点)与启动后第一分钟。
- 命令与判据:

      env | grep -E 'DVQUALIFY_NATIVE_RUN_BATCH|PRERUN_' ; env -u DVQUALIFY_NATIVE_RUN_BATCH <预跑启动命令>
      # 判据:启动环境里没有 DVQUALIFY_NATIVE_RUN_BATCH=1;若必须显式,置 0
      grep -m1 'window-lanes' <prerun-dir>/windows.tsv ; grep -m1 'PRERUN_LANES' <prerun-dir>/supervise.log
      # 判据:lanes=2;首窗不得出现 rc=124

- 负责人:agy-flow 启动与首分钟自检;cc 复核 lanes 与环境两行。
- 失败最短处置:立即停预跑,保留 rc=124 原行;把未完成窗按 2 车道重启(已 done 的窗带旧 source 不复用,
  见 prerun-drive.cx:1329-1348、:1396-1445),不提高时限、不改窗数。
- 边界:2 车道是裁定,不是建议;原生运行器开关默认为 0,不得在预跑里打开冒充默认。

## 汇总

| 损耗 | 106 实测 | 挡板位置 | 判据原行 | 负责人 | 最短处置 |
|---|---|---|---|---|---|
| CI 瞬态 404 | 21.1 分钟 | 派 CI 前 | assets 五件齐 + slice download sha 相等 | agy-flow | `--redispatch` 新 run,保留旧 run(不用 rerun --failed) |
| 读回冷缓存 | 6.1 分钟(首次已公开) | Step 5 末与 Step 8 前 | warm 收据 READY,1452 ms 级 | agy-flow | 再 warm,不 READY 不公开 |
| 预跑 lane/开关 | rc=124 与 122.6 分钟 | 预跑启动前后 | lanes=2,无 DVQUALIFY=1 | agy-flow | 停、保留原行、2 车道重启 |

互引:预热对象与精确 argv 见 research/agy-h/car2-prewarm-list.md(第 78-79 行的公开前后两点);
起链前置与 2 车道启动见 research/agy-h/car2-chain-launch.md;本页只写挡板,不重复清单。

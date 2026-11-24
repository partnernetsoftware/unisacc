# E01 关闭动作草案:release 裁决文件陈旧(v2 路径订正)

声称:release/decision.txt 与 release/evidence.tsv 可能停在旧版,release-exec 第 4 与第 7 步用同一份旧输入互比恒等。
代码现状:release-exec.cx:1352-1372 已有"evidence.tsv 必须绑定候选版本、PASS 证据文件必须存在"的 DECISION 拒绝路径
(d750ad123c 上为 :1313-1336,拒绝句 `release/evidence.tsv stale: path does not reference candidate version` 在 :1332)。
所以关闭动作不是写新代码,而是定向实跑+负控;命令与判据以 research/ccogweco/e01-exec-steps.md v2 为准
(期望原行来自源码字面量,附行号)。

## 动作

1. 正控(在私有布局,不碰发布证据):用 0.0.106 的 gap 布局(chain 产物 + release/candidate.tsv)跑
       sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout <PRIVATE_LAYOUT> --dry --from-step 4
   判据:rc=0,输出出现 `STEP\t4\tY1\tDONE`(release-exec.cx:1786)与
   `RELEASE_EXEC\tRESULT\tDRY_GATES_OK\tsteps=7...`(:1788)。
2. 负控(在私有布局的副本里):把 cwd 的 release/evidence.tsv 某一行 path 改成不含候选版本号的旧路径,重跑同一命令
   (release-exec 读 cwd 的 release/evidence.tsv 或 --out/evidence.tsv,不读 --layout 下的副本,见 :1297-1311、:1527-1528)。
   判据:rc!=0,stdout 含 `GATE\t4\tDECISION\tFAIL\trelease/evidence.tsv stale: path does not reference candidate version`
   (refuse :78-80、字面量 :1332);stderr 含 `REFUSE RELEASE_EXEC_GATE_DECISION step=4 ...`;不出现 STEP 4 Y1 DONE。
3. 记录:两行的原样输出(时间、candidate、路径)写进 release/evidence/0.0.107/ 或状态表。

## 时长与窗口

- 正控 1 分钟级(第 4 步起跑,不跑 pack/CI);负控再加 1 分钟;合计 2-3 分钟。
- 窗口:无在途链、低载;需要一份 0.0.106 的私有链布局(chain 目录在 `~/.cache/unisacc/chains`,v1 误写
  `~/.cache/chains`,已订正;另有备份 `~/.unisacc-release-evidence-backups/`,不含 0.0.106)。
- 硬前提:私有树必须带 .git 与 refs/remotes/origin/main 引用,否则第 4 步 gate_main 直接 MAIN_UNKNOWN
  (release-exec.cx:254-269);纯 git archive 不满足。
- 若代码已有断言但负控无法触发(路径按行扫描),视为夹具不足,补一行夹具后再关。

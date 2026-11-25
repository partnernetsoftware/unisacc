# E01 执行脚本步骤页 v2(release-exec 第 4 步对陈旧裁决文件具名拒绝)

校验规则(本页自 v2 起):所有判据原行必须逐字来自源码打印字面量,并附
`git show d750ad123c:tools/<file> | grep -nF '<片段>'` 的行号;源码变动后重跑 grep,不得凭记忆写文案。

目标:证明 release-exec 第 4 步(Y1)对陈旧 evidence 会具名拒绝,并证明正常布局通过;2-3 分钟。

## 前置(两点都是 v2 新增的硬前提)

1. **必须是带 .git 与 origin/main 引用的树。** 第 4 步先跑 gate_main(release-exec.cx:254-269):它要求
   `refs/remotes/origin/main` 与 `HEAD` 都是 40 位,且 origin/main 是 HEAD 祖先,否则拒绝 MAIN_UNKNOWN
   (release-exec.cx:263-264)。纯 `git archive` 没有 .git,正控在第 4 步前就会 REFUSE。做法(私有目录内):
       git init -q && git add -A && git -c user.email=t@l -c user.name=t commit -q -m init
       git update-ref refs/remotes/origin/main HEAD
   dry 模式不执行 `git fetch origin`(:261-262 只 show DRY),所以离线 update-ref 足够。
2. **布局**(v2 实测限制):第 4 步会跑 `release-decide --candidate release/candidate.tsv --evidence ...
   --assets <layout>`(release-exec.cx:1348),layout 里要有候选六件资产;只有 `release/` 一份拷贝不保证过。
   优先用该版本真实 pack/prep 布局;没有时先在私有目录用 release-flow prep 造一份,再跑。
3. 106 冻结提交树里**没有** `release/evidence/0.0.106/`(2e8a19a342、b150aa37e2、d750ad123c 都为空,
   `git ls-tree <rev> -- release/evidence/0.0.106/`);该树只有 `release/candidate.tsv`(blob fc4048b2)与
   `release/evidence.tsv`(blob ed66fe86)。所以材料取 `release/evidence.tsv`,不要等 106 证据目录。

## 准备(私有 archive,不碰共享检出与真实 release/)

    T=$(mktemp -d)
    mkdir -p $T/tree && git -C <repo> archive 2e8a19a342 | tar -x -C $T/tree
    cd $T/tree
    git init -q && git add -A && git -c user.email=t@l -c user.name=t commit -q -m init
    git update-ref refs/remotes/origin/main HEAD
    ln -sfn <repo>/unisacc.com unisacc.com          # 若 archive 内没有执行体

## 正控(正常布局)

    sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
        --layout <真实 pack/prep 布局> --dry --from-step 4 > $T/pos.out 2>&1
    rc=$?

判据(逐字,源码行号):
- `STEP\t4\tY1\tDONE`(release-exec.cx:1786 打印 `STEP %d %s DONE`,step_names[4]="Y1" 见 :67)。
- 末行 `RELEASE_EXEC\tRESULT\tDRY_GATES_OK\tsteps=7\tdeferred=..\tindex=..`(release-exec.cx:1788,:1771)。
- rc=0。
- 允许但不必贴:`GATE\t4\tMAIN_ANCESTOR\tPASS\t...`(gate_pass 格式 :92,:269)。

## 负控(改 cwd 的 evidence 源,不是 layout 副本)

release-exec 第 4 步读 evidence 的顺序:若 cwd 有 `release/evidence/<ver>/evidence-rows.tsv`,先同步到
`../../release/evidence.tsv`(release-exec.cx:1297-1311);否则用 cwd 的 `release/evidence.tsv`(:1527-1528)。
`--layout` 下的副本从不参与。106 树无 evidence 目录,所以直接改 cwd 的 evidence 源:

    cp -a $T/tree $T/tree-neg
    cd $T/tree-neg
    # 把 release/evidence.tsv 任一 PASS 行的路径列改成不含 0.0.106 的旧路径(行格式:步骤号<TAB>判定<TAB>路径)
    sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
        --layout <真实 pack/prep 布局> --dry --from-step 4 > $T/neg.out 2>&1
    rc=$?

等价做法:先备 `$T/out/evidence.tsv`,再给命令加 `--out $T/out`(dry 下 :1527 优先读它;注意若 tree 内
evidence-rows.tsv 存在会先被同步覆盖,106 树没有该文件,所以成立)。

判据(逐字):
- stdout 行 `GATE\t4\tDECISION\tFAIL\trelease/evidence.tsv stale: path does not reference candidate version`
  (refuse 先打 GATE 行,release-exec.cx:78-80;字面量原文 :1332)。
- stderr 行 `release-exec: REFUSE RELEASE_EXEC_GATE_DECISION step=4 release/evidence.tsv stale: path does not reference candidate version`(:80)。
- rc!=0,且不出现 `STEP\t4\tY1\tDONE`。

## 留证

- 把 pos.out 与 neg.out 的原行(含 GATE 4 DECISION FAIL 与 REFUSE 行)贴进状态表 E01 行,evidence 列写
  106 证据目录下文件名;若负控无法触发,记"夹具不足"并先补夹具再关,不改声称口径。
- 相关路径都写绝对路径;共享检出与真实 release/ 不动。

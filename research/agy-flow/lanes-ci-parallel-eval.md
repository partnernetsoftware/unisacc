# P0 评估:预跑 long 相位 lanes 2->4 私有验证方案与 release-exec 认早跑 CI 源码核查

## 1. 预跑 `long` 相位 `lanes 2 -> 4` 私有验证方案(只写方案与命令,0.1.0 预跑期间不执行)

### (1) 源码约束与必须区分的相界(`tools/prerun-drive.cx`)

1. **现行全局车道宽度合同**:
   - `tools/prerun-drive.cx:158-162` 定义可共享车道的窗口条件:
     ```c
     static int pd_can_share_window(const char *phase, const char *ids, int jobs) {
         if (jobs != 1 || !ids[0] || strchr(ids, ',') || pd_is_early_canary_id(ids)) return 0;
         return tx_eqz(phase, "long") ||
                (tx_eqz(phase, "heavy") && (pd_starts(ids, "netregen-") || tx_eqz(ids, "unit-frontcache-equiv3") || pd_is_isolated_long_id(ids)));
     }
     ```
   - `tools/prerun-drive.cx:1855-1873` 在组队时用同一个 `lanes` 上限(`while (queued < lanes)`)同时打包 `phase=long` 与 `phase=heavy(netregen-*)` 窗口。
2. **与 `prd_mempalace.md:272` 铁律的边界**:
   - `prd_mempalace.md:272` 明文规定:"预跑并发固定为 2 车道;曾以 3 车道跑 heavy 窗,三路全 rc=124,判定为负载不可行的实测"。
   - 因此,**绝不能简单把全局 `--window-lanes 2` 改为 `--window-lanes 4`**(否则 68 个 `heavy` `netregen-*` 窗也会按 4 车道并发并复现 `rc=124`)。
   - **正确做法**:只对 `phase == "long"` 的非 canary 单批窗口(0.1.0 共 137 个 `long` 窗,减去 `W0001..W0012` 12 个 early canary 后共 **125 个可共享 `long` 窗**,每窗 `jobs=1`,4 车道在 16 核机器上仅占 4 核,低于 `normal` 相位 `jobs=8` 的单窗并发度)扩到 4 车道;而 `phase == "heavy"` 严格保持 2 车道。

### (2) P0 私有验证方案(零改源码离线切片重跑 vs 最小 `--long-lanes` 扩展)

#### 阶段 A:零改源码离线切片验证(0.1.0 公开后在 P0 窗口立即对已公开 0.1.0 APE 实测)

从 0.1.0 正式预跑目录 `~/.cache/unisacc/prerun/v0.1.0-a11d28fc0a` 复制 `frozen-root` 符号视图与 `windows.tsv`,仅保留 `W0000` (`warmup`)与全部 125 个非 canary `long` 窗(`W0014..W0021` 中的 `long` 窗及 `W0092..W0206` 的 `long` 窗),把表头 `# window-lanes=2` 改为 `# window-lanes=4`,在独立私有目录 `/tmp/p0-lanes4-eval` 下对同一 0.1.0 候选 APE(`b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`)实跑一次:

```sh
# 只写命令,待 0.1.0 公开且机器空闲(load1 < 2.0)时在 P0 窗口执行
set -e
export PATH="/usr/bin:/bin:$HOME/.local/bin"
CDIR="$HOME/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1"
APE="$CDIR/ape-final/unisacc-next.com"
BASE_PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"
EVAL_PR="/tmp/p0-lanes4-eval"

rm -rf "$EVAL_PR"
mkdir -p "$EVAL_PR"
cp "$BASE_PR/frozen-source.rev" "$BASE_PR/done.prepare-opt-frame4" "$EVAL_PR/"
ln -s "$BASE_PR/frozen-root" "$EVAL_PR/frozen-root"

# 构造仅含 W0000 warmup + 125 个非 canary long 窗的 windows.tsv,设 window-lanes=4
sh ./unisacc.com tools/devsvc.cx -- status >/dev/null 2>&1 || true
{
  printf '# prerun-drive windows\n# timing=wave-v1 jobs=8 budget_ms=22000\n# window-lanes=4\nwindow\tjobs\tphase\tcount\tids\n'
  grep -E '^W0000	|^W00(1[3-9]|[2-9][0-9])	1	long	|^W0[12][0-9][0-9]	1	long	' "$BASE_PR/windows.tsv"
} > "$EVAL_PR/windows.tsv"

env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
  UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
  timeout -k 10s 3600s sh ./unisacc.com tools/prerun-drive.cx -- run \
    --root "$EVAL_PR/frozen-root" --candidate "$APE" --out "$EVAL_PR" \
    --max-jobs 8 --max-retries 2 --window-lanes 4 \
    > "$EVAL_PR/supervise.log" 2>&1
```

#### 阶段 B:对拍验收与准入判据(先红后绿)

1. **结论多重集 100% 等价**:
   - 提取 `$EVAL_PR/strict-W*.tsv` 与 `$BASE_PR/strict-W*.tsv` 中同 125 个 `long` 窗的 `(batch, domain, status, rc)` 行排序 diff,差异行数必须为 `0`。
2. **`rc=124` 与重试零劣化**:
   - `grep -c 'rc=124' "$EVAL_PR/supervise.log"` 必须不大于 `$BASE_PR/supervise.log` 同 125 个 `long` 窗的 `rc=124` 次数(目标为 `0`),且 `PRERUN_STOP` / `PRERUN_INCOMPLETE` 为 `0`。
3. **墙钟收益线**:
   - 统计 `$EVAL_PR/supervise.log` 中 `PRERUN_LANE_WALL` 总和:基线 `lanes=2` 下 133 个 `long` 窗约 49 分钟;4 车道下目标 `<= 30` 分钟(净省 `>= 18` 分钟)。若任一条不满足,按 fable 方案 D3 判否不上。

#### 阶段 C:若阶段 A/B 通过,`tools/prerun-drive.cx` 的最小分相实现(约 15 行)

- 在 `tools/prerun-drive.cx` 新增可选参数 `--long-lanes 1..8`(默认等于 `lanes`,写入 `# long-lanes=N` 头注释):
  - `tools/prerun-drive.cx:1855-1873` 组队时,若首窗 `tx_eqz(current.phase, "long")`,组队上限用 `eff_lanes = long_lanes`(可设为 4),且仅同为 `phase == "long"` 的后续窗可入同一波;
  - 若首窗 `tx_eqz(current.phase, "heavy")`,组队上限仍固定为 `lanes = 2`,严格守住 `prd_mempalace.md:272` 的 `heavy` 2 车道红线;
  - 配套在 `tools/check-prerun-drive.cx` 增加 `--window-lanes 2 --long-lanes 4` 下 `heavy` 组队宽度为 2、`long` 组队宽度为 4 的先红后绿夹具测试。

---

## 2. `tools/release-exec.cx` 是否认 CI 与预跑并行(早跑结果)的源码核查

### (1) 结论:现行源码 **原生支持** 认账链止后早跑的 CI,无需改 `release-exec.cx`,但须满足两条精确的数据绑定合同

经逐行核查 `tools/release-exec.cx:1141-1463`、`tools/release-decide.cx:1-107` 及 `.github/workflows/candidate-native.yml:12-120`:

1. **`step_ci()` 已内建早跑 `run_id` 跳过重派与零等待验账逻辑(`tools/release-exec.cx:1277-1321`)**:
   - `tools/release-exec.cx:1277-1279`:
     ```c
     if(!runid[0] && is_digits(cand.ci_run_id)) {
         cx_fmt(runid, sizeof runid, "%s", cand.ci_run_id);
     } else if(!runid[0])index_ref("exec-3-ci-dispatch","exec-3-ci",runid,sizeof runid);
     ```
   - `tools/release-exec.cx:1290-1291`:
     ```c
     if(runid[0]) {
         show(v,"DONE");say("SKIP","CI run recorded, not re-dispatched",runid);
     }
     ```
   - 随后直接调用 `ci_fetch_and_verify()`(`lines 1233-1264`)。若该 `runid` 在预跑期间已 `completed`,轮询在 `i = 0` 立即退出(耗时约 3 秒,省去 9.1 分钟等待),下载 `ci-<runid>.json` 与 `ci-<runid>.log` 并调用 `ci_verify_files(json, log)`。
2. **`ci_verify_files()` 与 `release-decide.cx` 不检查 CI 触发时点与 `commit` 参数是否为 `X`(`tools/release-exec.cx:1166-1194`)**:
   - `ci_verify_files()` 只校验:(a) `ci-<runid>.json` 顶层 `"conclusion":"success"` 且 6 个 `"name":"candidate (` 作业全为 `"conclusion":"success"`;(b) `ci-<runid>.log` 含 `CANDIDATE_QUALIFY\tverdict=` 且 `fail=0`、`invalid=0`、无 `FAIL`/`INVALID`。
   - `ci_verify_files()` **不校验** `createdAt` 是否晚于 Step 1 `X`,**也不校验** `headSha` 或 workflow `inputs.commit` 是冻结提交 `F` 还是 `X`(而 `F` 与 `X` 之间只差 `release/candidate.tsv`,整个 `tests/` 与候选 `APE_SHA` 100% 相同)。
3. **`step_draft()` 已内建草稿 Release 已存在时的幂等跳过与覆盖上传(`tools/release-exec.cx:1150-1162`)**:
   - `tools/release-exec.cx:1157-1159`:
     ```c
     if (st == 1) { show(cr, "DONE"); say("SKIP", "draft exists, not recreated", cand.tag); }
     else if (act(cr, 55, 0, 0)) refuse("DRAFT_READBACK", "gh release create failed");
     if (act(up, 55, 0, 0)) refuse("DRAFT_READBACK", "gh release upload failed");
     ```
   - 若链止 `T2` 时已先创建私有草稿 Release `v<V>` 供早跑 CI 下载资产,Step 2 `draft` 会识别 `st == 1`(`isDraft == true`),跳过 `gh release create`,直接 `--clobber` 上传正式 7 件资产并执行 `draft_readback()`。

### (2) 两条必须遵守的早跑操作合同(防踩坑)

1. **链止 `T2` 派早跑 CI 时必须传 `release_tag=v<V>` 且草稿含 `unisacc.com`、`slice-win-arm64`、`pkg.bin` 三件**:
   - `.github/workflows/candidate-native.yml:80-83` 的 `fetch` 作业仅在 `RELEASE_TAG` 非空时下载 `slice-win-arm64` 与 `pkg.bin`;若不建草稿只传 `ape_url`,则 `windows-11-arm` 格会因缺 `slice-win-arm64` 而 `FAIL`。
   - 因此链止 `T2` 早跑 CI 的标准三步为:
     1) 确认冻结提交 `F` 已推送到远端(`origin/v<V>` 或 `origin/main`);
     2) 以 `--draft --target <F>` 创建私有草稿 Release `v<V>` 并上传链目录 `$CDIR` 的 `unisacc.com`(`unisacc-next.com`)、`slice-win-arm64`、`pkg.bin`;
     3) 执行 `gh -R <REPO> workflow run candidate-native.yml --ref main -f release_tag=v<V> -f ape_sha256=<APE_SHA> -f commit=<F>`,记下 `<EARLY_RUN_ID>`。
2. **早跑 `<EARLY_RUN_ID>` 必须通过 `INDEX.tsv` 的 `exec-3-ci-dispatch` 行注入,而不能在 Step 1 `X` 前直接写进 `release/candidate.tsv` 的第 7 列(`ci_run_id`)**:
   - **源码致命细节(`tools/release-exec.cx:1444-1454`)**:
     - `step_y1()` 在第 1444 行仅当 `!tx_eqz(cand.ci_run_id, runid)` 时才调用 `set_ci_run_id()` 修改 `release/candidate.tsv`,紧接着在第 1453 行通过 `git log -1 --format=%H -- release/candidate.tsv` 取 `Y1` 的提交号 `y1sha`!
     - 如果在 Step 1 `X` 时就把 `<EARLY_RUN_ID>` 填进 `release/candidate.tsv`,那么 Step 4 `Y1` 不会改动 `release/candidate.tsv`,导致 `git log -1 --format=%H -- release/candidate.tsv` 取回旧的 `X` 提交号或在无变更时报错。
   - **正确注入方式**:
     - Step 1 `X` 提交的 `release/candidate.tsv` 保持 `ci_run_id = pending`(由 `release-run.cx -- --pack` 生成的默认态);
     - 在跑 `release-exec.cx -- --execute` 前(或 Step 2 `draft` 后),往 `release/evidence/<V>/INDEX.tsv` 写入一行:
       `exec-3-ci-dispatch	-	<ISO_TIME>	<EARLY_RUN_ID>	candidate-native.yml early dispatched at T2`
     - 这样 Step 1 `X` 正常提交 `pending`;Step 2 `draft` 幂等更新草稿并完成 `draft-readback`;Step 3 `ci` 从 `INDEX.tsv` 读到 `exec-3-ci-dispatch` 的 `<EARLY_RUN_ID>`,跳过重派、3 秒拉回早跑全绿日志并记 `exec-3-ci PASS`;Step 4 `Y1` 把 `release/candidate.tsv` 的 `pending` 更新为 `<EARLY_RUN_ID>` 并提交 `Y1`。全程零源码改动即可在 `X -> 公开` 段净省 **9.0 分钟**。

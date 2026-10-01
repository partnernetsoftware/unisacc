# unisacc 发布流水线手册（自 v0.0.10 实践沉淀）

一次发布 = 冻结源 → 同源参考 → P3 候选 → 本地全量门禁 → 平台客机 → 一次 push 取 CI → Apple 签名公证 → Windows 企业签名 → 草稿 Release → 正式发布。每一步的**输入身份**（源闭包 sha、候选 sha、UA、环境变量）决定下一步能否复用；下面按顺序给出命令、坑与判据。所有命令单次 ≤ 60 s，长任务放后台并用完成通知，不前台轮询。

## 0. 冻结源与版本
- 改 `src/version.h` 唯一的 `UNISACC_VERSION`；VERSIONINFO/产品版本由构建读取，`ape-version` 门禁比对候选。
- 产品来源闭包 = `exec/ unisa/ src/ kernel/ include/ weights/` 下 `.py .c .h .inc .tsv .json .sh`（见 `exec/pipeline/models.py:closure`）。**exec/ 下的测试脚本也在闭包里**，改一个字节就要重打包。
- 最后一步再刷新 `tests/gatedeps.json` 的已审核树哈希（`gate-infra` 门禁比对），任何之后的 exec/ 改动都会让 compilercheck 家族退化为未审核。

## 1. 同源私有参考 UA
`python3 tests/bound.py 58 ./tests/build_ref.sh /tmp/<tag>-ua.c /tmp/<tag>-ua`。旧提交构建的 UA 会在 `exec-driver-core-modes` 上因 `--version` 不同而红，且会污染差分。不要用共享 `/tmp/ua_ref`。

## 2. P3 候选
```
make model-com MODEL_DIR=$D MODEL_STEP=shared     # 与任一 target 两槽并行
make model-com MODEL_DIR=$D MODEL_STEP=osx/arm64  # … 六个 target，两两并行
make model-com MODEL_DIR=$D MODEL_STEP=pack UA=/tmp/<tag>-ua
```
每步外层 `tests/bound.py 58`。zsh 不对变量分词：循环里显式列目标。只改 pack 输入（UA）时可复制七个阶段目录只重跑 pack；候选字节由 `unisacc-next.com.build.json` 的 `sources_sha256` 绑定，`exec/c/provenance.py check` 验证。

## 3. 本地全量门禁（`tests/release.sh --com`）
驱动脚本要点（见 `release-queue-pitfalls` 记忆）：
- `env TERM_SH_NOFALLBACK=1 ./tests/term.sh env UNISACC_FFI_X86_PROVIDER=… MODEL_COM=… UA=… GATE_STATE=… ./tests/release.sh --com`，退出 75/142 都继续；**下一窗前等上一窗的 `gatequeue.py` 进程退出**（否则状态锁冲突、结果被清）。
- 环境变量全部进 stamp：第一窗就带齐 `UNISACC_FFI_X86_PROVIDER`（Rosetta union16 需要）。
- 耗时历史按 profile key 存于 `$TMPDIR/unisacc-gate-times-<key>.json`；新 key 先合并旧文件播种，否则每窗只开约 4 个套件。
- 队列运行期间**不改任何被声明文件**（tests、prd、research、README 都算），否则 `inputs changed` 清空全部结果。
- 独占套件（exec-bindx86、warningdriver）冷模型缓存会超 48 s：先在 Terminal 环境各跑一次暖缓存。
- 每次 term.sh 交接留一个 Terminal 窗口；几百个后 osascript 超时、交接被拒。批次之间关窗，或先 `defaults write com.apple.Terminal NSQuitAlwaysKeepsWindows -bool false`。
- 判据：`queue: N/N completed, 0 failed` 且 release.sh 打印 `final rc=0`；`UNVERIFIED` 行是门禁外义务，逐条写进回执。

## 4. 平台客机
- Linux：`SUITE_LIMIT=55 JOBS=2 ./tests/linux.sh all`（linux.sh 默认 900 s 与 all.sh 上限 60 冲突，必须显式给 55）。`default`（4 GiB）跑不动两个 2.2 GB 的 Python 自编译（bigclosure/ape），要么 JOBS=1，要么用 8 GiB 的 `minicon-lnx-aarch64`（需 gcc/clang/libffi-dev/perl(shasum)，用后 `limactl stop`）。
- Windows：`utmctl start minicon-win-arm-64`，等 `utmctl ip-address` 有值（agent 就绪）；`./tests/crossnative.sh examples/*.c`（8 例约 18 s）与 `env UA=… ./tests/nativeboot.sh --windows win/arm64|win/x86_64`（各约 10 s）都可从普通 shell 跑，**不需要 Terminal**：以前的 -10004 是 `utmctl exec --hide` 造成的（2026-09-29 查明）。客机坑见 prd v0.0.12 表“R11-0 ④ 复盘”。用后 `utmctl stop`。

## 5. CI（一次 push；2026-09-29 起为 minicon 模式）
- push 只触发 `release-check.yml`（约 1 分钟：预检、权重构造、ELF 实跑；若 `release/candidate.json` 封存了 GHCR 摘要，再在 ubuntu/macos 拉取候选实跑）。全量 `ci.yml` 每周一定时或手动，是安全网不是前提。
- `windows-signing.yml` 的上游是 release-check 的 run（id/attempt），main 按产品源闭包摘要比对；文档提交不打断签名。实测 push→签完约 4 分钟。
- GHCR 封存：`gh auth refresh -s write:packages` 一次后 `oras push ghcr.io/partnernetsoftware/unisacc-candidate:<v> unisacc.com unisacc.com.build.json model-audit/models.json`，把摘要写进 `release/candidate.json`。
- **每次产品闭包变化都要重封**（2026-09-29 实测）：`release-check` 的 candidate 作业先比对 `candidate.json.sources_sha256` 与本树闭包，不一致直接红；0.0.11 开发期三次 push 因此全红。做法：每片装根后立刻 `seal_candidate.sh DIR <ver>-dev NOTE`（暂存目录复制三件、`oras push --format json` 取摘要、写 candidate.json），与回执同一批提交；正式版再封 `<ver>` 标签。
`ci.yml` 先跑 `ciplancheck/ci_shard_plan/opt_partition_check`，再按 `all.sh --list` 逐套件 55 s。托管 runner 的 Python 参考 VM 约慢 4 倍：探针规模按 45 s 参考上限设计（b_malloc 已缩）。selfhost 在 `SELFHOST_STATE` 未准备时回退自建（acceptance8 在 prepare 之前运行）。

## 6. Apple 签名与公证
`release/macosbundle.py build|sign|dmg|assess` + 自己的 notarytool 步骤（见 scratch 中的 apple-sign.sh 流程）：专用临时 keychain 导入公司 Developer ID p12（`~/.private_keys/`），`codesign --options runtime --timestamp`，`ditto` 打 zip → `notarytool submit --keychain-profile minicon-notary --wait` → `stapler staple` app → dmg → 再公证 → staple → `spctl` 评估。profile 存于系统新式凭据库，`security find-generic-password` 找不到是正常的。

## 7. Windows 企业签名（`windows-signing.yml`）
前提：候选同 SHA 的 CI 成功、main HEAD == source_sha、草稿 Release 已传 `unisacc-unsigned.zip` + `unsigned-receipt.json`、`release/signing-policy.json` mode=required。
- 先 `mode=qualification`（零额度），再 `mode=company`。company 进入 `release-signing` 环境要审批：`POST /actions/runs/{id}/pending_deployments`（JSON body，environment_ids 为整数）。
- 草稿只能按 `tag_name` 在 `/releases?per_page=100` 里找（`/releases/tags/{tag}` 不返回草稿），且需要 `contents: write` 的 token。
- 任何 docs 提交都会移动 main HEAD：dispatch 前把回执（source_sha/run_id/**run_attempt**）重生成并重传，tag 与草稿 target 同步到该 SHA；CI 只重跑失败 job 会使 attempt 递增，回执必须写实际 attempt。
- `Azure/artifact-signing-action` 的坑：a) 首跑安装 ArtifactSigning 模块与客户端包超过 1 分钟 → step 4 分钟、`cache-dependencies: true`、服务 `timeout: 200`；b) 它以 catalog 文件所在目录为文件根（Split-Path），catalog 放仓库根会得到空 Path（`Get-CatalogFileList: Cannot bind argument to parameter 'Path'`），必须像 minicon 一样放进子目录 `signing-input/`，条目写相对文件名，签完再拷回 `signed/` 供信任法院；c) Windows runner 的 Python 子进程按 cp1252 解码 gh 输出，顶层 `PYTHONUTF8=1`。
- 托管 macOS runner 排队可达 30 分钟、速度波动大：CI 只允许重跑失败 job，不改源；`tools11`（tiny-regex test2）41–50 s 余量太薄，R11-1 拆分。

## 8. 发布
**发布后立即**把本轮候选放回仓根作为本地“当前产品”（不跟踪，但门禁与 `MODEL_COM` 默认读它）：`cp <cand>/unisacc-next.com unisacc.com && cp <cand>/unisacc-next.com.build.json unisacc.com.build.json && python3 exec/c/provenance.py check unisacc.com`——放的是**未签名候选字节**（与 build.json 的 artifact_sha256 一致；签名后的字节只在 release 资产里）。0.0.13 发布后仓根仍是 0.0.12 的 df8cc9b4…，主人发现 `--version` 不对（2026-09-30）。
签后产物从 run artifact `unisacc-company-signed-<attempt>` 下载，核对签前/签后 SHA 与尺寸，上传草稿，回执写入 `research/r<N>-release-acceptance.json`，`gh release edit --draft=false`。之后再提交回执/文档。

**推送的独立证据**（2026-09-29，dsh 建议）：GitHub 间歇 500 时 `git push` 的返回不可信——同一晚两次“以为已推、实际未推”。任何“已推送”的宣布都以下面两串相等为准，而不是以 push 返回为准：
```
[ "$(git ls-remote origin refs/heads/main | cut -c1-40)" = "$(git rev-parse HEAD)" ] && echo synced || echo NOT-PUSHED
```

## 9. 一条命令序列（0.0.15 R15-5，按 0.0.13–0.0.14 两次实发整理；工具在 `release/tools/`）

```
# 0 冻结：通知 cdx 冻结 main；src/version.h 改版本号；提交推送
S=<scratch>; V=0.0.N
python3 tests/bound.py 58 ./tests/build_ref.sh $S/ua.c $S/ua                 # 1 同源参考（每次源码变都重建）
release/tools/build_candidate.sh $S/cand $S/ua                               # 2 候选：七步 + pack-models + pack-driver
D=/tmp/seed-$V; mkdir -p $D; cp $S/cand/unisacc-next.com $D/unisacc-seed.com; cp $S/cand/unisacc-next.com.build.json $D/unisacc-seed.com.build.json
python3 exec/c/comboot.py stage 1 $D/unisacc-seed.com
for s in stage2 stage3 fixedpoint; do SEED_DIR=$D python3 exec/c/comboot.py shard $s; done   # 3 N22（stage2 装到仓根 unisacc.com）
MODEL_COM=$S/cand/unisacc-next.com tests/fb12multi.sh; python3 tests/comdemo.py --com $S/cand/unisacc-next.com --out $S/demo.json
release/tools/seal_candidate.sh $S/cand $V "<note>"; git commit -- release/candidate.json   # 4 封 GHCR
make gatedeps; git commit -- tests/gatedeps.json; git push                  # 5 最后一提交；ls-remote 核对
# 6 预热冷建网络的两项（直到 R16-5 自动化）：bindprep.sh（CORE_ASM_ARCH=arm64）两次、memorycheck.sh（x86_64 ua 1/3）一次
tests/term.sh env UA=$S/ua MODEL_COM=$S/cand/unisacc-next.com SEED_DIR=$D GATE_STATE=/tmp/gq UNISACC_FFI_X86_PROVIDER=... tests/release.sh --com   # 7 重复到 rc!=75
release/tools/release_prep.sh $S/cand $V && release/tools/apple-sign.sh /tmp/r<N>-release/unisacc.com <sha> /tmp/r<N>-release/apple   # 8 Apple（可与 7 并行）
# 9 release-check：push 后等 release-check.yml 对 HEAD 绿，记 run id / attempt
gh release create v$V --draft --target $(git rev-parse HEAD) --notes-file notes.md; gh release upload v$V <dmg> <app.zip>
python3 release/tools/unsigned_receipt.py /tmp/r<N>-release/unisacc.com /tmp/r<N>-release/windows $(git rev-parse HEAD) <run> <attempt>; gh release upload v$V <zip> <receipt>
release/tools/windows_sign.sh qualification v$V <receipt> <run> <attempt>    # 10 Windows
R=$(release/tools/windows_sign.sh company v$V <receipt> <run> <attempt>); release/tools/approve_signing.sh $R
gh api repos/.../actions/artifacts/<id>/zip > signed.zip                  # 11 下载签后产物，核对 before/after sha 与尺寸，本机跑 comdemo/fb12-multi
# 12 向主人确认公开；只留签名 unisacc.com 与 dmg；gh release edit v$V --draft=false --latest
# 13 回执 research/r<N>-release-acceptance.json；plans/v$V.md → archive/plans/；prd 版本行；通知 cdx 解冻
```

## 10. 发布前必跑清单（R16-14，0.0.16 起；每项要有当次的证据行，缺一不发）

| # | 项 | 怎么跑 | 证据 |
|---|---|---|---|
| 1 | 本地滚动队列全绿 | `tests/release.sh --com` 直到 rc=0 | GATE_STATE 目录与 HEAD |
| 2 | origin `release-check` 对结项提交绿 | `gh run list -w release-check -L 1` | run id / attempt |
| 3 | Windows `-run` 义务 | `tests/vms.sh up` → `./tests/crossnative.sh`（win/x86_64、win/arm64 都不得是 skip）→ `tests/vms.sh down` | crossnative 汇总行 |
| 4 | 六平台外部执行与自举证据 | release-check 的六个 candidate runner 全绿（含 win 与 osx 两架构） | 同 2 的 run 页 |
| 5 | Linux x86_64 真机运行 | `./tests/linux.sh`（minicon-lnx-x86_64，模拟时看门狗 ×10），含 elfobj 的 x86_64 程序链接后运行 | linux.sh 汇总行 |
| 6 | 产品自我演示 | `python3 tests/comdemo.py --com <候选>`、`MODEL_COM=<候选> tests/fb12multi.sh` | demo.json、fb12multi 汇总 |
| 7 | 语料棘轮 | `./tests/realprog.sh`、`./tests/tools.sh`、corpus 1–4：pass 不低于 baseline，且 realprog 比上一版至少多过一个 | 各自汇总行 |

0.0.15 因不发布而没有补的是第 3、4 两项；0.0.16 新增第 5 项（x86_64 的 `.o` 只在本机链接过，还没在 x86_64 机器上运行）。

**结项条件（0.0.16 起，发布与不发布的版本都适用）**：本地 `tests/release.sh --com` rc=0 **且** `gh run list -w release-check -L 1` 对结项提交为 success，两者缺一不得结项。0.0.15 只看了本地队列就结项，而 origin 自 300e8c9 起红了十二次提交（ci plan 自检被 0a8d265 的 knownfail 调用打破，127b6b1 修复）。

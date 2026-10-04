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
- Linux 源码套件：只在**原生 arm64** 客机上跑全套：`LIMA_VM=default SUITE_LIMIT=55 JOBS=2 ./tests/linux.sh all`（约 7 分钟；linux.sh 默认 900 s 与 all.sh 上限 60 冲突，必须显式给 55）。结论只认“only guest-known reds”那一行。
- **Linux x86_64 不在模拟机上跑全套**（0.0.21 教训）：`minicon-lnx-x86_64` 在 arm64 宿主上是 qemu 模拟，约慢 10 倍。显式 `SUITE_LIMIT=55` 会覆盖 linux.sh 的 ×10 放大，0.0.21 一轮约 95 个超时、8 个 rc=1，而这些套件在原生 arm64 上全绿；`tests/all.sh` 构建参考 UA 的 45 s 与 `tests/lib.sh` 的 30 s 限时也不随之放大（模拟机上 gcc -O2 需 76 s）。x86_64 的产品证据来自 release-check 的真机 runner（ubuntu-latest，见 §5）；模拟机只用于定向套件（ccinterop、forward 的 lnx/x86_64 项，队列运行时开着它即可）。`default`（4 GiB）跑不动两个 2.2 GB 的 Python 自编译（bigclosure/ape），要么 JOBS=1，要么用 8 GiB 的 `minicon-lnx-aarch64`（需 gcc/clang/libffi-dev/perl(shasum)，用后 `limactl stop`）。
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
前提：候选同 SHA 的 CI 成功、`rc/<tag>` 指向 source_sha 且它在 main 上（0.0.22 起；main 之后可以继续前进）、草稿 Release 已传 `unisacc-unsigned.zip` + `unsigned-receipt.json`、`release/signing-policy.json` mode=required。
- 先 `mode=qualification`（零额度），再 `mode=company`。company 进入 `release-signing` 环境要审批：`POST /actions/runs/{id}/pending_deployments`（JSON body，environment_ids 为整数）。
- 草稿只能按 `tag_name` 在 `/releases?per_page=100` 里找（`/releases/tags/{tag}` 不返回草稿），且需要 `contents: write` 的 token。
- 0.0.22 起签名绑定 `rc/<tag>`，不再比较 main 末端：main 上的后续提交（含产品改动）不影响签名；回执的 source_sha、草稿 target 都用 rc 指向的 SHA；CI 只重跑失败 job 会使 attempt 递增，回执必须写实际 attempt。
- `Azure/artifact-signing-action` 的坑：a) 首跑安装 ArtifactSigning 模块与客户端包超过 1 分钟 → step 4 分钟、`cache-dependencies: true`、服务 `timeout: 200`；b) 它以 catalog 文件所在目录为文件根（Split-Path），catalog 放仓库根会得到空 Path（`Get-CatalogFileList: Cannot bind argument to parameter 'Path'`），必须像 minicon 一样放进子目录 `signing-input/`，条目写相对文件名，签完再拷回 `signed/` 供信任法院；c) Windows runner 的 Python 子进程按 cp1252 解码 gh 输出，顶层 `PYTHONUTF8=1`。
- 托管 macOS runner 排队可达 30 分钟、速度波动大：CI 只允许重跑失败 job，不改源；`tools11`（tiny-regex test2）41–50 s 余量太薄，R11-1 拆分。

## 8. 发布
**只发封存字节**：签名前 `before_sha256` 必须等于本机队列验证过的候选哈希（`release/candidate.json` 的 `unisacc_com_sha256`）；`release/tools/publish.sh TAG 签后sha` 先核对草稿字节再公开，公开后再下载比对。
**发布后立即**把本轮候选放回仓根作为本地“当前产品”（不跟踪，但门禁与 `MODEL_COM` 默认读它）：`cp <cand>/unisacc-next.com unisacc.com && cp <cand>/unisacc-next.com.build.json unisacc.com.build.json && rm -rf model-audit && cp -R <cand>/model-audit model-audit && python3 exec/c/provenance.py check unisacc.com`——放的是**未签名候选字节**（与 build.json 的 artifact_sha256 一致；签名后的字节只在 release 资产里）。0.0.13 发布后仓根仍是 0.0.12 的 df8cc9b4…，主人发现 `--version` 不对（2026-09-30）。
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
release/tools/rc_tag.sh v$V                                                 # 5a 绑定：rc/v$V 指向这个提交；拒绝 sealed_from_commit 不在 origin 上的情况；之后 main 可以继续提交
# 5b x86 libffi 提供者（rosetta 套件需要；/var/folders 下的旧目录会被清掉）：
#    curl -sSLfo $S/libffi-3.5.2.tar.gz https://github.com/libffi/libffi/releases/download/v3.5.2/libffi-3.5.2.tar.gz   # sha256 f3a3082a…
#    python3 exec/c/buildffiprovider.py --tarball … --output $S/ffix86 --target osx/x86_64   # 调三次，每次一步
# 5d precheck.sh 现在检查 5b 的 manifest 与 5c 的虚拟机，缺一项就失败
# 5c 开 x86_64 Lima（ccinterop 的 lnx/x86_64 三项，否则 skip 判失败）：limactl start minicon-lnx-x86_64；用完 stop
# 6 预热：release.sh 前三窗自动做，标记按工作树区分（0.0.22 起，换工作树会重新预热）；手动预热时也要**在队列自己的工作树里**做（0.0.21 两项冷缓存 53 s 超时）：bindprep.sh（CORE_ASM_ARCH=arm64）两次、memorycheck.sh（x86_64 ua 1/3）一次、warningcheck.sh ua Wall 一次
tests/term.sh env UA=$S/ua MODEL_COM=$S/cand/unisacc-next.com SEED_DIR=$D GATE_STATE=/tmp/gq UNISACC_FFI_X86_PROVIDER=... tests/release.sh --com   # 7 重复到 rc!=75
release/tools/release_prep.sh $S/cand $V && release/tools/apple-sign.sh /tmp/r<N>-release/unisacc.com <sha> /tmp/r<N>-release/apple   # 8 Apple（可与 7 并行）
# 9 release-check：push 后等 release-check.yml 对 HEAD 绿，记 run id / attempt
RC=$(git rev-parse rc/v$V^{commit}); gh release create v$V --draft --target $RC --notes-file notes.md; gh release upload v$V <dmg> <app.zip>
python3 release/tools/unsigned_receipt.py /tmp/r<N>-release/unisacc.com /tmp/r<N>-release/windows $RC <run> <attempt>; gh release upload v$V <zip> <receipt>
release/tools/windows_sign.sh qualification v$V <receipt> <run> <attempt>    # 10 Windows
R=$(release/tools/windows_sign.sh company v$V <receipt> <run> <attempt>); release/tools/approve_signing.sh $R
gh api repos/.../actions/artifacts/<id>/zip > signed.zip                  # 11 下载签后产物，核对 before/after sha 与尺寸，本机跑 comdemo/fb12-multi
# 12 向主人确认公开；只留签名 unisacc.com 与 dmg；gh release edit v$V --draft=false --latest
# 12b 冒烟：gh workflow run release-smoke.yml -f tag=v$V -f sha256=<签后sha>，六格全绿写进回执（0.0.22 起）
# 13 回执 research/r<N>-release-acceptance.json（含 com-auditnet 的网络计数行）；plans/v$V.md → archive/plans/；prd 版本行；通知 cdx 解冻
```

## 10. 发布前必跑清单（R16-14，0.0.16 起；每项要有当次的证据行，缺一不发）

| # | 项 | 怎么跑 | 证据 |
|---|---|---|---|
| 1 | 本地滚动队列全绿 | `tests/release.sh --com` 直到 rc=0 | GATE_STATE 目录与 HEAD |
| 2 | origin `release-check` 对结项提交绿 | `gh run list -w release-check -L 1` | run id / attempt |
| 3 | Windows `-run` 义务 | `tests/vms.sh up` → `./tests/crossnative.sh`（win/x86_64、win/arm64 都不得是 skip）→ `tests/vms.sh down` | crossnative 汇总行 |
| 4 | 六平台外部执行与自举证据 | release-check 的六个 candidate runner 全绿（含 win 与 osx 两架构） | 同 2 的 run 页 |
| 5 | Linux 源码套件与 x86_64 真机运行 | 原生 arm64 `LIMA_VM=default ./tests/linux.sh all`（只剩客机已知红）；x86_64 由第 4 项的 ubuntu-latest runner 跑封存字节，模拟机全套不算证据 | linux.sh 汇总行 + run 页 |
| 6 | 产品自我演示 | `python3 tests/comdemo.py --com <候选>`、`MODEL_COM=<候选> tests/fb12multi.sh` | demo.json、fb12multi 汇总 |
| 7 | 语料棘轮 | `./tests/realprog.sh`、`./tests/tools.sh`、corpus 1–4：pass 不低于 baseline，且 realprog 比上一版至少多过一个 | 各自汇总行 |

0.0.15 因不发布而没有补的是第 3、4 两项；0.0.16 新增第 5 项（x86_64 的 `.o` 只在本机链接过，还没在 x86_64 机器上运行）。

**结项条件（0.0.16 起，发布与不发布的版本都适用）**：本地 `tests/release.sh --com` rc=0 **且** `gh run list -w release-check -L 1` 对结项提交为 success，两者缺一不得结项。0.0.15 只看了本地队列就结项，而 origin 自 300e8c9 起红了十二次提交（ci plan 自检被 0a8d265 的 knownfail 调用打破，127b6b1 修复）。

## 11. 0.0.17 回顾的新经验（2026-10-01）

- **改旗标语义前查遍调用方**：`-c -b` 从 tape 改为对象时，compilercheck 的依赖用例与 warningcheck 的障碍用例仍把 `-c` 当 tape，直到发布队列才红；exec/ 下的测试属于产品闭包，改它就要重建并重封候选（字节不变时 Apple 公证可沿用）。
- **队列运行期间连 prd/plans 也不提交**：一次 prd 索引行提交让 93/401 作废。队列开始后写的计划先放暂存目录。
- **Windows 就绪看 IP**：`utmctl exec`（含 `--hide`）在来宾代理未起时也返回 0；就绪判据用 `utmctl ip-address` 非空，再等十几秒。
- **签名前冻结在最终提交**：签名回执绑定源提交与上游 run；签名后再修就整套重做（0.0.16 发生过一次）。
- **公开后下载核对**：`gh release edit --draft=false` 之后下载公开的 `unisacc.com`，与签名回执的签后哈希比对，结果写进回执。

## 12. 不被打断的发布（0.0.19 R19-0，2026-10-01）
- 冻结前先跑 `release/tools/precheck.sh UA`：build-weights 本地 ≤19 s、队列预热三步都过，才建候选（0.0.18 三次重建都是这类问题建候选后才暴露）。
- `release/tools/queue.sh` 在 `/tmp/unisacc-queue-<sha>` 独立工作树里跑；main 上的提交不影响队列。realprog 语料经 `REALPROG_CACHE` 指回主检出。被忽略的根产物 `unisacc.com`(.build.json) 在建工作树时复制进去（0.0.19 队列在 408/415 时因 comboot 写出它而判“输入变化”作废）。用完 `git worktree remove`。
- `tests/release.sh` 以被声明输入的 `git ls-tree` 哈希判作废，不再看 HEAD：提交 plans/prd 不作废队列。

## 13. 0.0.20 回顾（2026-10-02，0.0.21 R21-14）

- 冻结前 `release/tools/precheck.sh` 现在也跑 subtract-safety，并核对仓根 `unisacc.com` 与 `unisacc.com.build.json` 成对；queue.sh 遇到不成对直接拒绝开跑。
- 平台必跑项串行：Windows 虚拟机做自举时不要同时跑 Linux 必跑项（0.0.20 一次并行导致 19 个套件超时假红）；冻结时就在后台启动 Windows 虚拟机（开机约 250 秒），`tests/vms.sh up` 等不到代理就手动 `utmctl start` 再等 IP。
- Apple 公证在载荷定稿（队列与平台必跑全绿）之后只做一次。
- 提速分析见 research/pipeline-speed-review-0020.md。
- **发布与同时修改互不冲突（0.0.21，主人要求）**：队列跑在钉住提交的独立工作树里；一轮里发现测试或清单要改时，直接提交，然后用**同一份 GATE_STATE** 在新 HEAD 上续跑（`QUEUE_WORKTREE=/tmp/unisacc-queue-<新sha> release/tools/queue.sh ...`）。release.sh 不再因树变化拒绝续跑；gatequeue 逐任务比对指纹（声明输入、设置、可执行文件），只重跑受影响的任务；任务表变了也只丢掉改了命令或已删除的任务。产品闭包变了才需要重建候选，那时同源编译器和候选的指纹会让相关任务自动重跑。
  **实测修正（0.0.21 发布）**：上面的续跑只在**同一个工作树**里成立。gatequeue 的指纹含工作树路径（ROOT），换到 `/tmp/unisacc-queue-<新sha>` 后约 420/432 项作废，等于整轮重跑；comboot 的阶段记录写在工作树的 `out/comboot/stages.json`，沿用旧的 seed 结果会让 stage2/3 报“recorded before”。所以在修好指纹（archive/plans/v0.0.22.md）之前：只改测试、不改产品时，**在原工作树里**补跑（把失败项从 `$GATE_STATE/results.json` 删掉再跑 queue.sh），必须换工作树时 comboot 整组一起删。

## 14. 0.0.21 回顾（2026-10-02）与下一版流程

**这次实际走通的顺序**：precheck → 候选（44f9eb15）→ comboot N22 → 封存 GHCR → 队列八轮加续跑（最终 432/432）→ push → release-check（六格真机 runner 全绿）→ 草稿 + 未签名回执 → Windows 资格签名 → 公司签名并审批 → 下载签后产物、核对 before = 候选 → 上传 → Linux arm64 原生全套 → `publish.sh` → 下载公开资产冒烟 → 回执 → 归档计划 → 通知 cdx 解冻。

**教训**
1. **模拟机误报**：在模拟的 x86_64 上跑全套只会得到超时（见 §4）。x86_64 证据用真机 runner。
2. **原生 runner 六格就是产品证据**：release-check 的 candidate 作业按 GHCR 摘要拉取封存字节，在 macos-15、macos-15-intel、ubuntu-latest、ubuntu-24.04-arm、windows-latest、windows-11-arm 上跑演示套件，runner 上不编译。回执的 release_check 字段记 run id 和六格结论。
3. **封存与重封**：release-check 先比对 `candidate.json.sources_sha256` 与树的产品闭包；产品闭包一变（含 exec/ 下的检查脚本）就要重建并重封。冻结 main 之后仍重封了 6 次，根源是候选和签名都绑在 main HEAD 上。
4. **sealed_from_commit 指向不存在的提交**：0.0.21 在封存后变基（远端先进了 e44dbe1），`sealed_from_commit` 留下的 40d3d9d 在 origin 上不存在。闭包摘要仍一致，所以字节可信，但溯源断了。规则：封存前先 `git fetch` 并与 origin 对齐，封存后不再变基；需要整合远端提交就用 merge，或者重封。
5. **外部状态会消失**：rosetta 套件用的 x86 libffi 提供者放在 /var/folders，被系统清掉后 11 项红。提供者放 scratchpad，或者每次发版按 §9 第 5b 步重建。
6. **skip 就是失败**：ccinterop 在 x86_64 Lima 没开时跳过 3 项，整套判红。
7. **限时边缘的任务**：bindprep 和 warningdriver 在冷缓存下超过 53 s，预热后只要 6–15 s。

**下一版的目标流程（照搬 minicon，排期见 archive/plans/v0.0.22.md）**：minicon 的 `.github/workflows/` 已经跑顺——
- `candidate.yml`：输入精确的 40 位源 SHA 和成功的六格 run，把候选绑定在这个 SHA 和 run 上，而不是“main HEAD 冻结”；封存信息记在 rc/ tag 上，main 照常提交推送。
- `release.yml`：只按候选 run 的封存字节发布，不重建；发布前在 Windows 上执行一次；要求输入确认串 `publish-vX.Y.Z`，并支持 dry-run。
- `release-smoke-test.yml`：发布后从公开资产下载，核对 checksum，在 linux、windows、macos（挂载 dmg、验签与公证）上冒烟。
- `defender-ci-scan.yml`：用 Windows Defender 扫描封存字节。
在这些落地之前，本文 §9 的序列仍然有效，加上 §4 和本节的修正。

## 15. 0.0.22 回顾（2026-10-03）：第一次 rc 绑定发版

- **rc 绑定已实测可用**：`release/tools/rc_tag.sh v$V` 在封存与 gatedeps 推送之后打标签；签名按 rc 指向的提交校验，main 在签名期间已前进，资格和公司签名都通过。第一次资格签名失败，原因是签名工作流的 checkout 是浅克隆，祖先检查找不到 sealed_from_commit；已改为 `fetch-depth: 0`。
- **候选重建时要移动 rc 标签**：`git push origin :refs/tags/rc/v$V && git tag -d rc/v$V`，然后重新执行 rc_tag.sh。
- **队列状态放在候选目录之外**（0.0.23 F6）：queue.sh 现在用 `CAND_DIR.queue/`（日志也在里面）。0.0.22 重建候选时清空了候选目录，原来放在里面的 GATE_STATE 被一起删掉，只能整轮重跑。
- **comboot 的先后顺序**：gatequeue 的 `AFTER` 表保证 stage2、stage3、fixedpoint 在前一阶段通过后才开始；以前“耗时最长优先”会把 stage2 排在 seed 前面。
- **precheck 的冷缓存**（0.0.23 F1）：第一遍超时只说明缓存是冷的，precheck 会自动再跑一遍热的，第二遍仍超时才判失败。
- **发布后冒烟**：`gh workflow run release-smoke.yml -f tag=v$V -f sha256=<签后 sha>`，0.0.22 六格全绿，结果写进回执的 post_release_smoke。


## 16. 0.0.23 回顾（2026-10-04）：K2 之后的第一次发版

本版队列多跑了约六轮，假红远多于真问题。下面每条都实际踩过；下次发版前先逐条对照。

**候选与闭包**
- **构建候选前，闭包目录里不能有任何未跟踪文件**。provenance 的闭包按 rglob 收集 exec/、unisa/、src/、kernel/、include/、weights/ 下的文件，未跟踪的也算。0.0.23 有个遗留的临时 tsv 在 exec/parse2 里，候选建完才删，结果队列第 7 窗报“product inputs changed”，只好重建候选。构建前先跑一次 `git ls-files -o -- exec unisa src kernel include weights iterate`，输出必须为空。
- **exec/build 下直接放的 *.py 是 K2 驱动，算构建输入**；生成的输出只能放在 exec/build/ 的子目录里。provenancecheck 和 queuecheck 的夹具已按此调整。gatequeue 的 reviewed_trees 清单现在只数 git ls-files 列出的文件，被忽略的生成物不再改变指纹。
- **改了 include/*.h 之后要跑 `python3 -m unisa emit-kernel`**，否则 kernel 门禁会报 STALE。生成的 kernel 文件是产品输入，重新生成后必须重建候选。

**打包与 comboot**
- **pack-models 已拆成 pack-prep-1..3 加只打包的 pack-models**（ace7d6d3）。K2 之后 parse2 单次构造要 30–40 秒，整步放不进 55 秒。prep-1 和 prep-2 各约 40 秒，余量只有约 4 秒；parse2 再变慢就得在 gen.py 内部继续拆。
- comboot 的 stage3 偶尔有一步撞上限时。步骤有完成标记，再跑一次 `comboot.py shard stage3` 就能从停下的地方接着做。

**本地发版队列（tests/release.sh --com）**
- **改了 tests/gatequeue.py，所有作业都会重跑**：每条依赖声明都包含它。只改 knownfail 或单个检查脚本时，只有声明了它的作业会重跑。
- **已经失败的作业，续跑时不会自动重跑**。修好原因后，从 `$GATE_STATE/results.json` 删掉那一条（先备份），再续跑。
- 驱动脚本必须把 rc=142（窗口超时）当作可以重试：大作业密集的那段时间里，窗口几乎每次都会超时，结果都记在 results.json 里，不会丢。连续超时的上限要设得很高（0.0.23 用的是 200）；设成 5 或 30 都中途停过。
- **在 --jobs 4 下贴着上限的套件都要拆分或预热**：csmithdiff 拆成 40 片，exec-chain 拆成 5 片，seed-matrix-features 拆成 3 片；warnings/errors 套件改用 warnprep 预热，lib-*-source 用 lib-source-prep，exec-*self 用 selfprep，都是共享一份按内容哈希命名的模型缓存。新加套件时，单独跑超过 30 秒的就照此处理。
- **任何 skip 都会被证据检查判为失败**（release: empty or skipped evidence）。新探针如果 include 同目录的头文件，difftest 和 difftest_o 编译参考副本时都要加 `-I "$(dirname "$f")"`。
- **产品修好一个探针后，所有 knownfail 文件里的对应条目都要删**：difftest.com.knownfail、product-refusals.knownfail、ccrun.knownwrong、pyfront.knownfail。报 revived 是好消息，但仍然算红。
- 发版期间 **Lima x86_64 要一直开着**（ccinterop 需要），**Windows 虚拟机也要先开**（winposix 需要）；任何一个没开，相应套件都会因为跳过而失败。

**Linux 客机（tests/linux.sh）**
- **全套用 8 GiB 的 minicon-lnx-aarch64，加 `JOBS=1`**。bigclosure 和 ape 的每个 python3 峰值要 3–4 GB，4 个作业并发会被 OOM 杀掉，日志里显示成“Python image build failed”。
- **客机的 /tmp 是约 4 GB 的内存盘**。linux.sh 现在把客机端 TMPDIR 指向 `$HOME/unisa-tmp`（虚拟机磁盘）。失败的运行会把整棵树（约 2–4 GB）保留在 TMPDIR 里，下一轮开始前要清掉。

**流水线（缩短总时长）**
- 发版队列一开跑，不依赖它的步骤就并行做：release_prep 加 Apple 签名和公证（基本是等网络）、草稿 release、release-check（推送 rc 提交时就触发）、Windows qualification 和 company 签名。0.0.23 在队列还没跑完时，这几步就都完成了。
- 公开发布仍然要等：本地队列 rc=0、Linux 全绿、crossnative 没有跳过，并且主人确认。

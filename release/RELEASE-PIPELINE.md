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
- **队列跑在冻结的分离工作树里**（0.0.31：主树上他人的提交/未提交编辑三次作废 118、396 个结果）：`git worktree add --detach scratchpad/wtNN HEAD`，同源 UA 从该树 `tests/build_ref.sh X.c X` 构建；gitignored 的 `unisacc.com`、`unisacc.com.build.json`、`unisacc-seed.com` 要手工拷进去（缺了 ffi-product、ape-version、apps-real、strconvert-model、qprefix 立即红）；驱动要带 `REALPROG_CACHE=<主检出>/corpus`（否则 realprog 跳过 lua 报回归）和 `UNISACC_FFI_X86_PROVIDER`（否则 gate-infra、lib-*-rosetta 红）；完事 `git worktree remove`。
- 独占套件（exec-bindx86、warningdriver）冷模型缓存会超 48 s：先在 Terminal 环境各跑一次暖缓存。
- 每次 term.sh 交接留一个 Terminal 窗口；几百个后 osascript 超时、交接被拒。批次之间关窗，或先 `defaults write com.apple.Terminal NSQuitAlwaysKeepsWindows -bool false`。
- 判据：`queue: N/N completed, 0 failed` 且 release.sh 打印 `final rc=0`；`UNVERIFIED` 行是门禁外义务，逐条写进回执。

## 4. 平台客机
- Linux 源码套件：只在**原生 arm64** 客机上跑全套：`LIMA_VM=default SUITE_LIMIT=55 JOBS=2 ./tests/linux.sh all`（约 7 分钟；linux.sh 默认 900 s 与 all.sh 上限 60 冲突，必须显式给 55）。结论只认“only guest-known reds”那一行（0.0.30 R3 起 bigclosure/ape 不再算客机已知红，只剩无 clang 的 warn）。**R2（0.0.30）：Linux 全套不与本地队列重叠**——0.0.29 重叠导致四个超时假红、队列因负载暂停 600 s；排在队列 rc=0 之后（等签名、公证的空档里跑），或与队列同跑时用 `JOBS=1`。
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
D=$S/cand/seed                                                               # build_candidate 已写出种子对
python3 exec/c/comboot.py stage 1 $D/unisacc-seed.com
for s in stage2 stage3 fixedpoint; do until SEED_DIR=$D python3 tests/bound.py 58 python3 exec/c/comboot.py shard $s; r=$?; [ $r -ne 75 ]; do :; done; [ $r -eq 0 ] || break; done   # 3 N22；R1：超 40 s 停在步骤边界退 75，同命令续跑
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

**主检出的 unisacc.com 要跟上版本**（主人 2026-10-04）：0.0.23 发版后，主检出根目录的 unisacc.com 还是 0.0.22，所有 --com 门禁和日常 `./unisacc.com` 用的都是旧版。新增一步：**本地发版队列 rc=0、Windows 签名核对通过之后，公开发布之前**，把候选的 `unisacc-next.com` 和 `unisacc-next.com.build.json` 成对装到主检出根目录（改名为 unisacc.com 和 unisacc.com.build.json），先跑 `exec/c/provenance.py check`，再跑 `tests/subtractsafety.py`。装的是**未签名的候选**：签名件的字节和 build.json 记录的不一致，新鲜度检查过不了。不管最后公开不公开，这一步都要做。同时把 `cand/seed/unisacc-seed.com`(.build.json) 成对装到仓根备用（主人 2026-10-06：种子仍 gitignore，但与 unisacc.com 一样留在仓里）。**工具**：`release/tools/install_root.sh <cand>` 一步完成两对拷贝、model-audit、provenance、subtract-safety 和版本核对（0.0.31 起）。**核对**：装完后 `./unisacc-seed.com --version` 与 `./unisacc.com --version` 必须同版本（0.0.30 发版漏装种子，根目录停在 0.0.29；10-06 从 v0.0.30 标签用 build_seed.sh 补建）。如果在这之后产品闭包又变了（0.0.24 的 T3 就是这种情况），候选不再新鲜，就要在当前 main 上重新构建一份再装。顺序：队列 rc=0 加签名核对通过 → 把候选装进主检出 → 请主人确认 → 公开。

**流水线（缩短总时长）**
- 发版队列一开跑，不依赖它的步骤就并行做：release_prep 加 Apple 签名和公证（基本是等网络）、草稿 release、release-check（推送 rc 提交时就触发）、Windows qualification 和 company 签名。0.0.23 在队列还没跑完时，这几步就都完成了。
- 公开发布仍然要等：本地队列 rc=0、Linux 全绿、crossnative 没有跳过，并且主人确认。

## 17. 0.0.24 回顾（2026-10-04）

总耗时约 8h22m（11:48→20:10）：开发 5h33m，三次封装 64 分，队列约 49 分，平台验证约 20 分。全文见 [research/r24-pipeline-retro.md](../research/r24-pipeline-retro.md)，改进作为流程项 P1–P7 写在 archive/plans/v0.0.25.md。下次发版前对照：
- **版本号提交之后只收修红的改动**：kill、ttyname 和参考侧的 extern 修复都是封版前十分钟进来的，引出了三次封装、首轮队列中断（66/568）和首轮 Windows 签名作废。
- **改了头文件，就同批重生 kernel 和 exec/facts**：precheck 现在会跑 export --check。
- **difftest 两条路线都要跑**：门禁带着 UA，只测 C 路线；difftest-py-1..4 补上了 Python 路线。前端一改，就扫描所有 knownfail 里复活的条目。
- **不要用只带 target_commitish 的 PATCH 去改草稿 release 的目标提交**，这会把 tag_name 冲成 untagged，qualification 随即报 “seal must be exactly one existing owner draft”。要改就把 tag_name 一起带上。
- **comboot 被看门狗杀掉后会留下锁**：每次重试前 rmdir 掉 comb-build/<stage>.lock。如果 stage2 还没完成，stage3 会拿主检出里的旧 unisacc.com 去构建，所以必须先删掉 stage3 目录（X6）。
- **在独立工作树里续跑队列时**，不要直接改工作树里的文件（release.sh 会拒绝脏树）。先在 main 上提交修复，再在工作树里 `git checkout --detach <新提交>`，从 results.json 删掉失败的条目，然后用同一个 QUEUE_WORKTREE 续跑。

## 18. 0.0.25 起的发版顺序（P4、P5、P7）

1. **封完立即开队列（P5）**：candidate.json 封装、gatedeps、rc 标签一完成，就启动 `release/tools/queue.sh`。队列跑在自己的 detached 工作树里，所以收口文档、草稿 release、release_prep 和 Apple 签名、Windows qualification 与 company 签名都在队列运行期间做。
2. **（0.0.25 实测后修订：默认串行）** **平台验证与队列并行（P4）**：队列开跑的同时，后台启动 Linux 全套（`LIMA_VM=minicon-lnx-aarch64 SUITE_LIMIT=55 JOBS=1 ./tests/linux.sh all`）。Windows 部分不再开虚拟机，以 release-check 的 winsuite 为准（X7），回执里写 release-check 的 run id。0.0.25 把两者并行，负载到 9 以上，机器发热到有焦味，所以**默认先跑完队列，再跑 Linux 全套**；只有机器明确空闲、负载低于 3 时才并行。
3. **字节不变的重封沿用队列状态（P7）**：如果重封只改了不进产品字节的文件（例如 unisa/ 的 Python 前端），候选字节和 artifact_sha256 不变，只有 sources_sha256 会变。这时**在同一个 CAND 目录里重建**，让 queue.sh 沿用 CAND.queue 状态，release.sh 只重跑依赖声明命中改动文件的作业。换目录会改变 MODEL_COM 路径，指纹随之全变，整轮重跑。0.0.24 第三次封装就是换了目录（cand3），所以重跑了全部 568 项。

## 19. 0.0.27 回顾（2026-10-05）

全文见 [research/r27-retrospective.md](../research/r27-retrospective.md)。版本号提交到公开用了 2 小时 29 分，其中返工约 1 小时。以后照下面几条做：

- **封存前，release-check 和契约层都要先绿**。这一版 rc 挪了两次，都是封存后才暴露的工具红：ledgercheck 读错了计划，gate-layers 漏了分层，workflow 里的 bound 写成 120。只要不进产品闭包，用 fix: 修、重打 rc 是安全的：候选字节不变，队列按指纹复用结果。但这是返工，0.0.28 R2 改成在 precheck 里跑契约层。
- **从 rc 工作树启动队列时，被 git 忽略的 corpus 不会跟着过去**。queue.sh 从 8ef0cdc1 起改为从主检出链接全部语料。
- **负载看门狗要盯 queue.sh 本身的 PID**：同时启动会抢跑，取到外层 shell 的 PID 会把 queue.sh 留成孤儿。0.0.28 R1 把看门狗做进 queue.sh。
- **队列期间不跑 make gatedeps**：机器满载时它跑不完 58 秒。门禁登记攒到队列结束后统一做。
- **Linux 全套**跑 4 GiB 的 `default` 客机，结论认“only guest-known reds”那一行（bigclosure、ape 缺内存，warn 缺 clang）；跑完清掉客机里保留的 TMPDIR。
- **Apple 签名可以和 Linux 全套并行**：它不占本机 CPU，约 2 分钟。
- **公开后同步到 UNISA 入口仓**（0.0.28 U1）：`gh release create unisacc-v$V -R partnernetsoftware/unisa` 挂签名后的 unisacc.com、dmg 和 SHA256SUMS.txt（在文件所在目录里生成，文件名不能带路径），重新下载用 `shasum -c` 核对；然后在 unisa 仓的 releases/unisacc/README.md 加一行。二进制不进 git 历史。

## 20. 0.0.28 回顾（2026-10-05）

全文见 [research/r28-retrospective.md](../research/r28-retrospective.md)。要点：

- **precheck 的契约层放在 comboot 之后、封存之前**：版本号提交后先 `make gatedeps`（否则 gate-infra 红），ape-version 要等 stage2 装上新产品。
- **POSIX 宿主行为探针放 tests/forward，不放 tests/c**：tests/c 被 difftest-py、ccrun、fat、closure、chain 都消费，Windows 目标与 Python 对照组跑不了 environ/fork/exec。
- **Python 路线的缺口各有清单**：pyfront.knownfail（fat）、difftest-py.knownfail（difftest-py）、ccrun.knownwrong（ccrun）；precheck 的本版号 knownfail 规则不看 pyfront。
- **签名部署批准可能被网络吞掉**：批准后查 `pending_deployments` 长度为 0 再等。
- **队列复用目前失效**（P7 实测 0%）：0.0.29 P7′ 修好之前，修一次红就要预计整轮重跑。

## 21. 0.0.31 回顾（2026-10-06，发布前先记）

- 根目录的 `.com` 必须与 `.build.json` 成对。0.0.30 收尾时拷进了签名版，旁边却是未签名包的 json，结果 0.0.31 队列拒绝启动。现在 queue.sh 一律从 CAND_DIR 装入候选的那一对（0d0ab8f8），收尾改用 `install_root.sh`。
- 签名前必须有草稿 release，否则资格签名会报 release not found。现在 `release_prep.sh` 发现缺少草稿时会自动创建。
- 不要改正在运行的驱动脚本：bash 边跑边读文件，0.0.31 队列结束时因此报了语法错误。要改就改一份副本。
- 改了 `tests/all.sh`，要接着跑 `tests/ciplancheck.py`。nativeboot 分片后，release-check 因套件改名而变红。
- exec/ 下的检查脚本也算产品来源闭包，候选建好后再改它，候选就会被判过期（X31，0.0.32 解决）。
- 产品 δ 的修复，要先用私有 .com 跑过原来失败的套件，再建候选。候选 2 只凭合成 tape 的字节相等就开建，结果 pthread 运行时段错误。
- 队列耗时：墙钟 47 分钟，其中作业 6820 s（4 路并行的下限是 28 分钟），窗口外开销 487 s，窗口内利用率 74%；csmithdiff 普通版与 com 版重复跑（已去重）。0.0.32 计划：跨候选按输入闭包复用结果、滚动调度、jobs 6 并保留负载保护。

## 22. 0.0.32 回顾（2026-10-07，发布前先记）
- **队列与构建分开起后台**：cand12 第一次跑了 30 分钟就被后台时限停掉；队列循环拆成 qloop.sh 后，续跑直接复用已有状态。长队列要单独起，并给足时限（2 h）。
- **版本号提交要在最后一个开发候选之前**：0.0.32 是先跑开发候选 cand12，再提交版本号、跑 cand13，多跑了一整轮 692 项（约 45 min）。0.0.33 Q1 改为先提交版本号，开发候选就是发版候选。
- **F4 基准钉住 HEAD**：f4time 从 `git archive HEAD apps/csih` 解出源码来编，不碰别人正在改的工作副本（工作副本测出约 5.1 s，HEAD 约 6.0 s）；解到临时目录后，路径必须用绝对路径。
- **push 和 Linux 并行**：本机 Linux 全套跑的同时就 push 触发 release-check（GitHub runner，不占本机）；本次 run 37566384751 绿。
- **代提交**：cdx 撞上 index.lock EPERM 时，由 cc 按路径代提交，等待链不会自己往下走。
- **queuecheck 的真实快照用例要求树已签章**：先提交接线、刷新 gatedeps，再跑 queuecheck。
- **分段 ape 要封完整长度**：R3′ 把数据零尾改成长度（Padded）后，tests/ape_stage.py 写 data.bin 只剩前缀，Linux 分段 ape 段错误（139）；record 现记 data_len（schema 3）。单跑 ape 走直接构建，所以只在 all 里红。
- **linux.sh all --suite NAME**：Linux 红项单独重跑（ape 14 s、native8/ccrun8 各约 15 s），不必再跑 1063 s 整轮；全套中 native8/ccrun8 的 55 s 超时单独复跑为绿，属负载。
- **未签名回执绑 rc 提交的 release-check run**：0.0.32 先按 20148939 的 run 做回执，封存推送后 rc 指向 28c850e6，资格签名断言 wrong upstream source 失败；改用 28c850e6 自己的 run（37572481970）重做回执（zip 字节不变）后通过。顺序：封存 → push → rc_tag → 等该提交的 release-check 绿 → unsigned_receipt.py → 上传草稿 → 资格/公司签名（scratchpad qual32.sh 一条链）。

## 23. 0.0.37 开发提速（政委 10-09 吸纳 cdx2 只读评审；不改验收）

依赖顺序不变：同源身份 → 候选 → 自举/契约 → 封存 → m4pro 全门禁 → 信任与发布。只重叠互不争用的等待。不做：加 jobs、放宽 60 s/55 s 预算、自动重试到绿、Linux 绿顶替 m4pro 全门禁。

1. **队列只用 `release/tools/queue.sh`**：它自带 `TERM_SH_NOFALLBACK=1`、同源候选对（stage 2 的 `.com` 与 build.json 配对检查）、seed/UA/提供者检查；0.0.37 起另有：同机已有 queue.sh 时拒绝再起第二个，macOS 上用 `caffeinate -dims` 保持屏幕与空闲不休眠（0.0.36 两次 539→15、713→11 大面积失效即屏幕休眠与手写续跑缺 NOFALLBACK）。不再手写续跑循环；续跑就是重跑同一条 queue.sh 命令。
2. **候选前清单（precheck.sh 先跑）**：版本提交、顺延账 `--final`、冻结窗、事实导出、提供者、已知红、契约层之外，0.0.37 起再查：构建/测试树内未提交或未跟踪的输入（plans/prd/research/archive/docs 不算）、`corpus/c-testsuite` 是否在位。任一项红先修再冻结，避免封存后重封与整轮重跑。
3. **状态与日志的目录外持久备份**：queue.sh 每窗结束（无 gatequeue 在跑）把 `CAND_DIR.queue/` 与候选 build.json 复制到 `QUEUE_BACKUP`（默认 `~/.unisacc/queue-backup/<候选目录名>-<候选 sha 前 12 位>/state`），整份复制成功后才替换旧份。重启时若状态目录不在而备份在，自动恢复；gatequeue 仍逐项核对输入指纹，只有身份完全相同的结果才复用。
4. **签名等待时重叠 Linux 预验**：同一候选身份稳定（封存后、字节不再变）时，Apple 公证/Windows 远端签名的等待窗里跑原生 Linux 对应套件（Lima `default` 或云机原生）。只在 m4pro 本机四路队列不在跑时让同宿主 Lima 起重任务；签名后核对 payload 字节与签前一致。Linux 结果是预验，不替代 m4pro 全门禁或六格 runner。
5. **日常阶段定向 + chain，再构正式候选**：开发中先用 `tests/stagerun.sh` / `gate.sh --suite` 跑改动所在阶段与 chain 组，人工补 `also` 标签与下游（`--stage` 只按主标签选，不含 also）；修红只重跑受影响项，沿用固定 CAND 与 GATE_STATE。阶段绿不签发 release；正式候选仍跑 `release.sh --com` 全量。

## 24. 构造器改动后的候选顺序（政委 10-09；cdx2 v4）：全矩阵预验 → 一次冻结 → 自举/队列

动因：0.0.37 云机先修 seed-gen 一处泄漏就整链重建，到 pack-prep-1 才发现 `parse2 --errors` 另一处 5.4 GB 峰值，白跑一次 UA/shared/六 target。改为：

1. **生成器全矩阵预验（不建候选）**：seed/gen.c、seed/facts.h、exec/build/ 任一改动后，先对候选实际调用的**全部** stage/flags 组合各跑一次——shared 七个（pp --shared-predefines、lex --typed、parse2、opt --o2、opt、prune、nativeabi）、六个 `lower --full [--osx|--win] [--arm64]`、pack-prep 十一个模型作业（buildcompiler.sh `modeljob`：parse2 --errors、parse2 --warnings --errors、parse2/units --locations、lex --locations、lex、pp --shared-predefines --no-autoinc、pp --locations --shared-predefines、lower --full --object [--arm64]、enc --object、enc/arm --object）。每项：AddressSanitizer 无错；输出与 `exec/build/gen.py` 同 flags 逐字节相同；记峰值 RSS（`/usr/bin/time -v`）与墙钟，峰值须低于宿主可用内存（云机 16 GB 无 swap，常仅 3.5–5 GB 可用），单项墙钟在 `b` 的 50 s 内。
2. **一次冻结**：矩阵全绿后才提交并冻结，从该提交**只建一次**候选（build_ref → build_candidate → seed）。
3. **再自举与队列**：comboot 定点 → queue.sh 全量。
4. **禁止**每修一处就整链重建；矩阵中途发现新红，回到 1，不起候选。候选建成后发现构造器红，同样先回 1 预验全矩阵，再冻结重建一次。


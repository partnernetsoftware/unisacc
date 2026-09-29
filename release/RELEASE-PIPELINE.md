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
- Windows：`utmctl start minicon-win-arm-64`，`./tests/term.sh env UA=… STRICT=0 ./tests/crossnative.sh examples/*.c`（全探针超 60 s；utmctl 的 AppleEvents 只在 Terminal 内可用）。`nativeboot.sh --windows` 同样要 Terminal。用后 `utmctl stop`。

## 5. CI（一次 push）
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
签后产物从 run artifact `unisacc-company-signed-<attempt>` 下载，核对签前/签后 SHA 与尺寸，上传草稿，回执写入 `research/r<N>-release-acceptance.json`，`gh release edit --draft=false`。之后再提交回执/文档。

# unisacc 0.0.91 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | a7b8ffe328f89f9ed18e092fcedcd3e76b1f587e |
| 候选提交 | 7be3cc5d4b03e1b3b281a372ac26b9da19c18b44 |
| 发布 APE SHA256(unisacc.com) | eb18eb439db1ffd4640099bd53afae039d00507745fbb3f476d703bdcb2938e8 |
| 源快照 SID | 967dbd907158db1a81ee503a842ba089d2fca024842eff6a06320da5a51f0278 |
| pkg.bin | b717fff8f266d4c3c924186657be94cdbbc1c470bdcbe1a6af7ef97bb1cb8eaa |
| slice-win-arm64 | 73bbd2eaacad256bd5ce51b1f0a6d51b6dcb9af36ed38be78ca5d5620e3416e0 |
| SHA256SUMS | b6362d7fab330f66357f155287fbf6644d98524e8aaa8f05aaf6e555f795a6c4 |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

- 严格预跑(148 窗, 459 批, Candidate APE `eb18eb439db1ffd4640099bd53afae039d00507745fbb3f476d703bdcb2938e8`, `frozen-root` 绑定候选提交 `T=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44`, `T..F` 零 A 类变动、同 `SID=967dbd907158db1a81ee503a842ba089d2fca024842eff6a06320da5a51f0278`):
  - 首轮执行(`attempt 1`): `W0002..W0147` 共 146 个窗口一次性完成(`W0098` 经 `serial-recovery` 自动收口 `rc=0`), 首波 `W0000`(`opt-frame4-o2-negative-label`)与 `W0001`(`lane-key-probes`)因启动初期冷编并发撞 58s 记未完成, 首轮末尾输出:
    `PRERUN_RUN	status=INCOMPLETE	ran=146	skipped=0	missing_windows=2	out=/tmp/unisacc-session-v0.0.91-prerun-7be3cc5d4b`
  - 续跑补窗(`attempt 2` 与 `--resume`): 外层循环自动进入 `attempt 2 --resume` 完成 `W0000`(`PRERUN_WINDOW window=W0000 phase=warmup attempt=1 jobs=1 rc=0 wall_ms=4439`, `PRERUN_WARMUP_READY window=W0000 ids=opt-frame4-o2-negative-label layout=ready kit=ready`), 随后外层 `timeout 3600s` 到点退出; 立即以 `--resume` 补跑最后 1 个窗口 `W0001`(`PRERUN_WINDOW window=W0001 phase=long attempt=1 jobs=1 rc=0 wall_ms=21930`)并收口:
    ```text
    PRERUN_RUN	status=DONE	ran=1	skipped=147	out=/tmp/unisacc-session-v0.0.91-prerun-7be3cc5d4b	window_wall_ms=21930	sum_batch_ms=20057	outer_overhead_ms=1873
    PRERUN_MERGE	windows=148	batches=459	PASS=457	FAIL=0	UNKNOWN=2	SUPERSEDED=0	out=/tmp/unisacc-session-v0.0.91-prerun-7be3cc5d4b/strict-final.tsv
    PRERUN_ARCHIVE	version=0.0.91	windows_bytes=12933	log_bytes=20348	home_paths_redacted=0
    ```
  - 2 项 `UNKNOWN`: `cx-booter-ape`、`tools-default-regression`(均为已知 `no-producer` 无生产者批; `product-qualification` 批 `0 FAIL`)。
- 候选基点 `T` 与冻结提交 `F` 双基点 17 项棘轮并列实测(证据文件 `release/evidence/0.0.91/ratchets-T-7be3cc5d4b.txt` 与 `release/evidence/0.0.91/ratchets.txt`):
  ```text
  [T 候选点] SOURCE	frozen-commit	commit=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44	tree=git-archive -> RATCHETS	ok=17	fail=0	elapsed_ms=24051
  [F 冻结点] SOURCE	frozen-commit	commit=a7b8ffe328f89f9ed18e092fcedcd3e76b1f587e	tree=git-archive -> RATCHETS	ok=17	fail=0	elapsed_ms=5686
  ```
- 六目标原生 CI:
  - `ci-early` `run_id=44989022233`(`rev=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44`, `ape_sha=eb18eb439db1ffd4640099bd53afae039d00507745fbb3f476d703bdcb2938e8`): `lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 六格一次性 `6/6 PASS`(`pass_cells=6, fail_cells=0`); 正式发布 CI `run_id` 待阶段 B 执行后回填 `release/candidate.tsv` 与 `release/evidence/0.0.91/INDEX.tsv`。
- 起链前置 20 项实测(`REV=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44`, `sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.91`):
  - `2a-derive PASS (DERIVE_SUMMARY ok=4 drift=0)`、`2b-ratchets PASS (RATCHETS ok=17 fail=0)`、`3a-cxmod-tri PASS (44/0)`、`3b-cx0-extract PASS (EXACT bytes=118131)`、`4-p1 PASS (2601 files, new-text rejections=0)`、`5-candidate-chain PASS (137/0)`、`11-version-slot PASS (version=0.0.91)`、`15-gen-all PASS (44/44)`、`19-chain-preflight PASS (a_files=19/20)`、`20-ident-helper PASS (identity-helper-real5)` -> `READY yes failed=none`。

## 产品改动

- 主题与变更口径披露(主题: 身份与构建边界, 基点 `v0.0.90..7be3cc5d4b`):
  - 机械口径(`19-chain-preflight` 对照 `build/lane-inputs.tsv`, 排除 `exec/`、`kernel/`): `19 / 20`(`v0.0.91` 分支 18 个 A 类文件 + 冻结收口 `7be3cc5d4b` 修正 `Z5` 迁出后悬空目录软链 `ext/fsx/path` 1 个);
  - 手工口径(含 `Z5` 删迁的 13 个旧 `cx/` 路径、`exec/` 0 个、`kernel/` 0 个): `32` 个 A 类相关路径(19 个在册 A 类 + 13 个删迁旧 `cx/` 路径; 另 `tests/workspace`、`tools/workspace` 为非 A 类目录软链)。
- L.1 源身份升级 `source-packages-v3`:
  - `seed/ident.c` 升级为 `source-packages-v3`: `cx/` 与 `src/` 下指向 7 棵产品树之外的类甲符号链接仅记录别名拓扑、不再把外部目标拉入源身份(`ext/textx/{build,json,num}.cx` 退出 SID); `build/core-closure.tsv`、`cxroot.tsv` 与打包支持源 `ext/textx/deflate.cx` 强制纳入支持集合; `collect()` 改为单一返回出口;
  - `tools/lane-inputs-gen.cx` 增加 `build/core-closure.tsv` 全部真实单元必须在 SID 中的硬断言, 并使 `ext/` 下所有非 SID 常规文件归 `lane B ext-non-sid`;
  - 新增并登记守门批次 `ident-v3`(`tools/check-ident-v3.cx`, 先红 `15/21` -> 转绿 `21/21`, 证据入仓 `plans/v0.0.91-l1-evidence.md`), 独立双层包 oracle `tests/identity.cx` 同步升级(`303/303 PASS`)。
- Z5 层次整理收尾:
  - 删除 `cx/text/{build,deflate,json,num}.cx` 4 条类甲反向软链与 `cx/workspace/{mem,sys}`, 将 `cx/stream/rw.cx`、`cx/path/path.cx`、`cx/os/{osx,utc}.cx`、`cx/workspace/*` 迁入 `ext/`(`build/product-links.tsv` 类甲软链清零: `total=18 class_a=0 class_b=18`), 并在 `7be3cc5d4b` 修正 `ext/fsx/path`、`tests/workspace`、`tools/workspace` 3 处目录软链;
  - `cxroot.tsv` 更新模块映射并保留 `text.{build,deflate,json,num}` 兼容映射供 `apps/csih` 28 个文件过渡;
  - `ext/path/path.cx` 改用 `sy_os_name()` 消除 `#ifdef _WIN32`, `ext/os/{osx,utc}.cx` 与 `ext/path/path.cx` 去除无用 `#pragma unisa dock`。
- B3 收回无用 `dock` 授权与 `ext-dock-exceptions.tsv` 清零:
  - 收回 `cx/` 下 12 个零宿主依赖(`includes=0, libc_calls=0, plat_if=0`)文件的无用 `#pragma unisa dock`(含改 `tools/cap-gen.cx` 与 `tools/refusals-gen.cx` 生成器), 判定表入仓 `plans/v0.0.91-b3-verdicts.tsv`, `build/dock-audit.tsv` 降至 42 行;
  - `ext/` 下 20 条 0.0.90 到期项已在 `main`(`5a36143766`)清零处置(14 个去 `dock`、6 个带 `0.0.93` 迁移条件续期), `check-ext-dock.cx -- --version 0.0.91` 实测 `failed=0 expired=0`。
- 字面量长度注解(`litlen` A 类 3 文件):
  - 并入 `cx/runtime/source_frame.cx`、`cx/sys/sha.cx`、`src/drive.cx` 3 个 A 类文件的字面量长度注解。

## 构造过程说明

- 候选整链(`REV=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44`)最终 `499/499 DONE` (`PROMOTE_CHECK OK rev=7be3cc5d4b03e1b3b281a372ac26b9da19c18b44 phases=499/499 inputs=11`, `EXIT 1795324523 pid=926647 rc=0 why=exited`):
  - 候选 APE `eb18eb439db1ffd4640099bd53afae039d00507745fbb3f476d703bdcb2938e8` (`8991222` 字节), `source_sid(source-packages-v3)=967dbd907158db1a81ee503a842ba089d2fca024842eff6a06320da5a51f0278`; 6 个目标切片(`lnx/x86_64 13d94c1b`, `lnx/arm64 d37c2d81`, `osx/x86_64 ca5523db`, `osx/arm64 b78c52b9`, `win/x86_64 7fbf1373`, `win/arm64 54bb23de`)、`p3-package.body` (`21a77765`) 与 `cache-manager` (`847748bb` / `f41542f3`) 全部与 `0.0.90` 逐字节恒等;
  - (1) `12:27` 首次起链在 `17.5s` 退出(`rc=2`): 因 `R72` 收紧 `sessionroot` 路径白名单后, `tools/bg-candidate.cx` 默认 `--base` 仍指向 `$TMPDIR/unisacc-bg-b1-$UID` 被拒; 在 `0a9b7b9d31` 将默认 base 迁至 `~/.cache/unisacc/chains/bg-b1` 并补 `check-bg-candidate` 正负控后于 `12:38` 重起链;
  - (2) `13:06` 首轮推进至 `495/499` 的 `pack` 相位时, `--resume --until pack` 被 `tools/candidate-chain.cx:3114-3117` 起点门拒绝(`rc=2`, 起点门在未传 `--cache-manager-osx` 时要求 `<--out 父目录>/src/tools/cache-retain.cx` 存在, 未认可链内已完成的 `DONE cache-manager` 阶段产物 `<out>/cache-manager/{x64,arm64}`; `0.0.90` 因手工建了 `<out 父目录>/src` 未暴露, 若追加 `--cache-manager-osx` 参数则会触发 `chain-run` 的 `identity mismatch`);
  - (3) 修复 `tools/candidate-chain.cx` 起点门后, 执行 `chain-run.cx -- --repin candidate-chain` 重钉链工具 `candidate-chain`(重钉的是链编排工具而非产品输入, 已完成 495 个相位完整保留), 续跑一次完成 `499/499 DONE`; 修复提交与先红后绿夹具已入 `main`(`b42e146deec0ff3f338b1a6b82dd07dd4010dcb4`, 实编二进制 `sha256=4f773f95775671e205ef3abb5921e7ee279d4f48e8733fdba7221c96e053abcc` 与 `TOOL_REPIN new_sha` 逐字节一致), 链内 `TOOL_REPIN` 原行:
    ```text
    TOOL_REPIN	status=landed	time=1795324349	tool=candidate-chain	prior_sha=7c523882b72a80224d5c46264fc62affbe9cb4b4a8661a091226807188c261fc	new_sha=4f773f95775671e205ef3abb5921e7ee279d4f48e8733fdba7221c96e053abcc	reason=accept completed in-chain cache-manager phase on --resume --until pack
    ```

## 已知缺口

- (1) 全套 `s9` `daily`/`release` 长跑尚未运行, 整体裁决保持 `NOT_QUALIFIED`;
- (2) `R76`(`ext/net/httpd.cx` B 类明文 HTTP 服务端)归 `main` B 类 `ext-non-sid` 车道(不进 `0.0.91` A 类 SID; 全 9 切片 `0f61a25f26` 与 `ccogw` 独立差分 `moltbaby 99c704b14` 已在 `main` 完成 `DONE`, 但作为九个 `.ts` 代理 drop-in 替代的前置缺口为 `R83` `httpd` 请求头枚举接口 `hs_req_header_count`/`hs_req_header_at`);
- (3) `tools/tsv-lint.cx` 未解析引用已知红(`R75`): 本版候选 `REV=7be3cc5d4b` 中 `tsv-lint` 仍为 `unresolved=11351 > 11050` (`TSV_LINT files=567 rows=28985 errors=1 warnings=11352 states=5584 checked=5584 unresolved=11351 status=ERROR`), 已排 `0.0.92` 治理并纳入棘轮门;
- (4) `asm volatile(...)` 行内汇编拒绝位置缺口: 输出 `reject: not covered: inline asm` (`rc=1`, 缺 `file:line:col` 行列号, 已排 `0.0.92` 迁 `E3` 修复);
- (5) `R69.6` 零断言守卫第一阶段影子计数已在 `main`(`22b2c76688`)落地, 尚未开启强制拒绝(第二阶段十批试点排 `0.0.93`, 最早 `0.0.96` 对新批强制);
- (6) `devsvc` 后台有界长作业入口缺口: `devsvc x --bg` 受 `FL_NODE_MAX_MS=55000` 限制无法直接承载超 55 秒的整链/预跑/ci-early 长作业(现走 `release-chain` 的 `setsid nohup timeout` 配方), 已记入 `prd.md` 候选池 T 排 `0.0.92` 后迭代。

## 回滚与撤回命令

- 若在阶段 A 提交(`X`)后、公开前终止发布:
  - 删除远端草稿 Release(若已创建): `gh release delete v0.0.91 -R partnernetsoftware/unisacc --yes`
  - 回退 `main` 上的阶段 A 提交: `git revert --no-edit <X_COMMIT> && git push origin main`
- 若在公开发布后需撤回 `v0.0.91` 并恢复 `v0.0.90` 为 Latest:
  - 将 `v0.0.91` 降回 Draft 并恢复 `v0.0.90` Latest:
    `gh release edit v0.0.91 -R partnernetsoftware/unisacc --draft=true && gh release edit v0.0.90 -R partnernetsoftware/unisacc --latest`
  - 本机 Root 回滚至 `0.0.90` 备份件(`sha256=46fd3db95f97df7fa4729026bd6e1aa7b7ddde834e5887aa87470020da0391e1`):
    `cp -f ~/.unisacc-release-evidence-backups/0.0.90/chain/ape-final/unisacc-next.com ./unisacc.com && sha256sum ./unisacc.com`

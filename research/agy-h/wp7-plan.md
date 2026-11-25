# WP7 拆批与冷态处置方案(agy-h,经 cdx 独立复核修正,未跑冷态)

依据 [margin-packages.md](margin-packages.md) 第 2 节工作包定义,对 WP7 全部 6 批重读入口源码、核对在飞分支(`v0.0.107` 头 `d750ad123c` / `v0.0.125`),并吸收 `cdx` 独立复核修正判型与边界。历史耗时均取自 [margin-audit.tsv](margin-audit.tsv) 与 `tests/commit-tier-timings.tsv` (`out/ltests-evidence/m8-remainder-02.log`, 旧 39 秒 watchdog 截断 UNKNOWN, 6 批均无现行 `batch-times.tsv` 单跑实测)。旧 39s 窗 UNKNOWN 仅用于排队,不能证明各批现行超限或某相占比。

判型定义沿用 [margin-packages.md](margin-packages.md): `(A)` 每片重编闭包/材料或重复启动, `(B)` 真实工作量大, `(C)` 依赖第二个工具链启动。

## 1. WP7(6 批) 逐批诊断与处置总览

| 批 | 层级/预算 | 历史现测 | 根因判定(已核) | 证据(源码行与登记) | 处置(材料相 / 分片 / 规则) | 预计冷态 | 写域与冲突 |
|---|---|---|---|---|---|---|---|
| `cx-procguard` | `daily/55` (`serial`) | `40.076s UNKNOWN` (`commit-tier-timings.tsv:172`) | `A + B`: 10 次子进程重复启动 `procguard.cx`, 另建多组 `sleep` 子树与 9MB 日志轮转 | `tools/check-procguard.cx:54-62,87,111,229` 10 次起 `sh unisacc.com tools/procguard.cx`; :86,163,167-172,202,214 固定睡等与最多 5 轮 400ms 重试; :226-227 9MB 轮转; :152-154 默认入口已含 `runaway_probe`, `--probe` 只跑该段; `tests/wave-domains.tsv:14` 标 `serial` | 材料相先行: 接 `procguard-tool:path` (`affected --prepare-materials` 预编) 并在入口复用 10 次调用; 入口加 `--no-probe` 开关供后续拆片时主体跳过 `runaway_probe` (避免与 `--probe` 重复跑 `probe`); 若复测仍 `>27.5s`, 拆为 `--no-probe` 主体与 `--probe` 两片, 两片并在 `tests/wave-domains.tsv` 继承 `serial` | 待复测(推断降冷启开销) | `tools/check-procguard.cx`、`tests/batches.tsv`、`tests/wave-domains.tsv`; 冲突中(`v0.0.125` 改 `cx/sys/proc.cx` / `reaper.cx`) |
| `unit-boundary` | `daily/55` | `39.224s UNKNOWN` (`commit-tier-timings.tsv:209`) | `B`: 9 个 fork 全执行同一被测产品 `{root}/unisacc.com`, 夹具生成、多单元编译和运行均属真实工作量 | `tests/unit-boundary.cx:43-66,79-100,187-201` 编跑 `{1,2,63,64,65,72,127,128,129}` 共 651 单元; `v0.0.107` `d750ad123c` 读 `FRONT_UNIT_MAX` (`256`) 动态改为 6 小组 + `max-1/max/max+1` 共 1035 单元; 注意 :70 `char cmd[12000]` 缓冲在 `257` 单元+长 `TMPDIR` 路径下的长度上限 | 只分片(等第二车 `cxm-maxu-256` 落库后): 先核对 `cmd[12000]` 在 `cap` 夹具和实际 `TMPDIR` 路径下能否传齐参数, 再冷测 1 次; 若超 `27.5s`, 加 `--part small\|cap` 拆两片(`1,2,63,64,65,72` 与 `max-1,max,max+1`), 保留 `cap+1` 实际拒绝断言与 `args={root}` | 待复测后定 | `tests/unit-boundary.cx`、`tests/batches.tsv`、`tests/batch-product-inputs.tsv`; 冲突高(`v0.0.107` 已改本文件) |
| `source-bom` | `daily/55` | `39.092s UNKNOWN` (`commit-tier-timings.tsv:263`) | `B + C`: 产品主体高频调用约 49 次, 镜像相另起 `cc`/`gcc` 编 4 单元建 `bom-host-driver` | `tests/check-source-bom.cx:45-48,51-68` 每步 `bound 12`, 总限 52s; :116-145 无第 5 参数 `HOST_CX_DRIVER` 时 `build-host.sh` 调 5 次 `cc`/`gcc` 编 4 源建 `bom-host-driver` 并对拍; :159-163 已原生支持 `PRODUCT ROOT NEW_OUT - HOST_CX_DRIVER`; :168-203 执行约 49 次 `PRODUCT` 调用 | 按 `B + C` 分片: 先解冻单测 1 次摸底; 若 `>27.5s`, 加 `--part core\|mirror` 拆 `source-bom` (E1/E2/BOM + `pair` 主体) 与 `source-bom-mirror` (镜像对拍); 镜像片可接 `PRODUCT ROOT NEW_OUT - HOST_CX_DRIVER` 供材料省 `cc`, 因主体本身另有约 49 次调用, 主体须单列测量与拆留评估 | 待复测后定 | `tests/check-source-bom.cx`、`tests/batches.tsv`、`tests/batch-product-inputs.tsv`; 冲突中低(`v0.0.107` 改 `src/cxmod.c`) |
| `clock-targets` | `daily/55` | `39.086s UNKNOWN` (`commit-tier-timings.tsv:262`) | `A + B`: 2 次冷启生成器 + 6 次跨目标编译全使用同一已装 `unisacc.com`, 目标枚举属真实工作量 | `tools/check-clock-targets.cx:34-38` 2 次 `np_run` 跑 `tools/clock-targets-gen.cx --check` (含 `drift.h` 负控); :39-57 对 6 目标(`lnx/osx/win` x `x86_64/arm64`) 均调同一 `unisacc.com -b target -I inc -c -funit`, 单步 `np_run` 限 55s | 先解冻单测 1 次摸底; 可接 `clock-targets-gen-tool:path` 复用生成器执行体; 拆片重点为按平台均衡拆分 6 目标为 `3 + 3` 两片, 生成器的正常检查与 `drift.h` 漂移负例只归一个分片(不重复) | 待复测后定 | `tools/check-clock-targets.cx`、`tests/batches.tsv`、`tests/manifests/clock-targets*`; 冲突无 |
| `p3-swap` | `daily/55` | `39.059s UNKNOWN` (`commit-tier-timings.tsv:201`) | `A + B`: 源码确认 6 处重复启动 `tools/lab-loader/p3-swap.cx`, 另有 APE 解包、bad-crc 解码与 `utf8.net` 比对 | `tools/lab-loader/check-p3-swap.cx:253,277,285,308,316,486` 共 6 处 `np_run` 执行 `sh ./unisacc.com tools/lab-loader/p3-swap.cx` (重复启动, 无逐次编译实测占比); :33-66 若 APE 布局另启 `--version`; :384-397 S4a bad-crc 解码; :406-505 S6 `utf8.net` | 材料复用优先: 入口引入 `cxlib.toolprep`, 批首调 1 次 `tq_native(ps_root, "tools/lab-loader/p3-swap.cx", p3_bin, ...)` 预编原生体后直接执行 6 处 `p3-swap`, 严格保留输入、退出码和 stderr 合同; 复测若仍需拆片, 按变更成功(S1/S2/S3)、损坏拒绝(S4)及边界/编码(S5/S6) 逐条归属, 避免重复计入 S4 | 待复测(复用消除 5 次驱动启动) | `tools/lab-loader/check-p3-swap.cx`、`tests/batches.tsv`; 冲突无 |
| `cx-host-run` | `daily/55` | `38.084s UNKNOWN` (`commit-tier-timings.tsv:144`) | `C + B`: 单批内用宿主 `gcc` 与 `clang` 各自编译并跑全矩阵, 外加跨宿主 `-cx-plan` 对拍 | `tests/cx-host-run.cx:1032-1061,1104-1107` 调 `gcc`/`clang` 编 8 `.o` + 2 链; :1063-1087 跨宿主 `parity_cases` 必须同时拿 `gcc_cx` 与 `clang_cx` 两份驱动; :1109-1112 跑 `unisacc_parity` (仅需 `gcc_cx`) 与两侧完整 `matrix()` | 按三片设计约束分片(双驱动对拍直接单列, 不作备选): 片 1 `cx-host-run` (`gcc` 构建+矩阵+`unisacc_parity`+`terminal_unit_cases`); 片 2 `cx-host-run-clang` (`clang` 构建+矩阵); 片 3 `cx-host-run-parity` (跨宿主 `parity_cases`, 由材料准备相显式绑定 `gcc_cx` 与 `clang_cx` 双产物及各自宿主身份) | 待复测后定 | `tests/cx-host-run.cx`、`tests/batches.tsv`、`tests/coverage.tsv`、`tests/batch-product-inputs.tsv`; 冲突中(`v0.0.107` `cxmod.c`, `v0.0.125` `proc.cx`) |

## 2. 分类归因总结

(1) 材料复用优先(`2` 批当前可出补丁): `cx-procguard` (接 `procguard-tool:path` 并加 `--no-probe` 开关防后续拆片重复跑 `probe`)、`p3-swap` (批首 `tq_native` 单编复用 6 次原生执行, 保留原 CLI/退出码/stderr 合同)。二者源码均明确存在多次子进程重复启动。

(2) 按平台/三片约束分片(`2` 批): `clock-targets` (`A+B`, 生成器材料 + 6 目标 3+3 拆且生成器仅归一片)、`cx-host-run` (`C+B`, `gcc`、`clang`、双驱动 `parity` 三片固定约束, 第三片显式绑定双驱动材料与宿主身份)。

(3) 只分片且待第二车落库 / 主体单测(`2` 批): `unit-boundary` (`B`, 等 `d750ad123c` `cxm-maxu-256` 落库后先核 `cmd[12000]` 缓冲与实际路径长度再量拆 `small/cap`, 保留 `cap+1` 拒绝)、`source-bom` (`B+C`, `core` 主体与 `mirror` 拆开并单列测量主体, 镜像可复用 `HOST_CX_DRIVER` 第 5 参数入口)。

## 3. 解冻后执行顺序与验收边界

(1) 冷态摸底: 解冻且无后台整链时按 `cold-fresh-layout-33s` (`UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0`, 不关 `E3CACHE`, `-j 1`, 60s 限) 先单跑 `1` 次核实当前冷态, 不凭旧 39s `UNKNOWN` 反推超限占比。

(2) 落库序:
- 材料复用首批(`wp7-material-a.patch`, 不依赖 `v0.0.107`/`v0.0.125` 内容): `p3-swap`、`cx-procguard` (同步在 `v0.0.125` 落地前复核 `proc` 无回归)。
- 无冲突拆片批(若冷态复测超 27.5s): `clock-targets` -> `source-bom`。
- 等 `v0.0.107` (`cxm-maxu-256`) 与 `v0.0.125` 落 `main` 后: `unit-boundary` (核 `cmd[12000]` + 按需拆片)、`cx-host-run` (三片)。

(3) 验收判据: 拆片断言并集等于原批 100%, 无重复断言(避免重复 `probe` 或重复 `S4`), 保持 55s 预算与 `serial` 域, 每片冷态 3 次峰值 `<= 27.5s`。

## 4. 只读核对命令原行

    grep -E '^(cx-procguard|unit-boundary|source-bom|clock-targets|p3-swap|cx-host-run)\b' tests/batches.tsv research/agy-h/margin-audit.tsv tests/commit-tier-timings.tsv tests/wave-domains.tsv
    sed -n '1,20p;54,125p;142,241p' tools/check-procguard.cx
    sed -n '1,105p;171,204p' tests/unit-boundary.cx
    sed -n '1,217p' tests/check-source-bom.cx
    sed -n '1,61p' tools/check-clock-targets.cx
    sed -n '1,70p;250,510p' tools/lab-loader/check-p3-swap.cx
    sed -n '1,25p;565,640p;1030,1117p' tests/cx-host-run.cx
    git -C .claude/worktrees/v0.0.107 diff origin/main..HEAD -- tests/unit-boundary.cx

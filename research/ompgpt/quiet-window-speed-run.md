# T1.4c 静默窗口测速

状态：r1 静默窗已执行，16/64 对象等价且分别为 94.037/114.316 倍，102 对象外限未完成，15 倍门未通过；原行与分相见 [M1 实测](tape-m1-result.md)。历史链目录缺失，重建的 102 件均不等于历史 SHA；`quiet-speed/objects-v010-r1/` 是首份重建的固定字节存档，不是历史恢复。见 恢复核对(私有证据,仓外 ~/.local/share/unisacc/research/ompgpt/quiet-speed/v010-g2-recovery.md) 与 [跨根不一致](object-nondeterminism.md)。

## 一键测速

在 `~/repos/unisacc` 运行，保持当前独占约定。devsvc 负责外层 290 秒及后代收割，内部总预算 280 秒：

```sh
q="$HOME/.local/share/unisacc/research/ompgpt/quiet-speed"
sh ./unisacc.com tools/devsvc.cx -- x --par 1 --timeout-ms 290000 -- taskset -c 10-11 "$q/timer" run "$q" "$q/objects-v010-r1" v010-r1
```

`quiet-speed/quiet-speed.cx` 采用 `LOAD_GATE`：load1 不高于 6.00，且在每档开始、tape 前后复核，无放宽开关。驱动和顺序表仍核原 `materials.sha256`；`v010-r1` 单独绑定 `objects-v010-r1/SHA256SUMS` 的 SHA-256 `905c92185716e8a12c339f3fb79857873838e5432d102b5e7e714f4da09be4f8`，共 102 件、16846672 字节，执行前逐件查 SHA。16 取 `lite.order` 前 16 项，64 取该表全体，102 取 `full.order`。不靠排序或通配符。原生与 tape 共用同一份生成的路径清单；原生输出重新核 SHA 与重复样本，不沿用历史对象的输出 SHA。

每档原生三次预热前样本、一次预热、三次热样本；每次启动新进程并核对输出 SHA，报告两组中位数。`cold_process` 只表示未专门预热的样本组，不声称 OS 页缓存冷。tape 各一次，16、64 外限各 55 秒，102 最多 170 秒，占用剩余预算，不重试。只有退出码与状态均为 0、同源 `memcmp=0`，且洁净窗口真实墙钟不超过原生热中位数乘 15，才给该档 PASS。超时给 `INCOMPLETE_TIMEOUT`，最后标记的 steps 仅为下界；不把 16 或 64 的倍率代替 102 唯一正式速度门。

## 输出与登记

`T14_QUIET job=...` 指向唯一会话目录，`T14_NATIVE_MEDIAN`、`T14_TAPE` 和 `LOAD_GATE` 是控制台原始回执。`tape-N.mark` 为原始记录；`tape-N.phase.tsv` 列对象/阶段、累计步骤、从启动到该标记的单调墙钟。时标在解释器的宿主诊断写入处取得，不改来宾时间、输出或计步；没有为测速增加来宾宿主槽。

102 执行生成可并入 `build/phase-times.tsv` 的六行 `phase-times.tsv`：读入、准备、标活、压紧、finish、总尝试。缺结束标记写 UNMEASURED；尝试超时的墙钟单列，不冒充全量完成耗时，也不假装单相可恢复。正式运行后保留会话证据并把有实测的登记行交共享表单写手；脚本不改 main。

材料准备与测量分开。入口 `check` 只核材料，`probe` 只验证一件对象的等价与计时标记，`baseline` 只生成 16、64、102 三档原生输出和 SHA；三者均不进入速度门。最后都带 `v010-r1` 参数。构造计时入口：

```sh
./tools/bound 55 taskset -c 10-11 sh ./unisacc.com "$HOME/.local/share/unisacc/research/ompgpt/quiet-speed/quiet-speed.cx" -o "$HOME/.local/share/unisacc/research/ompgpt/quiet-speed/timer"
```

私有材料目录的 `cxroot.tsv`、`cx`、`ext`、`tools` 链接到 `tape-m0` 工作树，只在构造时读取。`native` 和 `guest.tapeslice` 来源见 `tl-dce-mark-complexity.md`，`clock-phase.S` 基于同页计步探针。变更任一材料须重跑原生与 tape 对拍并重新绑定材料清单，不能静默覆盖旧计时身份。

验证边界：`timer check` 实跑确认 r1 清单和 102 件字节，正式 `run` 已验证 16/64 对象的输出和计时标记；102 对象只有截至 `mrk` 的阶段计数与超时尝试，没有完成态。历史单对象时标点验不并入本轮结果。

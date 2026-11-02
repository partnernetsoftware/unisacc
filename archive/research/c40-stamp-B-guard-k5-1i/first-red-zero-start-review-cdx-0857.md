# 首红停补证漏证短记（cdx；0857）

只读实际核查时间：2026-10-11T08:59:34.298244+08:00；HEAD `7b07b72343907d06f797340ae26585f14174a377`。被核材料 `first-red-zero-start-materials-0857.md` sha256 `d307d5afb8bddb446b8a3222603557db27296a59274628f4c809fa7587f8e663`；对照 `draft-front-residual-0852.md` 及 0857 授权。

结论：尚未闭合。cc 明确标注草案、未创建夹具、未实跑；固定 tip/driver/注入点和预期序已有，实测首个非零子 rc、首红后计数及日志原件仍缺。以下是草案需补正的可测性缺口，非实施授权：

- `chain.sh:55` 的 `echo "chain: E3 gen failed"` 输出到 stdout，材料 §4/§6 说 stderr 须含此句不吻合源码。shim 若只 return 3 不写诊断，e3-gen.err 也可能为空。
- §2 的 PATH sh/python3/cc shim 不覆盖 `$T/run` 与绝对路径 UA；因此仅 SHIMLOG 红事件后的零行不能证明 check-net/probe-run/UA 零启动。材料也承认 UA 未过 shim，须具名补可见性或收窄计数结论。
- §3 方案 (b) 的全局坏 SEED_GEN_BIN 会先被 E2 pp helper 使用，可能在 E2 就红，无法自动认作 E3 注入；需绑定实际 E3 调用的注入身份。方案尚二选一，未定稿。
- §2 `NETWORK=0` 跳过 net/check-net，不能实证这两类零启动；§6 写“六类”而列出 tbl/net/check-net/probe-run/UA/cc/python3-other 七类。需保留逐类定义、计数和足以证明实际到达 E3 的前序成功记录。
- §2 设置 CHAINKEEP 空列表在非故障对照中会被 `chain.sh:66` 拒绝，不能作为正常对照成功配置；正例是否走过待观测后续任务仍缺。
- 前后 HEAD/status、夹具及 driver 字节身份、日志/rc 原件与副本 sha 尚待实跑采集。旧 /tmp 原件已失，草案转述不能代证。

严格 PGID/SID 清空、TERM 与完整 UA 可见性等原限定继续保留；0857 本窗只补零启动，不把这些顺延项追加为自动实现任务。未实现、未改码或验收、未跑门、未 bump/freeze/Draft、未提交或推送；本记仅为当前草案快照，后续补证需再核。

缺口名：当前 driver 首红后零启动实测及可完整计数的日志原件未齐

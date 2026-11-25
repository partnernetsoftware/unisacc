# T1.4b-lite 十六对象批次登记卡

(1) 状态：候选批 `tape-tldce-lite16`，待批次容量落 main 后登记；本页不改共享表。Linux x86_64 工具自检，不作 102 对象全量验收，不作速度门。输入为 v0.1.0 g2 链 `a11d28fc0a61939ee71345d732a7dcf205b261f1` 的 Linux x86_64 对象；`tl-dce-mark-artifacts/lite16-order.txt` 定义唯一顺序，SHA-256 `8d33c1eac15804deccccee2bfa89128364b7c5a18aec631b047ad386bea4f63e`。`lite16-sha256.txt` 绑定 16 件实际对象字节，清单 SHA-256 `0396f5b8415c87c76772d209e737677e413dbc07976506f2fdb6a5c51937c042`；准备阶段在 g2 目标目录实跑 `sha256sum --check --quiet "$a/lite16-sha256.txt"`，不得只核列表名。

(2) 检查命令：按 `tl-dce-mark-complexity.md` 的「T1.4b-lite 最小批回放」准备 `plain.exe`、`count.exe`、`phase.so`、`16.list` 和固定容器；将该页有界执行段作为登记入口。批次先固定清单内容与链产物身份，再以同清单实跑两份原生与 tape，任一返回非 0 或任一 `cmp` 非 0 即失败。不缓存结果。准备与运行分开登记，工具和容器都需有输入身份；`internal.tapeslice` 摘要 `909cacfb03d1be04c1c93ada4f69aebad5731652f0d96eacfaec5e726113cf29`。

(3) 预期：三方 stdout SHA-256 `93b5e2180cbbd49c330f17d49b1b31c7e9ba82b01eac1007e1993cfb3ede7439`；首 32 字节为四个 LE u64：16、317638、10683003768862666889、199。最后一项是实际 `tl_dce_cut()`，不得为零。`mrk -> cpy` 为 64998445 步，当前插桩口径；身份变更后须用同源对拍解释差异，不取宽松范围。

(4) 新归档回放实测：`load1=4.20`，`T14_LITE16 status=PASS cut=199`，有界批阶段在 55 秒限内完成，整轮含两个新归档、两份原生构造、共享库构造和检查为 15.98 秒。未把材料构造算入下面的单批执行计时。速度门仍为 UNMEASURED，静默时另测。

(5) 关运行缓存单次回放实测：预构造材料不变，检查段用 `tools/bound 55 nice -n 19 taskset -c 10-11 env UNISACC_RUN_CACHE=0`，`load1=6.80`，墙钟 `15.604 s`，rc=0，三方 cmp=0，SHA-256 与上文一致，cut=199。只证明该次执行端关缓存，未清 OS 页缓存，也不是三轮冷态余量收据。

# omp 状态板（cc 维护，2026-11-02 14:55 AWST）

| 窗口 | 当前任务 | 拥有的路径（可写） | 阻塞 / 等待 | 下一次确认 |
|---|---|---|---|---|
| omp-bdy | 慢车道试点：pp-negative 两张表 → 私有根纪元 `out/epoch-pp1`；工具侧：release-ape 写 build.json、prepare-context 比 product_digest | `exec/pp/*`、`out/epoch-pp1/`、`tools/release-ape.cx`、`tools/cxlib/source-package*` | 等 omp-go 的 split2 接口；gen-shared-e2 报 receipt-bytes-mismatch 在查 | 15 分钟督导 |
| omp-go | 产品身份：评审 9 条已修，split2（a-f）进行中 | `tools/cxlib/productpkg.cx`、`allsession.cx`、`tests/*` 里与身份相关的批次、`tools/check-productpkg.cx`、`check-root-identity.cx` | 落地后把接口直接发 omp-bdy | 15 分钟 |
| omp-ds | census 复跑 27 个未知（等纪元结束）；register 加固已提交；P4 队列已提交 | `tools/census.cx`、`tools/register.cx`、`tools/check-*` 自有 | 暂停重批，等 cc 通知 | 回信时 |
| omp-go2 | 排期：①Orca 客户端安装配置（先出方案，我审后装，数据流向要先讲清）→ ②完成后由 cc 编排它开发 superharness（对应 DeepSeek super-agent 设计理念，见 plans/super-agent-omp-study-2026-11-02.md §8，原型在 out/superharness，须内建并发上限/深度守卫/精确 PID 清理）；材料第一层重建待派 | `plans/`、`out/`、moltbaby 仓（用量行） | 等 cc 审方案 | 回信时 |
| cc | 审核提交、冻结/解冻、对主人汇报、状态板 | `plans/omp-*`、提交 | — | — |

长跑会话：无（aw-m4-mat 已结束；census 暂停）。冻结：无。
常驻：`procguard`（systemd --user，enforce，over=0）。

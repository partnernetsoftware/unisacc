# S2 补丁 v2:sy_readable(fd,0) 回归与注入门控返工

回归根因(86090bebb 补丁):`syt_readable` 首轮在 `left<=0` 直接返回,`sy_readable(fd,0)` 不发一次 `poll(0)`,已就绪 fd 也报 0;`tools/check-cx-streamapi.cx:279` 依赖 flush 后立即报 1。

## 修复
- `cx/sys/time.cx` `syt_readable()`:新增 `first` 标志;首次一定执行 `poll`,预算为 0/已耗尽时 wait 取 0(不等待但至少问一次);EINTR 后按单调时钟剩余重入,`!first && left<=0` 才返回 0。
- 注入入口对齐 S1e:默认关闭;仅当 `UNISACC_TIME_INJECT_EINTR=1` 且 `UNISACC_TIME_INJECT_PID` 存在并精确等于本进程 pid 才生效(缺失/异 pid 均关闭)。

## 夹具(tools/check-time-eintr.cx,8 断言)
原行摘录:
- `ck(rc == 1, "readable(fd,0) returns 1 on a ready fd");`
- `ck(rc == 0, "readable(fd,0) returns 0 on an unready fd");`
- `ck(t1 - t0 >= 190 && t1 - t0 <= 1000, "readable wall inside 190..1000 ms");`
- `ck(rc == 1, "omitted PID disarms the seam (ready fd still 1)");`

## 变异与实跑
- M12(去掉首次 poll):`FAIL readable(fd,0) returns 1 on a ready fd`。
- M13(PID 可省略,恢复旧门控):`FAIL omitted PID disarms the seam`。
- 绿:`CHECK_TIME_EINTR PASS fail=0`;`CX_STREAMAPI ok=205 failed=0`;`CHECK_CX_SYS PASS ok=179 fail=0`;`CX_LINT files=1 findings=0`。
- 依赖调用点(git grep sy_readable):已跑 check-cx-streamapi、check-cx-sys;apps/csih 与 ujs 属他车道未动。

## 登记与门
- 补丁含登记:register --tool(`sources_added=1 inventory=1554`)与 register 批次(`REGISTERED`,`manifest=WRITTEN`,`REGISTER_LAYOUT_SELFTEST PASS 7574ms`),四表齐(cx/sys/time.cx + check-time-eintr)。
- 私有仓预热 canary:`AFFECTED_REGISTRY_CHECK batches=2019`;完整 `tools/precommit.cx` 在归档私有仓 55 s 内未跑完(`bound: timeout after 55 seconds: sh`),未记绿,交 cdx 在带热缓存的树复核。

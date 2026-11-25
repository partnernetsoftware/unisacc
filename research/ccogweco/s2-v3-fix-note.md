# S2 v3:cdx v2 复核两条补齐 + 现头重登记

基线:origin/main 70756430e;私有仓 /tmp/s2/v3chk(带 git 索引);补丁 6 文件,基于 70756430e 生成,scrub 0 命中。

## 补两条
1. **具名错误 PID 负控**:注入门改为把 `UNISACC_TIME_INJECT_PID` 与 `%ld(getpid())` 按字节精确比较;`PID=1`(外进程)时 on-ready 管道 `sy_readable(fd,0)=1`。夹具断言 `foreign PID does not arm the seam (ready fd 1)`;变异 M14(接受任意已解析 PID)→ 该断言 FAIL(fail=2)。
2. **PID 溢出拒绝**:不解析数字、只做字节比较,超长数字串(2^64+pid 的十进制,夹具内用十进制进位构造)必然不匹配;断言 `over-long PID text is rejected (ready fd 1)`;变异 M15(64 位无符号累加回绕后比较)→ 该断言 FAIL。
3. 既有回归保持:`readable(fd,0)` ready=1 / unready=0,变异 M12(去掉首次 poll)红;`omitted PID disarms`(M13 PID 可省)红;EINTR 墙钟 190..1000ms;封印触发可观测为 armed+ready fd ms=0 返回 0(去掉了新增导出符号,避免 coreapi 棘轮新增,现 259/0)。

## 现头登记与门(全部原行)
- `REGISTER_LAYOUT_SELFTEST id=check-time-eintr status=PASS ms=6179`(冷跑 <25 s);register --tool `REGISTERED`;批次 `REGISTERED`、`manifest=WRITTEN`。
- 冷跑 `BATCH_TIMING id=check-time-eintr total_ms=5982 ... assertions=10`,`BATCH_RUN status=PASS`。
- `CHECK_TIME_EINTR PASS fail=0`;`CX_STREAMAPI ok=205 failed=0`;`CHECK_CX_SYS PASS ok=179 fail=0`;`CX_HOST_RUN failed=0`;`EXT_PROCX_EXEC ok=26 failed=0`;`CX_SYS_NETPOLL ok=26 failed=0`;`CX_LINT files=2 findings=0`;`RATCHETS ok=19 fail=0 elapsed_ms=18843`。
- 未记绿:完整 `tools/precommit.cx` 与 `tools/derive.cx --check` 在 55 s 有界内两次超时(`bound: timeout after 55 seconds: sh`,load1 28.6 与 6.06 各一次);未跑完的相位不冒充 PASS,交 cdx 在热缓存树复核。

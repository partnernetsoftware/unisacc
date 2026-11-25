# 真并行核查:106 预跑 lane 是否被串行化(只读;2026-11-25 ccogweco)

数据:tools/prerun-drive.cx 当前源码、~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/supervise.log、
本窗口 winsum 聚合(238 窗、总窗墙 7,497,134 ms、本体 122.6 分钟、等效忙 1.01 lane)。

## 0 结论

lane 是**真并行**:每个 lane 一个独立会话子进程(prerun-drive.cx:1533-1541 的 sy_fork_session),
且每个 lane 有独立 TMPDIR(`.lane-tmp-N`,:1519-1524),不是共享 TMPDIR 串行。

但 106 的等效利用率只有 1.01 lane,原因是三个结构点,不是"锁":

1. **分组屏障**:一组 lane 全部 fork 后,必须等这一组全部结束才进入下一组(:1544-1564 的 waitpid 循环,
   组内 straggler 决定该组墙钟);组与组之间不重叠。
2. **可共享窗口判据很窄**:pd_can_share_window(:158-160)只允许 jobs=1、单 id、非 early canary 的 long 窗,
   以及 netregen/isolated 类 heavy;普通 heavy、jobs=8 的 normal 窗不参与 lane 并行。
   日志印证:首 20 行窗口之后才出现 `PRERUN_LANES first=W0016 windows=3`,此后多为 windows=2。
3. **前置串行段**:warmup、prebuild-self 与早期 canary 都是单窗串行(:649-711),前 20 窗没有 lane 并行。

## 1 点位与依据

| 点位 | 源码行 | 行为 | 影响 |
|---|---|---|---|
| lane 启动 | :1533-1541 | 每个 lane `sy_fork_session`,独立会话 | 真并行,非假并发 |
| lane 独立 TMPDIR | :1519-1524 | `TMPDIR=<out>/.lane-tmp-N` | 无共享 TMPDIR 串行 |
| 组内等待 | :1544-1564 | 等本组所有 pid 结束;超 deadline 才 kill | 组屏障,尾窗空转 |
| 可共享判据 | :158-160 | 仅 long 与部分 heavy;jobs 必须 1;单 id;排除 canary | 大部分 heavy/normal 不能并行 |
| 组窗口挑选 | :1855-1881 | 只在下一窗也满足判据时继续填组;否则 break | 组常常只有 2 个 |
| 串行前置 | :649-711 | warmup、canary、prebuild-self 单窗 | 前 20 窗串行 |

- 106 日志中 `PRERUN_LANES` 从 W0016 起,`PRERUN_LANE_WALL first=W0016 windows=3` 之后多为 windows=2;
  若组屏障是唯一问题,应看到"下一组紧接着前组结束",而不是整体 1.01 lane——所以窄判据与前置串行同样是主因。
- normal 窗是单窗 jobs=8(窗口内 dvqualify 并发),在 lane 计数里只算 1 个窗,不增加 lane 利用率。

## 2 最小改法设想(只写设计,不实施)

1. 滚动 lane(去组屏障):维护 N 个 lane 槽,任一线程空闲即从队列取下一个可共享窗口启动,而不是整组等齐。
   改动集中在 :1855-1881 的填组与 :1544-1564 的等待;收益上限 = 消除 straggler 空转,按 106 数据估计
   可回收几十 lane-分钟,但改变"同组同源"的现语义,需负控(同窗重跑、residue 判定)。
2. 放宽可共享判据:对 heavy 里的 netregen 以外批,若每批有独立 out/material 路径与独立 pin,可加入共享集合;
   依据是 :158-160 的注释"material contract known";这需要逐批材料表作为判据,不靠人工。
3. 排序:窗口队列按上一轮实测耗时降序(重窗先跑),减少组内 straggler;不改窗数与上限。
4. 前置串行段:kit 已热时可把 warmup/prebuild-self 并入首组(同候选、同 frozen-root 时),但 must keep
   PRERUN_FROZEN_SOURCE 与 source 戳校验。

## 3 这不等于把 122.6 减半

- 总窗墙 124.96 分钟是"窗内时间"总和;外层的 gating、outer_overhead、重试(12 次 rc124)与组屏障一起构成
  122.6 分钟。即使滚动 lane 成功,上限受"最长单窗 + 剩余"与不可共享窗约束,不是 62.5 分钟。
- 任何改法都要保留:每窗 55 秒内限、frozen-source 戳、residue 判定、原 FAIL 行保留;不得为提高并发放宽时限。

## 4 判定

- **不是锁,不是共享 TMPDIR**:lane 真并行、TMPDIR 独立。
- 串行化来自:组屏障(:1544-1564)、窄共享判据(:158-160、:1855-1881)、前置串行段(:649-711)。
- 提速可行但有上限;最小改动是滚动 lane + 重窗排序,需负控与材料判据,不能只改一行。

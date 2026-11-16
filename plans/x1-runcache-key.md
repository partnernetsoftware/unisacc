# X1 运行缓存键归一(0.0.71,待合入)

状态:补丁与 17 项检查已写好并实跑通过,尚未合入主线,原因是 srcshape 棘轮。

## 做了什么
- src/runcache.cx 的键由 `当前目录 + 原样参数 + 原样源路径 + 源字节` 改为"归一"形式:当前目录前缀下的绝对路径改写为相对路径进键,收据同样记相对路径,命中时在新目录下重新解析再核 sha;当前目录串本身不再进键。
- 单元或被读到的头文件含 `__FILE__` 时,条目"钉住":改存在旧形式键(含当前目录、原样参数与路径)下,收据记绝对路径;查找先查归一键,没有再查钉住键。这样 `__FILE__` 展开出本根路径的程序在另一个根下不会错误命中。
- 键版本 12 改为 13。

## 实测(候选 = entry-build 构造,2026-11-16)
- tests/runcache-roots.cx 17 项全过:两个根之间命中;改源一字节、改被 use 的模块、换编译器身份(候选副本新 inode)、改 -D 参数各自不命中且新值可见;`__FILE__` 在两个根下各打各的路径、互不命中;每个场景另跑 UNISACC_RUN_CACHE=0 臂,输出相同。已装产品跑同一检查,b-first-hit 失败,说明检查能区分新旧行为。
- 真实工具(tools/status.cx,两个 git archive 完整根):根 A 冷 7947 ms,根 B 首次 138 ms(命中),A 再次 134 ms,B 再次 140 ms。旧键下换根必不命中,每工具新根首跑约 5 至 8 秒。

## 阻塞
- tests/srcshape(src/ 只许变瘦):runcache.cx 670 行 -> 756 行,maxfn 118 -> 123,ifs 124 -> 139,src 总行数 5854 -> 5927。要合入须在 src/ 别处净删至少 73 行,或把归一逻辑改成模型步。R1(prune 判定移出驱动)可能腾出行数;未腾出前不提高基线。

## 合入办法
    git apply plans/patches/x1-runcache-key.patch      # 含 src/runcache.cx 与 tests/runcache-roots.cx
    sh ./unisacc.com tools/entry-build.cx -- $PWD $(readlink -f unisacc.com) /tmp/x1/eb
    rm -rf /tmp/x1/rr && sh ./unisacc.com tests/runcache-roots.cx -- /tmp/x1/rr /tmp/x1/eb/candidate
再登记 runcache-roots 批次(tests/manifests),并在冻结前走慢车道。

## 未验证
- bootstrap-0.0.59 多单元编译、两代不动点、六目标;旧检查 tests/runcache.cx(24 项)与 runcache-units.cx(6 项)在新代码下的复跑结果是上一轮子代理所报(24/24、6/6),本轮未复跑。

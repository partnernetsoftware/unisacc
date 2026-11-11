# 2c2 typed E3 terminal overlay

实际跑过；只有固定用例上的窄联证。构造源基点0b7e7920；提交基点随后前进到eb577c8e（仅boundcheck平台裁定，不改变模型读集）。详见 tests/seals/librarytypedreader/overlay.json 逐侧路径/长度/SHA、命令与源摘要。

## 范式组件五问

1. 复用有限表＋通用δ执行器；专属template只移动两个既有入口，复用MS/LX/MG、既有typed frame/tail与USTFLOW2。新增末尾call由fact:librarytyped守卫，无开关不安装任何overlay节点。
2. 不新增产品读写/资源/生命周期机制；没有驱动或W改动。动态C夹具使用现有cx通道函数，属于观察层，不是产品阶段。
3. typed逻辑消费者8与普通消费者1共用物理function owner1，语义分账；普通side是main中的同一wrapper投影，不再重复计一次消费者。
4. 不新增S/W封套、快照或提交点；本刀不宣称SC1产品接线。缺key、ABSENT、EMPTY保持既有区分，不用空流冒充拒绝。
5. 本刀把旧typed frame/tail复用到稳定ID引用，不复制尾段、不开第三通道；仅计这一复用面和窄联证，不计运行接线完成。

## 证据

- 关闭开关：完整JSON/table/network两侧逐字节相同；C/P JSON也相同。启用librarytyped+locations时C/P同字节，network1490237字节。原byte/result与W/公开面摘要不变。
- reader+overlay：5收、10具名拒、60次table/network/A/B观察。PLAN仍0，dispatcher改为.libraryfn；unused/defined不产wrapper。固定两int参考仅核这个frame模板，不冒充完整E3参考。
- 实际E3输出→audit2：typed symbol256仅main typed dispatcher引用，logical8；普通symbol256的side与main投影匹配、logical1。每个实际claim送分区模型byte10=2，C模型/参考选择结果相同；交叉消费者和mask9各拒，空stdout且具名。普通控制A/B的完整E3输出同字节，其side与已有ordinary参考逐字节同。
- 普通控制采真实E1 UNITOK/sourcefacts/attributes。初试旧裸tokens被source layout拒，未当产品回归；审计器初试重复计算ordinary main/side，已按既有投影合同纠正，不修改模型/参考判据。隔离树初次工具未指定观察产品路径、一次运行中修改观察脚本导致包装错误，弃用这些工具尝试，最终冷准备在冻结脚本上重跑。
- 8 flag seed路线：C/P/reader旧产物三方同，18命令rc0，最慢32.33秒；gen779/parse2380读集增删0。原始trace仓外，sanitized收据在overlay-seed-memory.json。全exec冻结初检遇到2个未读自测/说明变化；已按实际读集+seed/gen.c核对，故标PASS_SCOPED而非全树未变。
- 新键：gen2290b8c8c26c572e、parse23d00c28f2209089a。C构造器源码/二进制与reader相同；实测RSS均不高于既有准入上限，warnings,errors实测2148MiB仍保留2149MiB保守上限。不降低并发/余量，不放宽窗口。

## 冷准备（外部monotonic秒，逐命令bound58）

| 步骤 | rc | 秒 |
|---|---:|---:|
| tools | 0 | 6.23 |
| base-c | 0 | 4.28 |
| base-off | 0 | 19.42 |
| base-reference | 0 | 20.47 |
| base-tables-base | 0 | 2.27 |
| base-tables-off | 0 | 3.57 |
| base-net-base | 0 | 0.32 |
| base-net-off | 0 | 1.92 |
| typed | 0 | 25.84 |
| audit | 0 | 1.87 |

命名作业exec-librarytypedreader-model在隔离树rc0，8秒。

## 未证

未接lower typed span/validator、未在绑定后构造plan/closure、未证任意多符号/任意签名消费互斥、未证P1–P6；四槽/W/公开API均未改。原seedparse2-1/-2内存UNVERIFIED债仍须发布前足内存宿主补跑，串行8flags证据不替代该两作业通过。未开整闸、未新鲜产品构建、未碰发布面。

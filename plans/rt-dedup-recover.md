# RT-DEDUP / libc 副本：归属与回收（不改产品）

**同候选实测，不用对象大小预报。镜像单元贡献仍 UNATTRIBUTED。**

## 旧数字（不得当成本候选收益）

| 说法 | 来源 | 本页地位 |
|---|---|---|
| 76 份运行库私有副本 269,058B | T-LAYOUT / slice-breakdown 转写 | 未在本候选复跑 |
| libc 静态副本 ~477,728B、可省 ~412KB | items RT-DEDUP | 上限估算，非镜像实测 |
| cdx build9 少 241,664B | 仓外 | 非本树、非本候选 |

fable：切片大头是这些副本与 APE 三份模型，**与迁 NN 无关**。

## 本候选命令（2026-11-07，产品 `./unisacc.com`）

两份最小 C、`-c -funit -b lnx/x86_64`，再 `tools/image-contrib.cx`：

```
UNIT a.o tape_payload=647 object=3016 IMAGE=UNATTRIBUTED
UNIT b.o tape_payload=298 object=2296 IMAGE=UNATTRIBUTED
TOTAL image_attr=UNATTRIBUTED
```

`a.c`=`int main(void){return 0;}`，`b.c`=`int f(void){return 1;}`。

结论：

1. 两对象 tape 载荷 **不相等**，也 **远小于** 269KB；不能把旧 76×运行库抄到这两份 `.o`。
2. `-c` 对象的 tape/容器长 **不是** 最终 `entry.elf` 贡献。
3. 最终镜像仍 UNATTRIBUTED；要量化「76 份在镜像里可回收多少」必须 sidecar 或同候选 **链接后** 的可证明范围，当前工具拒绝摊销。

未跑：76 个切片 `.o` 清单、entry.elf 节/符号、stdio 头进对象。标 UNMEASURED。

## 回收方案（评审后才动产品）

沿已有 `tools/rt-linkage.cx` 三态（`static` / `RT_UNIT` / `RT_EXTERN`），**一次调用多单元链接** 只留一份定义。

门槛（旧 RT-DEDUP-FIX 教训）：

- gate a：不加宏时 empty/`host` 的 `.o` 与 `-o` 与改前 `cmp`。
- 必须用新包跑 `entry-build` 自举（forwarding stub 多定义曾红切片）。
- `/dev/full` stdio、stdio-crossunit、`-funit` 系统 ld 不多定义。
- 收益只记 **同候选** `entry.elf` 与五片 gzip **before/after 实测**，禁止用 412KB 估写入 ledger 当完成。

不实施本页。不碰 loader。

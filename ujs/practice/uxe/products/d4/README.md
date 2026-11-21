# D4 私有步态与跟拍

Wasm 计算四足髋、膝、足关节、车身偏移、行进距离和跟拍机位。从仓库根目录构造到临时目录。

```sh
cx tools/devsvc.cx -- x --par 1 -- cx ujs/practice/uxe/products/d4/build.cx -- /tmp/d4.wasm
```

`uxe_d4_frame(rest, out, work, framing, phase, mode, fixed)` 接受线性内存中 8 字节对齐的偏移，以下字段为 f64。

| 区域 | 字节 | 布局 |
|---|---:|---|
| rest | 288 | 四腿依照 D4 事实表顺序，每腿髋、膝、足各三个坐标 |
| framing | 64 | 初始相机位置和目标各三个坐标、参考画面高度、焦距像素 |
| out | 376 | 车身偏移三个坐标、行进距离、相机位置和目标各三个坐标、纵向视场角、四腿坐标 |
| work | 376 | 临时帧 |

每四个相位完成一轮步态，支持负相位。模式 0 至 5 对应原 D4 静态参考、行走、滑脚、冻结车身、三足腾空、第一条腿加长。固定机位参数取 0 或 1。成功返回 0。拒绝返回 -1 并保留上一输出帧。

浏览器验收夹具位于 `ujs/practice/uxe/test/d4-private-frame-r160.html`，通过旧 D4 步态参考和现有 UXE WebGL 绘制做独立对照。从仓库根运行 `cx ujs/tests/d4-private-frame-io.cx`。附加 `-- --mutate` 时预期返回 1。

当前产品切片覆盖步态和跟拍。后续迁移包括城镇、烟、机械腹部、Mixer 和完整产品画面对照。

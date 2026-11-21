# LineDashedMaterial

`LineDashedMaterial_init` 使用 r160 的默认值，scale 为 1，dashSize 为 3，gapSize 为 1，颜色为白色。`LineDashedMaterial_set` 更新虚线参数和 24 位 sRGB 颜色。scale、dashSize、gapSize 的范围是 0 到 65536，dashSize 与 gapSize 中至少一项达到 1/1048576。无效设置保留原记录。

`LineDashedMaterial_draw` 接收非索引线段。每个顶点含 NDC 位置和调用者给出的累计线距离。UJS 把缩放后的距离、虚线长度、周期和颜色写入现有 GPU 绘制包，由 WebGL2 线段插值和片元着色器决定覆盖。相位大于 dashSize 的片元被丢弃，gapSize 为零时保留完整线段。

试验使用真实 GPU bridge 并与 r160 的 LineSegments、computeLineDistances 和 LineDashedMaterial 对拍。当前实现采用屏幕空间插值和单位线宽，覆盖正交投影切片。透视校正、场景材质绑定、雾、顶点颜色、继承的材质生命周期尚未接入。

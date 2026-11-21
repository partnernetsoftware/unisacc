# PointsMaterial、LineBasicMaterial 与 SpriteMaterial 字段 JSON

三个构造器复用公共 Material 记录，并保存类自身的颜色、雾和数值字段。PointsMaterial 保存 size 与 sizeAttenuation，LineBasicMaterial 保存 linewidth，SpriteMaterial 保存 rotation 与 sizeAttenuation，且默认透明。本切片的同类 copy 保留目标身份与事件表。

字段导出遵循 r160 的缺省省略规则。PointsMaterial 的 size 和两个适用类的 sizeAttenuation 总会输出。LineBasicMaterial 的 linewidth 和 SpriteMaterial 的 rotation 仅在非默认时输出。r160 的字段 JSON 省略 linecap 和 linejoin，本切片保留这一行为。

导入检查类型、颜色和数值字段，然后应用公共 Material 字段。错误字段拒绝整次导入。字段投影省略 UUID、元数据、名称、贴图引用、linecap 和 linejoin 的对象状态、渲染和完整生命周期。数值沿用共享 JSON 解析器。

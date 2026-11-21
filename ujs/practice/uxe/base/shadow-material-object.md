# ShadowMaterial 公共字段 JSON

原生构造复用 Material，默认颜色为黑色，开启透明和雾。copy 复制颜色、雾及公共字段，保留目标标识和事件表。

字段 JSON 使用 r160 的缺省省略规则。导入缺失 transparent 时保留构造默认值 true，显式 false 会被读取，重新导出时按 r160 省略。color 始终输出，fog 仅为 false 时输出。

浏览器使用固定 r160 的 ShadowMaterial 与 MaterialLoader 核对两向 JSON 读取、同类复制、缺省值及拒绝坏输入。当前切片覆盖字段投影，省略 UUID、名称和元数据。阴影绘制及完整材质生命周期尚未接入。数字沿用公共 JSON 读取器。

# MeshPhysicalMaterial 清漆

`MeshPhysicalMaterial_coat` 记录清漆强度和粗糙度，两者范围均为 0 到 1。无效设置返回 -1 并保留原记录。`MeshPhysicalMaterial_direct` 计算单个线性色彩通道的直接光，复用 Standard 的 GGX 高光和 Lambert 漫反射。入射光强范围为 0 到 65536，法线、视线及半程向量的点积由调用者夹取到 0 到 1。

清漆和基底使用相同法线。Schlick 菲涅耳项衰减底层漫反射和高光，再叠加按强度加权的清漆 GGX 高光。当前覆盖空气到介电材质的固定折射率 1.5、恒定法线、单直接光源。环境光照、清漆法线贴图、几何粗糙度、透射及场景材质绑定尚未接入。

试验在 UJS 求每个平面的颜色，再经已有 GPU bridge 绘制。独立 r160 渲染相同的正交场景，使用真实 MeshPhysicalMaterial 和 DirectionalLight。三列清漆强度依次为 0、0.5、1，四行覆盖三种法线角度以及两档清漆粗糙度。禁用清漆的篡改保留相同几何、光源和底层材质。

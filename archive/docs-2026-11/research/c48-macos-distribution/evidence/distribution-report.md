# macOS Release 下载与隔离首跑

实测源为公开 v0.0.47，非本机开发 f069 候选。gh release download 实际退出0，资产6,215,778B，SHA256 fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe，与 release/candidate.tsv 的APE pin相同。标签名与候选标签不同，因此同时保存公开Release元数据，不伪造候选来源。首次curl HTTP504失败未产生资产；未改镜像来源。

系统为Darwin arm64/macOS26.5.1，Gatekeeper assessments enabled。下载工具不自动添加quarantine，测试显式写入独立副本属性 `0081;00000000;Codex-X4-isolated-test;`；这不是Safari天然下载或Finder点击测试。属性在首跑前后保留，未删除属性或关闭Gatekeeper。

spctl对原carrier及提取Mach-O均实际退出3/rejected；carrier报告source=no usable signature。正常文档中的sh入口在全新私有XDG cache首次运行--version退出0，hello.c默认-run实际返回7。sh入口可运行不能证明Gatekeeper信任通过；提取载荷无quarantine，不能把这个传播行为当系统认可。当前不承诺Finder双击分发或Notarized Developer ID信任。

首次提取arm64镜像为3,071,458B，SHA e5f80facc67e6b419e8824e2fc81efaa24b0f4936250e9ad7a6518c09e3de5b1。codesign --verify --strict --verbose=4实际退出0，仅证明已有ad-hoc签名结构有效；没有执行任何签名/公证操作。otool实际LC_BUILD_VERSION的platform1/minos13.0/sdk13.0，因此可记录映像声明最低macOS13.0。本轮仅26.5.1真机实测，macOS13本身与其它旧版本未运行，不宣称完整兼容矩阵。

Apple说明来源：https://help.apple.com/xcode/mac/current/en.lproj/dev1cc22a95c.html 、https://support.apple.com/en-us/102445 。官方区分spctl系统信任评估与隔离下载首开；本报告不把上述实验模拟扩大成Safari/Finder实测。

该目录尚为父工作记录，最终仅小证据归档research，下载与缓存镜像留out；缓存保留策略另由X4实现与正式批验收。

# Safari 天然 quarantine 补充实测

Safari直接打开GitHub公开v0.0.47资产地址，UI实际出现504 Gateway Time-out；与curl失败相同。随后将gh已经下载并核SHA的同一官方Release字节临时只绑定127.0.0.1:18763，经Safari下载本地同字节副本。Safari下载列表明确为unisacc.com/6.2MB，临时服务GET200/6215778B。服务器使用系统Ruby WEBrick（测试宿主依赖），外bound60在完成下载后实际终态142清理，不是产品依赖。

Safari写出的实际文件为/Users/wjc/Downloads/unisacc.com，6215778B，SHA256 fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe。天然com.apple.quarantine为0083;6af0484a;Safari;09586878-394B-42EA-A6A9-893FCFFDB261，WhereFroms为本地测试URL。这一副本没有人工写quarantine；公开资产来源由前一gh下载和SHA绑定，不能把本地暂存传输说成Safari直连GitHub成功。

系统Gatekeeper保持assessments enabled；spctl对天然隔离carrier实际exit3/rejected，见safari-gatekeeper.stderr。chmod+x后按README的sh入口在全新私有XDG cache执行--version实际exit0/0.0.47，hello实际exit7；quarantine-before/after内容相同，无删除xattr、无关闭系统保护、无签名/公证。此结果只证终端sh路径首跑，不证Finder双击或Developer ID信任。

该测试补强先前人工标记隔离实验；两轮材料分别保留，未把模拟写成天然下载。正式运行终态：a64e1d rc0（包含属性前后和version）、dc92e5 rc7（hello）、dde27f rc3（spctl）、54423 rc142（测试HTTP服务器有界清理）。

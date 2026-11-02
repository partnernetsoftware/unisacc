# 根产品身份（M4 dev-lnx 安装后，2026-11-02）

本地根产品 = lnx-only 开发 APE（薄 shell 头 + 旁目录），非发布物（NOT_RELEASE）。
发布仍待 osx/win 片 + UNITOK2。

三身份 sha256：
  unisacc.com（薄 shell 头）   1bfdcd20e5aa60ff057c42330f3f99615f0ce1fe697b9d3017801a1f75339464
  unisacc.com.dev/entry.elf    be8c63c8beeab3fec33fd69c39b1da6baea5b38ff1accb0b8f76979a6cfd06a7
  unisacc.com.dev/p3-package.bin  47a2275d7e1143b3bf23dc9b11f08123cc44e183d1b2432ef4c1f15c1320c600

来源：
  entry.elf = entry-build（输入 /tmp/p3fresh/pkg.bin，构造器 c9ec40e7 系）的 lnx/x86_64 片，
              含 nm 修复 bea9f64237 + _unisa_map/stdio 修复。
  p3-package.bin = 现建 P3（--no-compress-resources），sha 47a2275d。
  candidate（不装机）= 09518630cf5991041b875cd818bddb2d0c1cb6058fea2dd7eedd856214ce0b99。

回滚：out/m4-backup/unisacc.com.83343ec1 + ROLLBACK.md（旧 APE 83343ec1）。

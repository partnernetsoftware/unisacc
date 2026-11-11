# 根身份：osx-arm64本机

主机车道`osx-arm64-local`，ROOT=`/Users/wjc/repos/unisacc`，实际宿主`osx/arm64`，2026-11-08只读测量；没有安装或换资产。以下仅证明这台Mac实物，不代替构造来源或跨机发布资格。

```tsv
product_kind	ape1
product_digest	f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82
unisacc.com	f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82	5818178
```

共享`ape-receipt.txt`记录来源`/home/devali/repos/unisacc/out/c48-t14-e586-cdx/ape/unisacc-next.com`，SHA为`3d0e237dae4ca2c277c1692dfa4c3e8fdcd58637e914ed7ddd2eb18d51263645`、6425630B；`plans/root-identity.md`同样记录3d0e。该路径属于另一台机器的构造记录，不在本机验证其存在或身份，原记录不改。

本机运行旧全局检查`tools/check-root-identity.cx`实际不匹配，原始失败在`out/obj-c-prunes-external/root-record-mismatch.log`。ROOT-RECORD-HOST-SPLIT要求记录与核验入口都显式按主机/ROOT选择；在检查器完成该接口前不将现有全局检查冒作本机PASS，也不以本机f069覆盖他机3d0e。

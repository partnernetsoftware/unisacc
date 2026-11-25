# 教训:预热必须走正规入口(草稿,待并入技能页;2026-11-25 ccogweco)

## 现象

0.0.107 预跑前(01:48)手工预热 old-product 缓存条目:直接 mkdir 缓存目录、cp 公开 APE、chmod 文件;
因 umask 0002,目录落成 0775。结果 60 批里 16 个窗红(需要 old-product 材料的批),15 分钟内以 chmod 0700
修复后恢复。原命令形态(示意,非执行):

    mkdir -p ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>
    cp <公开APE> ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>/unisacc.com
    chmod 0755 ...        # 受 umask 0002 影响,目录为 0775

## 根因

old-product 的缓存目录由工具自己校验权限与属主,手工创建不满足:

- tools/old-product.cx:47-49 `private_dir`:要求存在且为目录、属主为当前用户、`mode & 0077 == 0`
  (组与其他无任何权限);0775 直接 `REFUSE cache-directory`。
- tools/old-product.cx:89-90:入口只接受 `VERSION ABS_CACHE [--mirror ABS_DIR]` 两种形态,版本必须在
  tools/old-product-assets.tsv 登记且字节/sha 匹配;手工放文件绕过 select_asset 与 verify。
- 缓存产物格式由工具写出:目录 0700(48 行 `sy_mkdir(p,0700)`)、`fetch.lock` 0600(94 行)、
  资产文件 0500(108 行 `sy_chmod(tmp,0500)` 后原子 rename);手写产物不会带这些属性。

## 固化动作

1. 预热只用正规入口:

       sh ./unisacc.com tools/old-product.cx -- 0.0.106 <ABS_CACHE> [--mirror <ABS_MIRROR>]

   期望 `OLD_PRODUCT PREPARED`(rc 0);失败 rc 2 为事实/字节/存储无效,rc 3 为材料不可用。
   该命令写 0700 目录、0600 lock、0500 资产,并核 version 与 sha。

2. 预跑前核验缓存条目(只读):

       CACHE=~/.cache/unisacc/prep-cache/old-product-cache/<sha256>
       stat -c '%a %U %n' "$CACHE" "$CACHE/unisacc.com" "$CACHE/fetch.lock"
       # 期望:目录 700、资产 500、lock 600,属主为当前用户
       sha256sum "$CACHE/unisacc.com"
       # 与 tools/old-product-assets.tsv 对应行的 sha256 与 bytes 逐字相等

3. 不用 mkdir/cp/chmod 组合造缓存;umask 0002 环境下尤其容易得到 0775。

## 108 候选

- 所有手工预热件(不只 old-product)在加入预跑前必须有"格式核验"一步:属主、权限位、字节数、sha、
  以及工具侧 recongized 产物名(如 fetch.lock);核验不过即视为未预热,按冷路径跑。
- 建议把核验做成一个小检查(输入:缓存根与登记表;输出:格式与身份的 PASS/REFUSE),随 0.1.x 工具线落。

## 落点

- skills/unisacc-orch-supervise 的预跑/材料节(由 cc 合并);
- tools/old-product.cx:47-49、:89-90、:108(源码依据);
- research/agy-h/car2-108-candidates.md(候选条目);
- 本页只列教训,不改技能文件与工具。

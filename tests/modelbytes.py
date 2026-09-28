#!/usr/bin/env python3
"""Keep the measured artifact's functional byte ledger in PRD reproducible.
Snapshot check is offline; --artifact also verifies its exact size and digest.
No size ceiling: this is attribution, not a reason to remove functionality.
"""
import argparse
import collections
import hashlib
import json
import gzip
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BEGIN = "<!-- model-bytes:begin -->"
END = "<!-- model-bytes:end -->"
FUNCTIONS = {
    "e2": "预处理、目标预定义宏与位置模式",
    "e1": "词法与 token/位置输出",
    "e3": "解析、类型/作用域、tape、错误与警告",
    "e4": "O1/O2 优化",
    "lower": "ABI、调用、目标指令 lowering 与数据布局",
    "elf": "目标指令编码及 ELF/Mach-O/PE 镜像写出",
    "tokenpp": "公开 token 路线的预处理",
    "tokenlex": "公开 token 路线的词法输出",
    "units": "多文件分帧与文件级 static 隔离",
}


def capture(path, source):
    raw=path.read_bytes()
    assert raw[-16:-8]==b'UNIPKG1\n', 'missing package footer'
    size=int.from_bytes(raw[-8:],'little'); off=len(raw)-16-size
    assert off>0 and size>0
    pkg=raw[off:-16]; pos=0
    def line():
        nonlocal pos
        end=pkg.index(b'\n',pos)+1; b=pkg[pos:end];pos=end;return b
    def body(n):
        nonlocal pos
        assert 0<=n<=len(pkg)-pos
        b=pkg[pos:pos+n];pos+=n;return b
    h=line();fields=h.split();assert fields[:2]==[b'P',b'2'] and len(fields)==5
    nm,nd,nr=map(int,fields[2:]);assert min(nm,nd,nr)>0
    parts={'package header':len(h),'directory':0,'model bodies':0,'model record headers':0,
           'resource record headers':0,'resource keys':0,'C header bodies':0,'kernel bodies':0}
    refs=collections.Counter(); kinds=collections.defaultdict(set);routes=set()
    for _ in range(nd):
        b=line();parts['directory']+=len(b);f=b.split();assert len(f)==6 and f[0]==b'D'
        i=int(f[5]);assert 0<=i<nm
        refs[i]+=1;kinds[i].add(f[2].decode());routes.add(f[1].decode())
    models=[]; tags=collections.Counter()
    for i in range(nm):
        b=line();parts['model record headers']+=len(b);f=b.split();assert len(f)==2 and f[0]==b'M'
        data=body(int(f[1]));parts['model bodies']+=len(data)
        assert data.endswith(b'\n')
        for record in data.splitlines(keepends=True):
            tag=record[:1].decode();assert tag in ('N','S','Q','C','H');tags[tag]+=len(record)
        models.append({'index':i,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
                       'refs':refs[i],'stages':sorted(kinds[i])})
    resources=[]
    for _ in range(nr):
        b=line();parts['resource record headers']+=len(b);f=b.split();assert len(f)==3 and f[0]==b'F'
        key=body(int(f[1]));data=body(int(f[2]));parts['resource keys']+=len(key)
        r={'key':key.decode(),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        if key.startswith(b'\0hdr/'):parts['C header bodies']+=len(data)
        else:
            assert key.startswith(b'\0kernel/') and data[:8]==b'UNIKERN1'
            kind,entry,slot,n=struct.unpack_from('<4Q',data,8);assert n+40==len(data)
            r.update(kind=kind,entry=entry,slot=slot,payload_bytes=n,header_bytes=40)
            parts['kernel bodies']+=len(data)
        resources.append(r)
    assert pos==len(pkg) and sum(parts.values())==size
    peoff=struct.unpack_from('<I',raw,60)[0];assert raw[peoff:peoff+4]==b'PE\0\0'
    nsec=struct.unpack_from('<H',raw,peoff+6)[0];opt=struct.unpack_from('<H',raw,peoff+20)[0]
    secpos=peoff+24+opt;secs=[]
    for i in range(nsec):
        sec=raw[secpos+i*40:secpos+(i+1)*40];n,p=struct.unpack_from('<II',sec,16)
        secs.append({'name':sec[:8].rstrip(b'\0').decode(),'bytes':n,'offset':p})
    pebytes=max(q['offset']+q['bytes'] for q in secs)
    slices=[]
    for m in re.finditer(rb'  ([A-Za-z0-9_|]+)\) o=([0-9]+) n=([0-9]+) k=[a-f0-9]+;;',raw[:peoff]):
        begin,n=int(m[2])-1,int(m[3]);data=raw[begin:begin+n]
        assert len(data)==n and begin>=pebytes and begin+n<=off
        slices.append({'selector':m[1].decode(),'offset':begin,'bytes':n,
                       'sha256':hashlib.sha256(data).hexdigest(),'uncompressed_bytes':len(gzip.decompress(data))})
    assert len(slices)==4
    a={'file':str(path),'source_commit':source,'sha256':hashlib.sha256(raw).hexdigest(),'total_bytes':len(raw),
       'package_offset':off,'package_bytes':size,'footer_bytes':16,'package_parts':parts,
       'models':models,'unique_models':nm,'stage_rows':nd,'repeated_references':nd-nm,
       'logical_model_bytes':sum(m['bytes']*m['refs'] for m in models),'resources':resources,'resource_count':nr,
       'network_record_bytes':dict(tags),'routes':sorted(routes),'unix_slices':slices,
       'pe':{'bytes':pebytes,'sections':secs,'signature_offset':peoff,
             'header_bytes':struct.unpack_from('<I',raw,peoff+24+60)[0]},
       'alignment_bytes':off-pebytes-sum(q['bytes'] for q in slices)}
    assert a['alignment_bytes']>=0
    return {'artifact_ledger':a}

def render(evidence):
    a = evidence["artifact_ledger"]
    total = a["total_bytes"]
    parts = a["package_parts"]
    assert sum(parts.values()) == a["package_bytes"]
    assert a["package_offset"] + a["package_bytes"] + a["footer_bytes"] == total
    assert sum(m["bytes"] for m in a["models"]) == parts["model bodies"]
    assert sum(a["network_record_bytes"].values()) == parts["model bodies"]
    assert len(a["models"]) == a["unique_models"]
    assert len({m["sha256"] for m in a["models"]}) == len(a["models"])
    assert sum(m["refs"] for m in a["models"]) == a["stage_rows"]
    assert sum(m["bytes"] * m["refs"] for m in a["models"]) == a["logical_model_bytes"]
    def row(name, n):
        return f"| {name} | {n:,} | {n / total * 100:.2f}% |"
    lines = [f"快照 SHA-256：`{a['sha256']}`；总计 **{total:,} B**。", "",
             "| 物理内容 | 字节 | 占整个 .com |", "|---|---:|---:|"]
    for name, n in [(f"{a['unique_models']} 个共享网络体", parts["model bodies"]),
                    ("平台驱动、APE 启动/加载与对齐（混合账）", a["package_offset"]),
                    (f"{sum(r['key'].startswith(chr(0) + 'hdr/') for r in a['resources'])} 份 C 头文件/库实现源码", parts["C header bodies"]),
                    ("两 ISA 通用推理执行核资源", parts["kernel bodies"]),
                    ("目录、记录头与资源键", a["package_bytes"] - parts["model bodies"] - parts["C header bodies"] - parts["kernel bodies"]),
                    ("尾部", a["footer_bytes"])]:
        lines.append(row(name, n))
    grouped = collections.defaultdict(lambda: [0, 0, 0])
    for m in a["models"]:
        assert len(m["stages"]) == 1 and m["stages"][0] in FUNCTIONS
        v = grouped[m["stages"][0]]
        v[0] += 1; v[1] += m["bytes"]; v[2] += m["refs"]
    assert set(grouped) == set(FUNCTIONS)
    lines += ["", "| 模型阶段 / 具体功能 | 物理模型数 | 模型体 B | 占 .com | 阶段行引用数 |",
              "|---|---:|---:|---:|---:|"]
    for stage, meaning in FUNCTIONS.items():
        n, size, refs = grouped[stage]
        lines.append(f"| `{stage}`：{meaning} | {n} | {size:,} | {size / total * 100:.2f}% | {refs} |")
    lines += ["", "下表是网络体的内部拆分，与上表重叠，不能再相加：", "",
              "| 网络记录 / 含义 | 字节 | 占整个 .com |", "|---|---:|---:|"]
    meanings = {"H": "阈值/选择网络参数记录", "Q": "动作序列声明（包含编译模板动作）",
                "S": "字节字符串声明", "N": "网络头记录", "C": "动作序列共享前缀声明"}
    assert set(a["network_record_bytes"]) <= set(meanings)
    for tag, meaning in meanings.items():
        if tag in a["network_record_bytes"]:
            lines.append(row(f"`{tag}`：{meaning}", a["network_record_bytes"][tag]))
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--artifact", type=Path)
    parser.add_argument("--capture", type=Path)
    parser.add_argument("--source", default="unspecified")
    args = parser.parse_args()
    ledger = ROOT / "research/model-bytes.json"
    if args.capture:
        evidence = capture(args.capture, args.source)
        ledger.write_text(json.dumps(evidence, indent=2, sort_keys=True)+"\n")
    else:
        evidence = json.loads(ledger.read_text())
    text = render(evidence)
    if args.artifact:
        raw = args.artifact.read_bytes()
        a = evidence["artifact_ledger"]
        assert len(raw) == a["total_bytes"] and hashlib.sha256(raw).hexdigest() == a["sha256"], "artifact differs from measured snapshot"
    prd = ROOT / "prd.md"
    old = prd.read_text()
    assert old.count(BEGIN) == old.count(END) == 1, "missing/duplicate ledger marker"
    start = old.index(BEGIN) + len(BEGIN)
    end = old.index(END, start)
    new = old[:start] + "\n" + text + old[end:]
    if args.write:
        prd.write_text(new)
    else:
        assert old == new, "PRD model byte ledger is stale; run tests/modelbytes.py --write"
    print("model byte ledger: physical sums, sharing and functional attribution match")


if __name__ == "__main__":
    main()

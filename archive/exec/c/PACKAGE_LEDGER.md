# Fixed candidate package ledger

Measured 2026-09-28 by reading the existing artifact, without rebuilding or executing it.
Candidate: `/tmp/unisacc-call-label-candidate-928/unisacc-next.com`.
SHA-256: `81c04285acba68ef72cbe902f1474d551e457995c7b167e279b615d6de2cf3a6`.
Its adjacent `source.json` records source commit `e5c56cb705fdfbcf94218802cb302f69bf080106`
and explicit reuse of unchanged stages. This ledger identifies bytes by artifact hash,
not by the checkout in which this document is stored.

## Exact physical partition

Offsets are zero-based; byte counts include serialized record delimiters.

| Component | File bytes |
|---|---:|
| Windows x86_64 PE driver, including APE shell stub | 97,280 |
| Four gzip Unix driver members | 85,978 |
| Verified zero alignment between members/package | 38 |
| Package header | 14 |
| Route/stage directory | 53,169 |
| Model record headers | 274 |
| Unique network model bodies | 5,869,042 |
| Resource record headers | 204 |
| Resource keys | 268 |
| Carried C header source bodies | 103,636 |
| Two kernel resource bodies, including blob headers | 15,368 |
| Package footer | 16 |
| **Total artifact** | **6,225,287** |

Package extent is `[183296, 6225271)`, exactly **6,041,975 B**; its bytes
match the adjacent `compiler.pkg`. The parser consumes every package byte and
asserts the complete physical sum. It also verifies each gzip member's recorded
hash prefix and decompresses it successfully (including gzip CRC verification).

## S17 categories and limits

* **Generic execution kernel:** separately carried raw blobs below, **15,288 B**
  combined payload plus **80 B** blob headers, uncompressed in the package.
  Payload includes code, constants/strings and the import slot; it is not an
  independently measured instruction-only `.text` count. Historical object-size
  tables in `CORE.md` / `asm/README.md` do not establish this candidate's code-only size.
* **OS startup/syscall adaptation, native loader and linked C library:** combined
  in the five native drivers: **514,629 B** decoded file images, **183,258 B**
  carried in the APE (Windows uncompressed, Unix gzip). No symbol/link map in this
  artifact separates these three semantic categories. They are explicitly a
  combined total, not inferred by subtracting kernel bytes.
* **Decompression:** the Unix APE shell invokes the host's `gzip -dc`; that host
  executable is external, not an embedded decompressor. The shell stub is already
  counted in the Windows PE header. Native package/model loading remains in the
  combined driver category.
* **Carried C library inputs:** **19 header resources, 103,636 B** of source text.
  These are separate from any runtime library code linked into native drivers.
* **Models / templates:** all **5,869,042 B** are retained in the accounting.
  Exact network record classes: `N` headers **778 B**, `H` threshold-bank records
  **2,120,667 B**, `Q` action declarations **3,695,271 B**, `S` byte-string declarations
  **52,326 B**. `Q` and `S` include compilation-specific actions and template data;
  they are not omitted or counted as free runtime infrastructure. The format does
  not mark a separate template-only subrange within these declarations, so such a
  semantic template subtotal is **not separately measurable here**. Model bodies
  are plain serialized networks, not gzip compressed.

## Driver slices and six target routes

Six compilation targets do not imply six physically embedded host drivers.
The Windows ARM64 host uses the same x86_64 PE via OS emulation; it has no additional
native ARM64 PE driver in this APE. Both Windows target model families are present.
This is layout/source inspection, not evidence that a particular VM run passed.

| Host selector / driver | Offset | Carried B | Decoded file B |
|---|---:|---:|---:|
| Windows x86_64; Windows ARM64 through emulation | 0 | 97,280 | 97,280 |
| Linux x86_64 | 97,280 | 21,164 | 88,834 |
| Linux aarch64 / arm64 | 118,448 | 20,467 | 97,023 |
| Darwin x86_64 | 138,928 | 22,536 | 115,746 |
| Darwin arm64 | 161,472 | 21,811 | 115,746 |
| **Driver total** | | **183,258** | **514,629** |

The Windows PE has 1,536 B of headers (including the shell stub), followed by
`.text` raw extent 91,136 B at 1,536; `.rdata` 1,024 B at 92,672;
`.data` 3,072 B at 93,696; `.reloc` 512 B at 96,768. These file-aligned extents
are structural measurements, not separate OS/library/loader allocations.

| Kernel key | Blob B | Header B | Payload B | Entry offset | Import slot offset |
|---|---:|---:|---:|---:|---:|
| `kernel/arm64` | 7,704 | 40 | 7,664 | 6,912 | 7,656 |
| `kernel/x86_64` | 7,664 | 40 | 7,624 | 7,044 | 7,616 |

Offsets in the last two columns are relative to the payload; resource keys begin
with a NUL byte. Both resource hashes match adjacent `kernels/` files:

* arm64: `49641cc68100f29f174055eed1179d8d787260586193199495e0e337821523bd`
* x86_64: `97e45476e93dc381e7abdcb2aa0ce6eb72609e943c052c36957e0aab581a7fe5`

## Model sharing

**241 routes, 938 stage rows, 32 unique model bodies, 906 repeated references**.
All stored models are referenced and their complete-byte hashes are distinct.
Expanding every directory reference would name **368,261,754 B** of model bodies;
physical deduplication carries **5,869,042 B**, avoiding **362,392,712 B** of repeated
body bytes (directory and record framing are accounted independently above).

| Model index | Stage | Body B | References |
|---|---|---:|---:|
| 0 | e2 | 78,447 | 20 |
| 1 | e1 | 285,681 | 120 |
| 2 | elf | 111,755 | 13 |
| 3 | e2 | 76,937 | 1 |
| 4 | e3 | 1,103,053 | 108 |
| 5 | e4 | 8,919 | 72 |
| 6 | e4 | 105,392 | 72 |
| 7 | lower | 164,002 | 24 |
| 8 | e2 | 78,440 | 20 |
| 9 | elf | 103,084 | 13 |
| 10 | e2 | 76,930 | 1 |
| 11 | lower | 152,337 | 24 |
| 12 | e2 | 78,448 | 20 |
| 13 | elf | 136,888 | 13 |
| 14 | e2 | 76,938 | 1 |
| 15 | lower | 154,503 | 24 |
| 16 | e2 | 78,441 | 20 |
| 17 | elf | 127,605 | 13 |
| 18 | e2 | 76,931 | 1 |
| 19 | lower | 143,152 | 24 |
| 20 | e2 | 78,276 | 20 |
| 21 | elf | 134,928 | 13 |
| 22 | e2 | 76,766 | 1 |
| 23 | lower | 254,171 | 24 |
| 24 | e2 | 78,269 | 20 |
| 25 | elf | 124,528 | 13 |
| 26 | e2 | 76,759 | 1 |
| 27 | lower | 240,936 | 24 |
| 28 | tokenpp | 58,258 | 1 |
| 29 | tokenlex | 42,819 | 1 |
| 30 | e3 | 1,122,304 | 108 |
| 31 | units | 363,145 | 108 |

## Reproduce

Save the following standard-library-only reader as `/tmp/package-ledger.py` and run:

```sh
python3 /tmp/package-ledger.py /tmp/unisacc-call-label-candidate-928/unisacc-next.com
```

It refuses a different artifact hash. JSON output includes all model/resource hashes,
model sharing, decoded slice hashes, raw offsets and additive partition checks.
Format sources: `exec/c/pack.py`, `exec/c/net.py`, `exec/c/compilerpack.py`,
`unisa/ape.py`. No compiler, VM or product execution is needed.

```python
import collections,gzip,hashlib,json,pathlib,re,struct,sys
p=pathlib.Path(sys.argv[1]); b=p.read_bytes(); sha=lambda x:hashlib.sha256(x).hexdigest()
assert sha(b)=='81c04285acba68ef72cbe902f1474d551e457995c7b167e279b615d6de2cf3a6'
assert b[-16:-8]==b'UNIPKG1\n'
n=struct.unpack('<Q',b[-8:])[0]; start=len(b)-16-n; q=b[start:-16]; pos=0
parts=collections.Counter()
def line(kind):
 global pos
 end=q.index(b'\n',pos)+1; x=q[pos:end]; pos=end; parts[kind]+=len(x); return x.split()
h=line('package header'); nm,nd,nr=map(int,h[2:]); rows=[line('directory') for _ in range(nd)]
refs=collections.Counter(int(r[-1]) for r in rows); models=[]
for i in range(nm):
 m=line('model record headers'); length=int(m[1]); raw=q[pos:pos+length];pos+=length
 parts['model bodies']+=length
 models.append(dict(index=i,bytes=length,refs=refs[i],stages=sorted({r[2].decode() for r in rows if int(r[-1])==i}),sha256=sha(raw)))
netparts=collections.Counter()
for m in models:
 assert m['refs']>0
# Re-read model bodies to measure actual network record extents.
mp=0
for _ in range(1+nd): mp=q.index(b'\n',mp)+1
for _ in range(nm):
 end=q.index(b'\n',mp)+1; length=int(q[mp:end].split()[1]); raw=q[end:end+length];mp=end+length
 for record in raw.splitlines(keepends=True):
  tag=record.split(None,1)[0].decode(); assert tag in ('N','S','Q','H');netparts[tag]+=len(record)
assert sum(netparts.values())==parts['model bodies']
resources=[]
for i in range(nr):
 f=line('resource record headers'); k,l=map(int,f[1:]);key=q[pos:pos+k];pos+=k;raw=q[pos:pos+l];pos+=l
 parts['resource keys']+=k;kind='kernel bodies' if key.startswith(b'\0kernel/') else 'C header bodies';parts[kind]+=l
 item=dict(key=key.decode(),bytes=l,sha256=sha(raw))
 if kind=='kernel bodies':
  assert raw[:8]==b'UNIKERN1';isa,entry,slot,length=struct.unpack('<4Q',raw[8:40]);assert length+40==l
  item.update(kind=isa,entry=entry,slot=slot,header_bytes=40,payload_bytes=length)
 resources.append(item)
assert pos==len(q)==sum(parts.values()); assert len({m['sha256'] for m in models})==nm
pe=struct.unpack_from('<I',b,60)[0];assert b[pe:pe+4]==b'PE\0\0';count=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0];sizeheaders=struct.unpack_from('<I',b,pe+24+60)[0]
sections=[]
for i in range(count):
 o=pe+24+opt+40*i;name=b[o:o+8].rstrip(b'\0').decode();size,offset=struct.unpack_from('<II',b,o+16);sections.append(dict(name=name,offset=offset,bytes=size))
head=max([sizeheaders]+[s['offset']+s['bytes'] for s in sections]);slices=[];cursor=head;pad=0
for m in re.finditer(rb'([^\n]+)\) o=(\d+) n=(\d+) k=([0-9a-f]{16});;',b[:sizeheaders]):
 off=int(m[2])-1;size=int(m[3]);raw=b[off:off+size];assert sha(raw).startswith(m[4].decode());assert not any(b[cursor:off]);pad+=off-cursor
 dec=gzip.decompress(raw);slices.append(dict(selector=m[1].decode(),offset=off,bytes=size,uncompressed_bytes=len(dec),sha256=sha(dec)));cursor=off+size
assert len(slices)==4;assert not any(b[cursor:start]);pad+=start-cursor
assert head+sum(s['bytes'] for s in slices)+pad+len(q)+16==len(b)
result=dict(file=str(p),sha256=sha(b),total_bytes=len(b),package_offset=start,package_bytes=len(q),package_parts=dict(parts),routes=sorted({r[1].decode() for r in rows}),network_record_bytes=dict(netparts),stage_rows=nd,unique_models=nm,repeated_references=nd-nm,logical_model_bytes=sum(m['bytes']*m['refs'] for m in models),models=models,resource_count=nr,resources=resources,pe=dict(signature_offset=pe,header_bytes=sizeheaders,bytes=head,sections=sections),unix_slices=slices,alignment_bytes=pad,footer_bytes=16)
print(json.dumps(result,indent=2))
```

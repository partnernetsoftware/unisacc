"""Deterministic VERSIONINFO packaging for an unsigned PE32+ head.
Call before APE computes Unix-slice offsets. Already packed images, signed
images, existing resources, and insufficient section-header slack are refused.
No compiler/model decisions or original section contents are changed.
Format references:
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
https://learn.microsoft.com/en-us/windows/win32/menurc/vs-versioninfo
"""
import re
import struct

U32 = (1 << 32) - 1

def _u32(n):
    if not 0 <= n <= U32: raise ValueError('PE uint32 overflow')
    return n

def _align(n, alignment):
    return _u32((n + alignment - 1) // alignment * alignment)

def _version_block(key, value=b'', text=False, children=()):
    keybytes=(key+'\0').encode('utf-16le')
    block=bytearray(b'\0'*6+keybytes)
    block.extend(b'\0'*(-len(block)%4));block.extend(value)
    for child in children:
        block.extend(b'\0'*(-len(block)%4));block.extend(child)
    value_len=len(value)//2 if text else len(value)
    if len(block)>65535 or value_len>65535: raise ValueError('VERSIONINFO uint16 overflow')
    struct.pack_into('<HHH',block,0,len(block),value_len,int(text))
    return bytes(block)

def _version_info(product_name, version):
    if not isinstance(product_name,str) or not product_name or len(product_name)>1024 or any(ord(c)<32 for c in product_name):
        raise ValueError('product_name must be bounded, nonempty text without controls')
    if not isinstance(version,str) or not re.fullmatch(r'[0-9]{1,5}(?:\.[0-9]{1,5}){2,3}',version):
        raise ValueError('version must have three or four decimal uint16 components')
    parts=[int(x) for x in version.split('.')]
    if any(x>65535 for x in parts):raise ValueError('version component exceeds uint16')
    parts += [0]*(4-len(parts));ms=parts[0]<<16|parts[1];ls=parts[2]<<16|parts[3]
    fixed=struct.pack('<13I',0xFEEF04BD,0x10000,ms,ls,ms,ls,0x3F,0,0x40004,1,0,0,0)
    strings=[]
    for key,value in [('ProductName',product_name),('ProductVersion',version),('FileDescription',product_name),('FileVersion',version)]:
        strings.append(_version_block(key,(value+'\0').encode('utf-16le'),True))
    table=_version_block('040904B0',text=True,children=strings)
    sfi=_version_block('StringFileInfo',text=True,children=[table])
    translation=_version_block('Translation',struct.pack('<HH',0x0409,1200))
    vfi=_version_block('VarFileInfo',text=True,children=[translation])
    return _version_block('VS_VERSION_INFO',fixed,children=[sfi,vfi])

def add_version_info(unsigned_pe_bytes, product_name, version):
    """Return PE head bytes with a new .rsrc/RT_VERSION; reject unsafe inputs.
    Raises ValueError on malformed/unbounded/unsupported layout, including any
    overlay. Three-component versions get a zero fourth fixed-file component.
    Existing raw offsets, RVAs, section headers and raw bytes remain unchanged.
    """
    if not isinstance(unsigned_pe_bytes,bytes):raise ValueError('input must be immutable bytes')
    src=unsigned_pe_bytes
    if len(src)<64 or len(src)>U32 or src[:2]!=b'MZ':raise ValueError('invalid DOS header or file size')
    pe=struct.unpack_from('<I',src,60)[0]
    if pe<64 or pe+24>len(src) or src[pe:pe+4]!=b'PE\0\0':raise ValueError('invalid PE signature')
    count=struct.unpack_from('<H',src,pe+6)[0];opt_size=struct.unpack_from('<H',src,pe+20)[0]
    opt=pe+24;sects=opt+opt_size
    if not 1<=count<96 or opt_size<152 or sects+40*count>len(src):raise ValueError('invalid PE header bounds')
    if struct.unpack_from('<H',src,opt)[0]!=0x20B:raise ValueError('requires PE32+')
    section_align,file_align=struct.unpack_from('<II',src,opt+32)
    if not (512<=file_align<=65536 and file_align&(file_align-1)==0 and
            section_align>=file_align and section_align<=0x10000000 and section_align&(section_align-1)==0):
        raise ValueError('unsupported section/file alignment')
    directories=struct.unpack_from('<I',src,opt+108)[0]
    if not 5<=directories<=(opt_size-112)//8:raise ValueError('invalid data directory count')
    if any(struct.unpack_from('<II',src,opt+112+8*i)!=(0,0) for i in [2,4]):
        raise ValueError('existing resource or certificate directory is forbidden')
    headers=struct.unpack_from('<I',src,opt+60)[0]
    image_size=struct.unpack_from('<I',src,opt+56)[0]
    if headers>len(src) or headers%file_align or image_size%section_align:
        raise ValueError('invalid header/image size alignment')
    raw_ranges=[];virtual_ranges=[];max_raw=headers;max_virtual=_align(headers,section_align)
    for i in range(count):
        at=sects+40*i;name=src[at:at+8].rstrip(b'\0')
        vsize,rva,rawsize,raw=struct.unpack_from('<4I',src,at+8)
        if name==b'.rsrc':raise ValueError('existing resource section is forbidden')
        if rva%section_align or rva<_align(headers,section_align):raise ValueError('invalid section RVA')
        vend=_u32(rva+max(vsize,rawsize));max_virtual=max(max_virtual,vend)
        if vend>rva:virtual_ranges.append((rva,vend))
        if rawsize:
            end=_u32(raw+rawsize)
            if raw<headers or raw%file_align or rawsize%file_align or end>len(src):raise ValueError('invalid raw section range')
            raw_ranges.append((raw,end));max_raw=max(max_raw,end)
    for ranges in [raw_ranges,virtual_ranges]:
        ranges.sort()
        if any(b[0]<a[1] for a,b in zip(ranges,ranges[1:])):raise ValueError('overlapping sections')
    if image_size<_align(max_virtual,section_align):raise ValueError('SizeOfImage truncates sections')
    if len(src)!=max_raw:raise ValueError('overlay forbidden: pass un-packed PE head, never complete APE')
    new_header=sects+40*count
    first_raw=min((a for a,b in raw_ranges),default=headers)
    if new_header+40>min(headers,first_raw) or any(src[new_header:new_header+40]):
        raise ValueError('insufficient zero section-header slack')
    info=_version_info(product_name,version)
    rva=_align(max(image_size,max_virtual),section_align)
    raw=_align(len(src),file_align)
    resource=bytearray(88)
    # Three ID-only directories: RT_VERSION / name 1 / en-US language 0409.
    for at,ident,target,is_directory in [(0,16,24,True),(24,1,48,True),(48,0x0409,72,False)]:
        struct.pack_into('<IIHHHH',resource,at,0,0,0,0,0,1)
        struct.pack_into('<II',resource,at+16,ident,target|(0x80000000 if is_directory else 0))
    struct.pack_into('<IIII',resource,72,_u32(rva+88),len(info),1200,0)
    resource.extend(info);rawsize=_align(len(resource),file_align)
    _u32(raw+rawsize);new_image=_align(_u32(rva+len(resource)),section_align)
    initialized=struct.unpack_from('<I',src,opt+8)[0]
    out=bytearray(src);out.extend(b'\0'*(raw-len(out)));out.extend(resource);out.extend(b'\0'*(rawsize-len(resource)))
    struct.pack_into('<H',out,pe+6,count+1)
    struct.pack_into('<I',out,opt+8,_u32(initialized+rawsize))
    struct.pack_into('<I',out,opt+56,new_image)
    struct.pack_into('<I',out,opt+64,0)  # old unsigned checksum is no longer valid
    struct.pack_into('<II',out,opt+112+16,rva,len(resource))
    struct.pack_into('<8sIIIIIIHHI',out,new_header,b'.rsrc\0\0\0',len(resource),rva,rawsize,raw,0,0,0,0,0x40000040)
    return bytes(out)

// Standalone service-owned ZIP32 parser; generated from handoff.mjs parser.
import zlib from 'node:zlib';
const MAX=64*1024*1024,need=(v,m)=>{if(!v)throw Error(m)};
function crc32(b){let c=0xffffffff;for(const x of b){c^=x;for(let j=0;j<8;j++)c=(c>>>1)^((c&1)?0xedb88320:0)}return(c^0xffffffff)>>>0}
function extras(b,local){let p=0;const ids=[];while(p<b.length){need(p+4<=b.length,'ZIP extra header');const id=b.readUInt16LE(p),n=b.readUInt16LE(p+2);need(p+4+n<=b.length&&!ids.includes(id),'ZIP extra extent/duplicate');need(id===0x5855&&n===(local?12:8),'unsupported ZIP extra');ids.push(id);p+=4+n}return ids}
// Generic structural parser is exported for raw CI-format regression only;
// production callers ALWAYS use zip() below to enforce fixed inventories.
export function parseZip(raw){
  need(Buffer.isBuffer(raw)&&raw.length>=22&&raw.length<=MAX,'ZIP size');
  const end=raw.length-22;need(raw.readUInt32LE(end)===0x06054b50&&!raw.readUInt16LE(end+20),'ZIP end/comment');
  const count=raw.readUInt16LE(end+10),size=raw.readUInt32LE(end+12),start=raw.readUInt32LE(end+16);
  need(!raw.readUInt16LE(end+4)&&!raw.readUInt16LE(end+6)&&count===raw.readUInt16LE(end+8)&&count>0&&count<20000&&start+size===end,'ZIP disk/inventory');
  const result=new Map(),intervals=[],details=[];let p=start,total=0;
  for(let i=0;i<count;i++){
    need(p+46<=end&&raw.readUInt32LE(p)===0x02014b50,'ZIP central header');
    const f=raw.readUInt16LE(p+8),method=raw.readUInt16LE(p+10),crc=raw.readUInt32LE(p+16),packed=raw.readUInt32LE(p+20),len=raw.readUInt32LE(p+24),nl=raw.readUInt16LE(p+28),xl=raw.readUInt16LE(p+30),cl=raw.readUInt16LE(p+32),disk=raw.readUInt16LE(p+34),mode=raw.readUInt32LE(p+38)>>>16,at=raw.readUInt32LE(p+42);
    need(p+46+nl+xl+cl<=end&&nl>0&&nl<1024&&!cl&&!disk&&!(f&~0x808)&&[0,8].includes(method),'unsupported ZIP features');
    const extraIds=extras(raw.subarray(p+46+nl,p+46+nl+xl),false);
    const nb=raw.subarray(p+46,p+46+nl),name=nb.toString('utf8'),parts=name.replace(/\/$/,'').split('/');
    need(Buffer.from(name).equals(nb)&&parts.every(x=>/^[A-Za-z0-9_. -]+$/.test(x)&&x!=='.'&&x!=='..')&&!result.has(name),'unsafe/duplicate ZIP path');
    const dir=name.endsWith('/'),type=mode&0xf000;need(type===0||type===(dir?0x4000:0x8000),'ZIP link/special file');
    need(at+30<=start&&raw.readUInt32LE(at)===0x04034b50&&raw.readUInt16LE(at+6)===f&&raw.readUInt16LE(at+8)===method,'ZIP local identity');
    for(const [off,want]of [[14,crc],[18,packed],[22,len]])need(raw.readUInt32LE(at+off)===want||((f&8)&&raw.readUInt32LE(at+off)===0),'ZIP local size/CRC');
    const lnl=raw.readUInt16LE(at+26),lxl=raw.readUInt16LE(at+28),begin=at+30+lnl+lxl;
    need(lnl===nl&&raw.subarray(at+30,at+30+lnl).equals(nb)&&begin+packed<=start&&len<=MAX,'ZIP local extent');
    const localExtraIds=extras(raw.subarray(at+30+lnl,begin),true);
    let finish=begin+packed;
    if(f&8){need(finish+12<=start,'ZIP descriptor extent');let d=finish;if(raw.readUInt32LE(d)===0x08074b50){d+=4;need(d+12<=start,'ZIP signed descriptor extent')}need(raw.readUInt32LE(d)===crc&&raw.readUInt32LE(d+4)===packed&&raw.readUInt32LE(d+8)===len,'ZIP descriptor identity');finish=d+12}
    const data=raw.subarray(begin,begin+packed),out=method===0?data:zlib.inflateRawSync(data,{maxOutputLength:MAX});
    need(out.length===len&&crc32(out)===crc&&(!dir||len===0),'ZIP size/CRC');total+=len;need(total<=MAX,'ZIP inflated budget');
    result.set(name,out);details.push({name,method,flags:f,mode,extraIds,localExtraIds,descriptor:!!(f&8)});intervals.push([at,finish]);p+=46+nl+xl+cl;
  }
  need(p===end,'ZIP central extent');intervals.sort((a,b)=>a[0]-b[0]);let offset=0;for(const [a,b]of intervals){need(a===offset,'ZIP gap/overlap');offset=b}need(offset===start,'ZIP payload extent');
  // A regular-file ancestor must never be used as a directory by extraction.
  for(const name of result.keys()){let prefix='';for(const part of name.split('/').slice(0,-1)){prefix+=part;need(!result.has(prefix),'ZIP file/directory collision');prefix+='/'}}
  result.details=details;return result;
}

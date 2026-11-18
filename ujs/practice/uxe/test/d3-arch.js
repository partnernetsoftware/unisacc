// Extruded masonry bounded by a semicircle and its two vertical jambs.
export function archMeshes(bay, mode) {
  const [center, spring, radius, front, back, halfSpan, deck, base, segments] = bay;
  const face = [], intrados = [], faceUV = [], innerUV = [];
  function quad(target, uv, a,b,c,d, coords) {
    const corners=[a,b,c,d];
    for (const k of [0,1,2,0,2,3]) {
      target.push(...corners[k]);
      uv.push(...coords[k]);
    }
  }
  const scaleUV = (x,y)=>[x/2,y/2];
  function strip(a,b) {
    for(const z of [front,back]) {
      quad(face,faceUV,[a.x,a.y,z],[b.x,b.y,z],[b.x,deck,z],[a.x,deck,z],
        [scaleUV(a.x,a.y),scaleUV(b.x,b.y),scaleUV(b.x,deck),scaleUV(a.x,deck)]);
    }
  }
  function lining(a,b,s,t) {
    quad(intrados,innerUV,[a.x,a.y,front],[b.x,b.y,front],[b.x,b.y,back],[a.x,a.y,back],
      [[s,0],[t,0],[t,front-back],[s,front-back]]);
  }
  const left=center-halfSpan, right=center+halfSpan;
  const yRadius=mode==="low-crown" ? .8*radius : radius;
  if(mode==="sealed") strip({x:left,y:base},{x:right,y:base});
  else {
    strip({x:left,y:base},{x:center-radius,y:base});
    strip({x:center+radius,y:base},{x:right,y:base});
    for(const side of [-1,1]) {
      const x=center+side*radius;
      lining({x,y:base},{x,y:spring},base,spring);
    }
    const arc = angle=>({x:center+radius*Math.cos(angle),y:spring+yRadius*Math.sin(angle)});
    for(let i=0;i<segments;i++) {
      const a=i*Math.PI/segments, b=(i+1)*Math.PI/segments;
      const p=arc(a), q=arc(b);
      strip(p,q);
      lining(p,q,a*radius,b*radius);
    }
  }
  for(const x of [left,right]) lining({x,y:base},{x,y:deck},base,deck);
  lining({x:left,y:deck},{x:right,y:deck},left,right);
  return [[face,faceUV],[intrados,innerUV]].map(([positions,uvs])=>({
    positions:new Float32Array(positions),uvs:new Float32Array(uvs),vCount:positions.length/3
  }));
}

export async function loadBays() {
  const response=await fetch(new URL("d3-arch.tsv",import.meta.url));
  if(!response.ok) throw new Error("拱圈规格读取失败");
  return (await response.text()).trim().split("\n").slice(1).map(line=>{
    const values=line.split("\t").map(Number);
    if(values.length!==9 || !values.every(Number.isFinite)) throw new Error("拱圈规格格式错误");
    return values.map((value,i)=>i===8?value:value/1000);
  });
}

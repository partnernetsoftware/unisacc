// Extruded masonry bounded by a semicircle and its two vertical jambs.
export function archMeshes(bay, mode) {
  const [center, spring, radius, front, back, halfSpan, deck, base, segments] = bay;
  const face = [], intrados = [], faceUV = [], innerUV = [], faceNormals = [], innerNormals = [];
  function quad(target, uv, normals, normal, a,b,c,d, coords) {
    const corners=[a,b,c,d];
    for (const k of [0,1,2,0,2,3]) {
      target.push(...corners[k]);
      uv.push(...coords[k]);
      normals.push(...normal);
    }
  }
  const scaleUV = (x,y)=>[x/2,y/2];
  function strip(a,b) {
    for(const z of [front,back]) {
      quad(face,faceUV,faceNormals,[0,0,z===front?1:-1],[a.x,a.y,z],[b.x,b.y,z],[b.x,deck,z],[a.x,deck,z],
        [scaleUV(a.x,a.y),scaleUV(b.x,b.y),scaleUV(b.x,deck),scaleUV(a.x,deck)]);
    }
  }
  function lining(a,b,s,t,normal) {
    quad(intrados,innerUV,innerNormals,normal,[a.x,a.y,front],[b.x,b.y,front],[b.x,b.y,back],[a.x,a.y,back],
      [[s/2,0],[t/2,0],[t/2,(front-back)/2],[s/2,(front-back)/2]]);
  }
  const left=center-halfSpan, right=center+halfSpan;
  const yRadius=mode==="low-crown" ? .8*radius : radius;
  if(mode==="sealed") strip({x:left,y:base},{x:right,y:base});
  else {
    strip({x:left,y:base},{x:center-radius,y:base});
    strip({x:center+radius,y:base},{x:right,y:base});
    for(const side of [-1,1]) {
      const x=center+side*radius;
      const start = side === 1 ? 0 : Math.PI * radius;
      lining({x,y:base},{x,y:spring},start-side*(spring-base),start,[-side,0,0]);
    }
    const arc = angle=>({x:center+radius*Math.cos(angle),y:spring+yRadius*Math.sin(angle)});
    for(let i=0;i<segments;i++) {
      const a=i*Math.PI/segments, b=(i+1)*Math.PI/segments;
      const p=arc(a), q=arc(b);
      strip(p,q);
      lining(p,q,a*radius,b*radius,[p.y-q.y,q.x-p.x,0]);
    }
  }
  for(const x of [left,right]) lining({x,y:base},{x,y:deck},base,deck,[x===left?-1:1,0,0]);
  lining({x:left,y:deck},{x:right,y:deck},left,right,[0,1,0]);
  return [[face,faceUV,faceNormals],[intrados,innerUV,innerNormals]].map(([positions,uvs,normals])=>({
    positions:new Float32Array(positions),uvs:new Float32Array(uvs),
    normals:new Float32Array(normals),vCount:positions.length/3
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

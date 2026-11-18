export async function closeCamera() {
  const response=await fetch(new URL("d4-framing.tsv",import.meta.url));
  if(!response.ok) throw new Error("D4 机位规格读取失败");
  const row=(await response.text()).trim().split("\n")[1]?.split("\t").map(Number);
  if(row?.length!==11 || !row.every(Number.isFinite)) throw new Error("D4 机位规格格式错误");
  const [, , , , , , x,y,z,target,focal]=row;
  return {eye:[x/1000,y/1000,z/1000],target:[0,target/1000,0],fovy:2*Math.atan(360/focal)};
}

export function barrelMesh(width,length) {
  const positions=[], normals=[], sides=16;
  const rings=[[0,.72],[.08,.93],[.20,1],[.80,1],[.92,.93],[1,.72]];
  function point(ring,side) {
    const [y,scale]=rings[ring], a=2*Math.PI*side/sides;
    const radial=[Math.cos(a),0,Math.sin(a)];
    return {pos:[radial[0]*width*scale/2,(y-.5)*length,radial[2]*width*scale/2],radial};
  }
  function emit(pos,normal) { positions.push(...pos); normals.push(...normal); }
  for(let side=0;side<sides;side++) {
    for(let ring=0;ring<rings.length-1;ring++) {
      const a=point(ring,side),b=point(ring,side+1),c=point(ring+1,side+1),d=point(ring+1,side);
      for(const p of [a,b,c,a,c,d]) emit(p.pos,p.radial);
    }
    for(const ring of [0,rings.length-1]) {
      const normal=[0,ring===0?-1:1,0];
      emit([0,normal[1]*length/2,0],normal);
      emit(point(ring,side).pos,normal);
      emit(point(ring,side+1).pos,normal);
    }
  }
  return {positions:new Float32Array(positions),normals:new Float32Array(normals)};
}

import { paintedCanopy, paintedLeafMesh } from "./d3-painted.js";

export function liningCourses(lining) {
  const ranges=[[0,.19,"barrelLip"],[.19,.22,"barrelJoint"],[.22,.54,"barrelMiddle"],
    [.54,.57,"barrelJoint"],[.57,1,"barrelDeep"]];
  const layered=new Map();
  const mix=(a,b,t)=>a.map((v,i)=>v+(b[i]-v)*t);
  for(const [start,end,material] of ranges) {
    if(!layered.has(material)) layered.set(material,{positions:[],uvs:[],normals:[]});
    const mesh=layered.get(material);
    for(let i=0;i<lining.positions.length;i+=18) {
      const take=(source,stride,vertex)=>Array.from(source.slice((i/3+vertex)*stride,(i/3+vertex+1)*stride));
      for(const [key,stride] of [["positions",3],["uvs",2],["normals",3]]) {
        const source=lining[key];
        const a=take(source,stride,0),b=take(source,stride,1),c=take(source,stride,2),d=take(source,stride,5);
        const p=mix(a,d,start),q=mix(b,c,start),r=mix(b,c,end),s=mix(a,d,end);
        mesh[key].push(...p,...q,...r,...p,...r,...s);
      }
    }
  }
  return [...layered].map(([material,mesh])=>[material,{
    positions:new Float32Array(mesh.positions),uvs:new Float32Array(mesh.uvs),
    normals:new Float32Array(mesh.normals),vCount:mesh.positions.length/3
  }]);
}

export function brushLeafMesh(seed,brushed) {
  const mesh=paintedLeafMesh(seed);
  if(!brushed) return mesh;
  for(let i=0;i<mesh.positions.length;i+=9) {
    const k=i/9;
    const edge=j=>1+.11*Math.sin(j*3.006+seed);
    for(let axis=0;axis<2;axis++) {
      mesh.positions[i+3+axis]*=edge(k);
      mesh.positions[i+6+axis]*=edge((k+1)%23);
    }
    mesh.normals[i]=.15*Math.sin(k*.79+seed);
    mesh.normals[i+1]=.55+.20*Math.sin(k*1.38+seed);
    mesh.normals[i+2]=.83+.11*Math.cos(k*.93+seed);
  }
  return mesh;
}

export function brushCanopy(x,y,z,r,brushed,emit) {
  paintedCanopy(x,y,z,r,(material,variant,px,py,pz,size,yaw,pitch,roll)=>{
    if(!brushed) { emit(material,variant,px,py,pz,size,yaw,pitch,roll); return; }
    const brush=Math.sin(px*4.7+py*2.2);
    emit(material,variant,px,py,pz,size*(.94+.10*brush),yaw+.20*brush,pitch+.14*brush,roll+.25*brush);
    if(material==="leafSun" && brush>-.15)
      emit("leafPaint",(variant+1)%3,px+size*.29,py-size*.14,pz+.09,size*.28,yaw,pitch,roll+.44);
  });
}

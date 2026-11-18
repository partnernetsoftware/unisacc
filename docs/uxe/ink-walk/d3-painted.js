import { leafMesh } from "./d3-foliage.js";

export function paintedLeafMesh(seed) {
  const points=[], positions=[], normals=[];
  const count=23;
  for(let i=0;i<count;i++) {
    const angle=i*2*Math.PI/count;
    const edge=.82+.12*Math.sin(i*2.39+seed)+.10*Math.cos(i*1.24+seed*3);
    points.push([Math.cos(angle)*edge,Math.sin(angle)*edge*.79,0]);
  }
  for(let i=0;i<count;i++) {
    positions.push(0,0,.25,...points[i],...points[(i+1)%count]);
    normals.push(0,.55,.84,0,.55,.84,0,.55,.84);
  }
  return {positions:new Float32Array(positions),normals:new Float32Array(normals),vCount:positions.length/3};
}

export function paintedCanopy(x,y,z,r,emit) {
  for(let i=0;i<15;i++) {
    const angle=i*2.399963+x, reach=.27+Math.sqrt(i/14)*.65;
    const cx=Math.cos(angle)*reach, cy=Math.sin(angle)*reach;
    const size=r*(.43+.21*Math.sin(i*1.27+x)**2);
    const px=x+cx*r, py=y+cy*r*.78, pz=z+(.16+.33*reach)*r;
    emit("leafInk",i%3,px,py,pz,size,.14,.09,angle*.1);
    if(i%5!==4) emit("leafPaint",i%3,px-size*.09,py+size*.16,pz+.10,
      size*.83,.14,.09,angle*.1);
    if(cy>-.31 && i%3!==1) emit("leafSun",i%3,px-size*.18,py+size*.29,pz+.20,
      size*.46,.14,.09,angle*.1);
  }
}

export function tuftMesh() {
  const tuft=leafMesh(.43);
  for(let i=0;i<tuft.positions.length;i+=3) {
    tuft.positions[i]*=.66;
    tuft.positions[i+1]=Math.max(0,tuft.positions[i+1])*.41;
    tuft.positions[i+2]*=.42;
  }
  return tuft;
}

export function hangingVines(deck, depthAt, addMesh, instance) {
  const stems=[], leaves=[];
  const tri=(arr,a,b,c)=>arr.push(...a,...b,...c);
  for(let i=0;i<29;i++) {
    const anchor=-18+1.27*i, length=.55+(i*17%27)*.105;
    for(let j=0;j<16;j++) {
      const node=k=>{
        const t=k/16, x=anchor+Math.sin(t*6+i*1.6)*.21*t;
        return [x,deck-.04-length*t,2.565+depthAt(x)];
      };
      const a=node(j),b=node(j+1),w=.027+(16-j)*.0026;
      const c=[a[0]+w,a[1],a[2]],d=[b[0]+w,b[1],b[2]];
      tri(stems,a,c,d); tri(stems,a,d,b);
      if((j+i)%3!==1) {
        const direction=j%2?1:-1, size=.10+(i*11+j*23)%13*.013;
        const p=[a[0],a[1],a[2]+.02], tip=[a[0]+direction*size*2,a[1]-.19,a[2]+.024];
        tri(leaves,p,[a[0]+direction*size,a[1]+.052,a[2]+.021],tip);
        tri(leaves,p,tip,[a[0]+direction*size*.55,a[1]-.19,a[2]+.02]);
      }
    }
  }
  const emit=(material,positions)=>instance(material,
    addMesh({positions:new Float32Array(positions),vCount:positions.length/3}),0,0,0,1);
  emit("vine",stems);
  emit("vineMoss",leaves);
}

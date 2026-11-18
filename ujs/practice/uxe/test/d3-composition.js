import { BOX_MESH, MESH_OCTA } from "../meshes.js";

export async function loadComposition() {
  const reply = await fetch(new URL("d3-composition.tsv", import.meta.url));
  if (!reply.ok) throw new Error("构图规格读取失败");
  const fields = (await reply.text()).trim().split("\n")[1]?.split("\t").map(Number);
  if (fields?.length !== 8 || !fields.every(Number.isFinite)) throw new Error("构图规格格式错误");
  const [x,y,z,scale,houseX,width,height,depth] = fields.map(value => value/1000);
  return {eye:[x,y,z],scale,houseX,width,height,depth};
}

export function stretchBridge(mesh, scale, bottom) {
  for (let i=1;i<mesh.positions.length;i+=3) {
    mesh.positions[i] = bottom + (mesh.positions[i]-bottom)*scale;
    mesh.normals[i] /= scale;
  }
  return mesh;
}

export function buildClifftop(layout, deck, addMesh, instance) {
  const {houseX:x,width,height,depth} = layout;
  const z=-1, front=z+depth/2;
  function blockMesh(w,h,d) {
    const positions = BOX_MESH.map((v,i) => v*[w,h,d][i%3]);
    return addMesh({positions,vCount:positions.length/3});
  }
  instance("abutment",blockMesh(width,deck+2,depth+1.2),x,(deck-2)/2,z,1);
  instance("plaster",blockMesh(width,height,depth),x,deck+height/2,z,1);
  const slope = Math.PI/6;
  const roofSize = (width/2+.45)/Math.cos(slope);
  const tile = blockMesh(roofSize/5+.045,.095,(depth+1)/9+.045);
  const gable = new Float32Array([-width/2,0,0,width/2,0,0,0,width/2*Math.tan(slope),0]);
  const gableMesh=addMesh({positions:gable,vCount:3});
  for (const side of [-1,1]) {
    instance("plaster",gableMesh,x,deck+height,z+side*depth/2,1);
    for (let row=0;row<5;row++) for (let col=0;col<9;col++) {
      const run = (row+.5)*(width/2+.45)/5;
      instance((row+col)%6===0?"roofPale":"roof",tile,x+side*run,
        deck+height+(width/2-run)*Math.tan(slope),z+(col-4)*(depth+1)/9,1,0,0,-side*slope);
    }
  }
  const beam=blockMesh(.16,height,.13);
  for (const edge of [-.48,.48]) instance("timber",beam,x+edge*width,deck+height/2,front+.045,1);
  const sill=blockMesh(width+.55,.23,1.15);
  instance("timber",sill,x,deck+.75,front+.46,1);
  const baluster=blockMesh(.105,.83,.12);
  for (let i=-6;i<=6;i++) instance("timber",baluster,x+i*width/13,deck+1.23,front+1,1);
  instance("timber",blockMesh(width,.11,.14),x,deck+1.68,front+1,1);
  const window=blockMesh(.66,.96,.12);
  for (const offset of [-.31,.24]) instance("window",window,x+offset*width,deck+height*.49,front+.065,1);
  instance("abutment",blockMesh(.65,2.2,.7),x+width*.28,deck+height+1.2,z-.48,1);
  for (let i=0;i<14;i++) {
    const widthScale=.68+.11*(i%5);
    instance(i%3===0?"cliffLight":"cliffShade",MESH_OCTA,x-2.8+(i%4)*1.05,
      -.3+Math.floor(i/4)*deck*.21,front+1.3+.43*Math.cos(i*2),1.8*widthScale,i*.81);
  }
  for(let i=0;i<7;i++) {
    instance("ridge",MESH_OCTA,-36+i*14,-12+3*Math.sin(i*2.5),-46-4*(i%2),16+i%3*2,.4*i);
  }
}

import { BOX_MESH, MESH_OCTA } from "../meshes.js";

export async function loadComposition() {
  const reply = await fetch(new URL("d3-composition.tsv", import.meta.url));
  if (!reply.ok) throw new Error("构图规格读取失败");
  const fields = (await reply.text()).trim().split("\n")[1]?.split("\t").map(Number);
  if (fields?.length !== 8 || !fields.every(Number.isFinite)) throw new Error("构图规格格式错误");
  const [x,y,z,scale,houseX,width,height,depth] = fields.map(value => value/1000);
  return {eye:[x,y,z],scale,houseX,width,height,depth};
}

export async function loadPalette() {
  const reply = await fetch(new URL("d3-palette.tsv", import.meta.url));
  if (!reply.ok) throw new Error("色板读取失败");
  const rows = (await reply.text()).trim().split("\n").slice(1);
  const roles = ["skyTop","skyHorizon","leafLight","leafDark","stone","barrel","leaf.light"];
  if (rows.length !== roles.length) throw new Error("色板行数错误");
  return Object.fromEntries(rows.map((row,i) => {
    const [role,...fields] = row.split("\t");
    const rgb = fields.map(Number);
    if (role !== roles[i] || rgb.length !== 3 ||
        !rgb.every(v=>Number.isInteger(v) && v>=0 && v<=255)) throw new Error("色板格式错误");
    return [role,rgb];
  }));
}

export function tintSky(rgba, width, height, palette) {
  for (let y=0; y<height; y++) {
    const old=[66+y,126+Math.floor(y*3/4),184+Math.floor(y/2)];
    const delta=palette.skyTop.map((v,j)=>v+(palette.skyHorizon[j]-v)*y/(height-1)-old[j]);
    for(let x=0;x<width;x++) {
      const p=(y*width+x)*4;
      // The generated clouds have a constant blue-to-red difference of 16.
      // Recover the uncovered fraction before remapping the blue gradient.
      const clear=Math.max(0,Math.min(1,(rgba[p+2]-rgba[p]-16)/(old[2]-old[0]-16)));
      for(let j=0;j<3;j++) rgba[p+j]=Math.max(0,Math.min(255,Math.round(rgba[p+j]+clear*delta[j])));
    }
  }
}

export function stretchBridge(mesh, scale, bottom) {
  for (let i=1;i<mesh.positions.length;i+=3) {
    mesh.positions[i] = bottom + (mesh.positions[i]-bottom)*scale;
    mesh.normals[i] /= scale;
  }
  return mesh;
}

function blockMesh(addMesh,w,h,d) {
  const dimensions=[w,h,d];
  const positions=BOX_MESH.map((v,i)=>v*dimensions[i%3]);
  return addMesh({positions,vCount:positions.length/3});
}

export function bridgeLines(deck, addMesh, instance, rails) {
  if (rails) {
    const post=blockMesh(addMesh,.13,.67,.14);
    const bar=blockMesh(addMesh,24,.078,.10);
    for(const z of [-.35,2.34]) {
      for(let x=-12;x<=12;x+=1.5) instance("iron",post,x,deck+.335,z,1);
      instance("iron",bar,0,deck+.6,z,1);
      instance("iron",bar,0,deck+.11,z,1);
    }
  }
  const positions=[];
  for (const [index,anchor] of [-10.7,-4.5,3.8,4.4,11.1].entries()) {
    const length=1.0+(index%3)*.84;
    for(let step=0;step<12;step++) {
      const a=step/12,b=(step+1)/12;
      const x0=anchor+Math.sin(a*8+index)*.16;
      const x1=anchor+Math.sin(b*8+index)*.16;
      const y0=deck-.15-length*a,y1=deck-.15-length*b;
      const w=.026+(.055*(1-b)), z=2.53+index*.002;
      positions.push(x0-w,y0,z,x0+w,y0,z,x1+w,y1,z,
        x0-w,y0,z,x1+w,y1,z,x1-w,y1,z);
      if(step%2===0) {
        const side=step%4===0?.20:-.20;
        positions.push(x0,y0,z,x0+side,y0-.11,z,x0,y0-.19,z);
      }
    }
  }
  instance("vine",addMesh({positions:new Float32Array(positions),vCount:positions.length/3}),0,0,0,1);
}

export function buildClifftop(layout, deck, addMesh, instance) {
  const {houseX:x,width,height,depth} = layout;
  const z=-1, front=z+depth/2;
  const block=(w,h,d)=>blockMesh(addMesh,w,h,d);
  instance("abutment",block(width,deck+2,depth+1.2),x,(deck-2)/2,z,1);
  instance("plaster",block(width,height,depth),x,deck+height/2,z,1);
  const slope = Math.PI/6;
  const roofSize = (width/2+.45)/Math.cos(slope);
  const tile = block(roofSize/5+.045,.095,(depth+1)/9+.045);
  const eave = block(.14,.12,depth+1.03);
  const gable = new Float32Array([-width/2,0,0,width/2,0,0,0,width/2*Math.tan(slope),0]);
  const gableMesh=addMesh({positions:gable,vCount:3});
  for (const side of [-1,1]) {
    instance("iron",eave,x+side*(width/2+.42),deck+height-.28,z,1);
    instance("plaster",gableMesh,x,deck+height,z+side*depth/2,1);
    for (let row=0;row<5;row++) for (let col=0;col<9;col++) {
      const run = (row+.5)*(width/2+.45)/5;
      instance((row+col)%6===0?"roofPale":"roof",tile,x+side*run,
        deck+height+(width/2-run)*Math.tan(slope),z+(col-4)*(depth+1)/9,1,0,0,-side*slope);
    }
  }
  const beam=block(.16,height,.13);
  for (const edge of [-.48,.48]) instance("timber",beam,x+edge*width,deck+height/2,front+.045,1);
  const sill=block(width+.55,.23,1.15);
  instance("timber",sill,x,deck+.75,front+.46,1);
  const baluster=block(.105,.83,.12);
  for (let i=-6;i<=6;i++) instance("timber",baluster,x+i*width/13,deck+1.23,front+1,1);
  instance("timber",block(width,.11,.14),x,deck+1.68,front+1,1);
  const window=block(.66,.96,.12);
  for (const offset of [-.31,.24]) instance("window",window,x+offset*width,deck+height*.49,front+.065,1);
  instance("abutment",block(.65,2.2,.7),x+width*.28,deck+height+1.2,z-.48,1);
  for (let i=0;i<14;i++) {
    const widthScale=.68+.11*(i%5);
    instance(i%3===0?"cliffLight":"cliffShade",MESH_OCTA,x-2.8+(i%4)*1.05,
      -.3+Math.floor(i/4)*deck*.21,front+1.3+.43*Math.cos(i*2),1.8*widthScale,i*.81);
  }
  for(let i=0;i<7;i++) {
    instance("ridge",MESH_OCTA,-36+i*14,-12+3*Math.sin(i*2.5),-46-4*(i%2),16+i%3*2,.4*i);
  }
}

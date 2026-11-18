const dot=(a,b)=>a.reduce((sum,v,i)=>sum+v*b[i],0);
const sub=(a,b)=>a.map((v,i)=>v-b[i]);
const norm=(v)=>Math.hypot(...v);
function unit(v) {
  const length=norm(v);
  return v.map(x=>x/length);
}

function kneeFor(rest, hip, foot, extend) {
  const [originalHip,originalKnee,originalFoot]=rest;
  const upper=norm(sub(originalKnee,originalHip))+extend;
  const lower=norm(sub(originalKnee,originalFoot));
  const delta=sub(foot,hip), d=norm(delta), axis=unit(delta);
  const bend=sub(originalKnee,originalHip);
  const alongBend=dot(bend,axis);
  const outward=unit(bend.map((v,i)=>v-alongBend*axis[i]));
  const along=(upper*upper-lower*lower+d*d)/(2*d);
  const across=Math.sqrt(Math.max(0,upper*upper-along*along));
  return hip.map((v,i)=>v+along*axis[i]+across*outward[i]);
}

export function gaitPose(rest, phase, mode) {
  const t=((phase%4)+4)%4;
  const walking=["walk","sliding-feet","frozen-body"].includes(mode);
  const stride=walking?.48:0;
  const travel=mode==="frozen-body"?0:phase*stride/4;
  const active=Math.floor(t), progress=t-active;
  const swing=Math.max(0,Math.min(1,(progress-.2)/.6));
  const envelope=progress<.2 ? progress/.2 : progress>.8 ? (1-progress)/.2 : 1;
  const ease=envelope*envelope*(3-2*envelope);
  const stance=rest.filter((_,i)=>i!==active);
  const offset=[0,1,2].map(axis=>axis===1 ? -.95*ease :
    ease*.39*stance.reduce((sum,leg)=>sum+leg[2][axis],0)/3);
  offset[0]+=travel;
  // Keep the nominal links within reach even as an anchored foot trails the hips.
  if(walking) offset[1]-=.45;
  const legs=rest.map((leg,i)=>{
    const hip=leg[0].map((v,axis)=>v+offset[axis]);
    const raised=i===active || (mode==="three-air" && i!==(active+1)%4);
    const lift=raised?Math.sin(Math.PI*swing)**2:0;
    const blend=swing*swing*(3-2*swing);
    const cycle=Math.floor(phase/4);
    const anchor=(cycle+(i<active?1:i===active?blend:0))*stride;
    const foot=[leg[2][0]+(mode==="sliding-feet"?travel:anchor),
      leg[2][1]+.80*lift,leg[2][2]+lift*.28*Math.sin(2*Math.PI*swing)];
    return [hip,kneeFor(leg,hip,foot,mode==="long-leg" && i===0?.48:0),foot];
  });
  return {legs,offset,travel};
}

// Three orthographic coordinate channels packed into opaque raster pixels.
// The probe consumes exactly the same pose used to position the visible links.
export function readGaitPixels(rest, mode, frames=48) {
  const canvas=document.createElement("canvas");
  canvas.width=36; canvas.height=frames;
  const ctx=canvas.getContext("2d",{willReadFrequently:true});
  const data=ctx.createImageData(canvas.width,canvas.height);
  for(let row=0;row<canvas.height;row++) {
    const {legs}=gaitPose(rest,row/12,mode);
    legs.flat(2).forEach((value,col)=>{
      const n=Math.round((value+16)*1024), offset=4*(row*canvas.width+col);
      data.data.set([n>>8,n&255,0,255],offset);
    });
  }
  ctx.putImageData(data,0,0);
  const pixels=ctx.getImageData(0,0,canvas.width,canvas.height).data;
  const bytes=[];
  for(let i=0;i<pixels.length;i+=4) bytes.push(pixels[i],pixels[i+1]);
  return bytes.join(",");
}

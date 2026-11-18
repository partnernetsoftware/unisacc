const WIDTH=426, HEIGHT=240, MAP=256;
const palette=[
  [19,67,128],[30,111,165],[45,158,190],[231,204,134],
  [133,183,71],[89,153,59],[60,115,61],[119,111,121]
];
const muted=[[29,74,112],[43,108,143],[68,148,164],[216,190,133],
  [94,148,77],[66,123,67],[52,95,62],[111,111,114]];
const foam=[174,225,231], haze=[173,214,234];
const islands=[[162,82,44,62],[68,103,32,42],[227,65,32,26],[92,29,40,30],[188,15,58,37]];
const bayer=[0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5];
const clouds=[
  [[93,74,39,18],[202,63,23,14],[155,8,32,15],[55,25,39,18],[231,14,36,17],
    [117,11,26,19],[136,6,36,15],[216,10,31,14],[31,43,38,20]],
  [[115,103,45,24],[142,96,43,25],[78,48,34,20],[212,93,40,19],[190,53,39,13],
    [116,109,53,33],[155,99,54,34],[148,107,48,30]]
];
const puff=[
  "........LLL........",
  ".....LLLWWWWL......",
  "..LLLWWWWWWWWWLL...",
  ".LWWWWWWWWWWWWWWLL.",
  "LLLWWWWWWWWWWWWLLLL",
  "...LLLLLLLLLLLL...."
];

function hash(x,y,seed) {
  let n=(Math.imul(x+17,374761393)^Math.imul(y+29,668265263)^Math.imul(seed,1442695041))>>>0;
  n=Math.imul(n^(n>>>13),1274126177)>>>0;
  return (n^(n>>>16))&255;
}
function noise(x,y,step,seed) {
  const u=Math.floor(x/step),v=Math.floor(y/step),s=x%step,t=y%step;
  const mask=MAP/step-1;
  const a=hash(u,v,seed)*(step-s)+hash((u+1)&mask,v,seed)*s;
  const b=hash(u,(v+1)&mask,seed)*(step-s)+hash((u+1)&mask,(v+1)&mask,seed)*s;
  return Math.floor((a*(step-t)+b*t)/(step*step));
}
function elev(x,y,seed) {
  let peak=-49;
  for(const [cx,cy,rx,ry] of islands) {
    const dx=Math.min(Math.abs(x-cx),MAP-Math.abs(x-cx));
    const dy=Math.min(Math.abs(y-cy),MAP-Math.abs(y-cy));
    peak=Math.max(peak,93-Math.floor(96*dx*dx/(rx*rx))-Math.floor(96*dy*dy/(ry*ry)));
  }
  return peak+Math.floor((noise(x,y,32,seed)-128)*.35)
    +Math.floor((noise(x,y,8,seed+3)-128)*.20)
    +Math.floor((noise(x,y,2,seed+5)-128)*.07);
}
function band(h) {
  if(h<-25) return 0;
  if(h<-8) return 1;
  if(h<0) return 2;
  if(h<8) return 3;
  if(h<28) return 4;
  if(h<55) return 5;
  return h<101?6:7;
}
function mix(a,b,t) {
  return a.map((v,i)=>Math.round(v+(b[i]-v)*t));
}
function rgb(color) {
  return 0xff000000 | (color[2]<<16) | (color[1]<<8) | color[0];
}
function worldDelta(v,base) { return v-base-512*Math.round((v-base)/512); }

export function pixelWorld(context,query) {
  const seed=query.has("wrong-seed")?31:7;
  const colors=query.has("wrong-bands")?palette.map((color,i)=>i===5?palette[7]:color)
    :query.has("muted-palette")?muted:palette;
  const heights=new Int16Array(MAP*MAP);
  const bands=new Uint8Array(MAP*MAP);
  for(let y=0;y<MAP;y++) for(let x=0;x<MAP;x++) {
    const at=y*MAP+x;
    heights[at]=elev(x,y,seed);
    bands[at]=band(heights[at]);
  }
  const fineSize=MAP*2;
  const relief=new Int16Array(fineSize*fineSize);
  for(let y=0;y<fineSize;y++) for(let x=0;x<fineSize;x++) {
    const left=x>>1,top=y>>1,right=(left+(x&1))&255,bottom=(top+(y&1))&255;
    relief[y*fineSize+x]=Math.floor((heights[top*MAP+left]+heights[top*MAP+right]
      +heights[bottom*MAP+left]+heights[bottom*MAP+right])/4);
  }
  const image=context.createImageData(WIDTH,HEIGHT);
  const pixels=new Uint32Array(image.data.buffer);
  const limit=new Int16Array(WIDTH);
  const stages=Array.from({length:17},(_,step)=>
    [...colors,foam,...colors.map(color=>color.map(channel=>Math.round(channel*.87)))]
      .map(color=>rgb(mix(color,haze,step/16*.86))));
  const sky=Array.from({length:49},(_,step)=>rgb(mix([42,126,185],[189,229,239],step/48)));
  function cloudLayer(flight,horizon,forwardX,forwardZ,rightX,rightZ,layer) {
    for(const [gx,gz,height,size] of clouds[layer]) {
      const x=worldDelta(gx*2-256,flight.x),z=worldDelta(gz*2-256,flight.z);
      const depth=x*forwardX+z*forwardZ;
      if(depth<(layer?28:94) || depth>(layer?130:295)) continue;
      const sx=Math.round(WIDTH/2+151*(x*rightX+z*rightZ)/depth);
      const sy=Math.round(horizon+(flight.altitude+13-height)*151/depth);
      const step=Math.max(1,Math.round(size/depth*4));
      for(let row=0;row<puff.length;row++) for(let col=0;col<puff[row].length;col++) {
        const type=puff[row][col];
        if(type===".") continue;
        context.fillStyle=type==="W"?(layer?"#f5f4e8":"#dce9e8"):(layer?"#a7c7d6":"#b7d1d9");
        context.fillRect(sx+(col-9)*step,sy+(row-3)*step,step,step);
      }
    }
  }
  return {
    paint(flight,tick) {
      const rad=flight.heading*Math.PI/180, fx=Math.sin(rad), fz=-Math.cos(rad),rx=-fz,rz=fx;
      const horizon=113+Math.round(flight.pitch*.50), cam=flight.altitude+13;
      for(let y=0;y<HEIGHT;y++) for(let x=0;x<WIDTH;x++) {
        const shade=Math.min(48,48*y/horizon);
        const dither=bayer[((y&3)<<2)|(x&3)]/16;
        pixels[y*WIDTH+x]=sky[Math.min(48,Math.floor(shade+dither))];
      }
      limit.fill(HEIGHT);
      for(let depth=4;depth<=400;depth+=Math.max(.60,depth*.022)) {
        const fade=Math.max(0,Math.min(16,Math.floor((depth-59)/18)));
        const x0=flight.x+depth*fx,z0=flight.z+depth*fz;
        for(let col=0;col<WIDTH;col++) {
          const lateral=(col+.5-WIDTH/2)*depth/151;
          const gx=Math.floor(x0+lateral*rx+256)&511,gy=Math.floor(z0+lateral*rz+256)&511;
          const h=relief[gy*fineSize+gx],id=band(h);
          const top=Math.max(0,Math.floor(horizon+(cam-Math.max(0,h)*.22)*151/depth));
          if(top>=limit[col]) continue;
          const ripples=id<3 && hash(gx,gy,seed+Math.floor(tick/4))>(id===0?250:id===1?245:229);
          const waterline=id===2 && h>-3 && hash(gx,gy,seed+71)>165;
          const shade=id>=4 && (h%10<2 || hash(gx,gy,seed+19)>234);
          const color=stages[fade][ripples || waterline?8:shade?id+9:id];
          for(let y=top;y<limit[col];y++) pixels[y*WIDTH+col]=color;
          limit[col]=top;
        }
      }
      context.putImageData(image,0,0);
      cloudLayer(flight,horizon,fx,fz,rx,rz,0);
      cloudLayer(flight,horizon,fx,fz,rx,rz,1);
    },
    swatches(target,points) {
      points.forEach(([x,y],i)=>{
        target.fillStyle="rgb("+colors[bands[y*MAP+x]].join(",")+")";
        target.fillRect(i,0,1,1);
      });
    }
  };
}

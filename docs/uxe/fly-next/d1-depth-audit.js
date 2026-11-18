const WIDTH=426, HEIGHT=240, MAP=256;
const palette=[
  [19,67,128],[30,111,165],[45,158,190],[231,204,134],
  [133,183,71],[89,153,59],[60,115,61],[119,111,121]
];
const muted=[[29,74,112],[43,108,143],[68,148,164],[216,190,133],
  [94,148,77],[66,123,67],[52,95,62],[111,111,114]];
const foam=[174,225,231], haze=[173,214,234];
const islands=[
  [100,106,34,28,113],[163,96,22,18,97],[130,67,12,13,84],
  [72,56,16,15,100],[187,52,17,13,102],[104,25,16,14,98],
  [150,20,15,13,103],[215,19,19,13,104]
];
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
const shelf=[
  [24,129,54,113,20,27],[54,113,79,104,22,42],[79,104,106,92,22,50],
  [106,92,114,73,14,28],[35,124,37,98,15,27],[63,116,68,133,11,28],
  [89,103,103,129,13,30]
];
function elev(x,y,seed,continuous) {
  let peak=-49;
  for(const [cx,cy,rx,ry,summit] of islands) {
    const dx=Math.min(Math.abs(x-cx),MAP-Math.abs(x-cx));
    const dy=Math.min(Math.abs(y-cy),MAP-Math.abs(y-cy));
    peak=Math.max(peak,summit-Math.floor(116*dx*dx/(rx*rx))-Math.floor(116*dy*dy/(ry*ry)));
  }
  if(continuous) {
    let shelfHeight=-49;
    for(const [ax,ay,bx,by,r,h] of shelf) {
      const dx=bx-ax,dy=by-ay;
      const t=Math.max(0,Math.min(1,((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy)));
      const cross=(x-ax-t*dx)**2+(y-ay-t*dy)**2;
      shelfHeight=Math.max(shelfHeight,h-Math.floor((h+19)*cross/(r*r)));
    }
    const lagoon=((x-59)/10.5)**2+((y-111)/8.2)**2;
    if(lagoon<1) shelfHeight-=Math.floor((1-lagoon)*65);
    peak=Math.max(peak,shelfHeight);
  }
  return peak+Math.floor((noise(x,y,64,seed)-128)*.45)
    +Math.floor((noise(x,y,16,seed+3)-128)*.25)
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

function mainlandCells(heights) {
  const seen=new Uint8Array(heights.length), main=new Uint8Array(heights.length);
  const queue=new Int32Array(heights.length);
  let largest=[];
  for(let cell=0;cell<heights.length;cell++) {
    if(seen[cell] || heights[cell]<0) continue;
    let size=1;
    seen[cell]=1;
    queue[0]=cell;
    for(let at=0;at<size;at++) {
      const p=queue[at], x=p%MAP;
      const neighbors=[x?p-1:-1,x+1<MAP?p+1:-1,p-MAP,p+MAP];
      for(const next of neighbors) {
        if(next<0 || next>=heights.length || seen[next] || heights[next]<0) continue;
        seen[next]=1;
        queue[size++]=next;
      }
    }
    if(size>largest.length) largest=queue.slice(0,size);
  }
  for(const cell of largest) main[cell]=1;
  return main;
}

export function pixelWorld(context,query) {
  const seed=query.has("wrong-seed")?31:7;
  const colors=query.has("wrong-bands")?palette.map((color,i)=>i===5?palette[7]:color)
    :query.has("muted-palette")?muted:palette;
  const heights=new Int16Array(MAP*MAP);
  const bands=new Uint8Array(MAP*MAP);
  const continuous=!query.has("broken-coast");
  for(let y=0;y<MAP;y++) for(let x=0;x<MAP;x++) {
    const at=y*MAP+x;
    heights[at]=elev(x,y,seed,continuous);
    bands[at]=band(heights[at]);
  }
  const samples=query.has("coarse-relief")?2:4;
  const fineSize=MAP*samples;
  const relief=new Int16Array(fineSize*fineSize);
  for(let y=0;y<fineSize;y++) for(let x=0;x<fineSize;x++) {
    const left=Math.floor(x/samples),top=Math.floor(y/samples);
    const right=(left+1)&255,bottom=(top+1)&255;
    const u=(x%samples)/samples,v=(y%samples)/samples;
    relief[y*fineSize+x]=Math.floor(
      (heights[top*MAP+left]*(1-u)+heights[top*MAP+right]*u)*(1-v)
      +(heights[bottom*MAP+left]*(1-u)+heights[bottom*MAP+right]*u)*v);
  }
  const image=context.createImageData(WIDTH,HEIGHT);
  const pixels=new Uint32Array(image.data.buffer);
  const land=query.has("coast-audit")?new Uint8Array(WIDTH*HEIGHT):null;
  const mainland=land?mainlandCells(heights):null;
  const limit=new Int16Array(WIDTH);
  const slope=new Float32Array(WIDTH),skyScale=new Float32Array(WIDTH);
  const stages=Array.from({length:17},(_,step)=>
    [...colors,foam,...colors.map(color=>color.map(channel=>Math.round(channel*.87))),
      [121,155,65],[103,168,81],[52,104,56]]
      .map(color=>rgb(mix(color,haze,step/16*.86))));
  const rockFade=Array.from({length:17},(_,fade)=>Array.from({length:9},(_,tone)=>
    rgb(mix(mix(colors[6],colors[7],tone/8),haze,fade/16*.86))));
  const rockSamples=new Array(9).fill(0);
  const sky=Array.from({length:49},(_,step)=>rgb(mix([42,126,185],[189,229,239],step/48)));
  function cloudLayer(flight,horizon,forwardX,forwardZ,rightX,rightZ,layer) {
    for(const [gx,gz,height,size] of clouds[layer]) {
      const x=worldDelta(gx*2-256,flight.x),z=worldDelta(gz*2-256,flight.z);
      const depth=x*forwardX+z*forwardZ;
      if(depth<(layer?28:94) || depth>(layer?130:295)) continue;
      const sx=Math.round(WIDTH/2+151*(x*rightX+z*rightZ)/depth);
      const sy=Math.round(horizon+(flight.altitude+13-height)*151/depth+(flight.horizonTilt||0)*(sx-213)/213);
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
      const horizon=100+Math.round(flight.pitch*.50), cam=flight.altitude+20;
      const lean=flight.horizonTilt||0,largeWaves=!query.has("tiny-waves");
      const wavePhase=query.has("still-waves")?0:Math.floor(tick/3);
      const groundDetail=!query.has("bare-ground");
      if(land) land.fill(0);
      for(let x=0;x<WIDTH;x++) {
        slope[x]=lean*(x-213)/213;
        skyScale[x]=48/(horizon+slope[x]);
      }
      for(let y=0;y<HEIGHT;y++) for(let x=0;x<WIDTH;x++) {
        const shade=Math.min(48,y*skyScale[x]);
        const dither=bayer[((y&3)<<2)|(x&3)]/16;
        pixels[y*WIDTH+x]=sky[Math.min(48,Math.floor(shade+dither))];
      }
      limit.fill(HEIGHT);
      rockSamples.fill(0);
      for(let depth=4;depth<=400;depth+=Math.max(.60,depth*.022)) {
        const fade=Math.max(0,Math.min(16,Math.floor((depth-52)/13)));
        const x0=flight.x+depth*fx,z0=flight.z+depth*fz;
        for(let col=0;col<WIDTH;col++) {
          const lateral=(col+.5-WIDTH/2)*depth/151;
          const gx=Math.floor((x0+lateral*rx+256)*samples/2)&(fineSize-1);
          const gy=Math.floor((z0+lateral*rz+256)*samples/2)&(fineSize-1);
          const wx=Math.floor((x0+lateral*rx+256)*2)&1023;
          const wy=Math.floor((z0+lateral*rz+256)*2)&1023;
          const h=relief[gy*fineSize+gx],id=band(h);
          const top=Math.max(0,Math.floor(horizon+slope[col]+(cam-Math.max(0,h)*.22)*151/depth));
          if(top>=limit[col]) continue;
          const waveX=(wx+wavePhase)&1023;
          const crest=(wy+((wx>>2)&15)+wavePhase*2)&63;
          const swell=largeWaves && id<3 && crest<4 && wx%83<43 && hash(wx>>5,wy>>6,seed+63)>94;
          const ripples=id<3 && (swell || hash(waveX>>2,wy>>1,seed+2)>(id===0?250:id===1?245:229));
          const waterline=id===2 && h>-3 && hash(wx>>1,wy>>1,seed+71)>165;
          const shade=id>=4 && (h%10<2 || hash(wx,wy,seed+19)>234);
          let material=ripples || waterline?8:shade?id+9:id;
          if(groundDetail) {
            if(id===4 && (wx+2*wy)%13<3 && hash(wx>>5,wy>>5,seed+27)>83) material=17;
            if((id===5 || id===6) && hash(wx>>1,wy>>1,seed+31)>224) material=(wx+wy)%3?18:19;
          }
          const detail=hash(wx>>2,wy>>2,seed+79)-128;
          const tone=Math.max(0,Math.min(8,Math.round((h-84+detail/18)*.25)));
          const softRock=groundDetail && !query.has("hard-rock") && h>=81 && h<=120;
          const color=softRock?rockFade[fade][tone]:stages[fade][material];
          if(softRock) rockSamples[tone]+=limit[col]-top;
          const landLabel=land && h>=0?1+mainland[Math.floor(gy/samples)*MAP+Math.floor(gx/samples)]:0;
          for(let y=top;y<limit[col];y++) {
            pixels[y*WIDTH+col]=color;
            if(land) land[y*WIDTH+col]=landLabel;
          }
          limit[col]=top;
        }
      }
      context.putImageData(image,0,0);
      cloudLayer(flight,horizon,fx,fz,rx,rz,0);
      cloudLayer(flight,horizon,fx,fz,rx,rz,1);
    },
    rockView() { return rockSamples.join(","); },
    coastMap() {
      let cells="";
      for(let y=0;y<MAP;y+=2) for(let x=0;x<MAP;x+=2) cells+=String(Number(heights[y*MAP+x]>=0));
      return cells;
    },
    coastView() {
      let cells="";
      for(let y=110;y<HEIGHT;y+=3) for(let x=0;x<WIDTH;x+=3) cells+=String(land[y*WIDTH+x]);
      return cells;
    },
    reliefSwatches(target,points) {
      points.forEach(([x,y],i)=>{
        const h=128+relief[Math.floor(y*samples/4)*fineSize+Math.floor(x*samples/4)];
        target.fillStyle="rgb("+h+","+h+","+h+")";
        target.fillRect(i,0,1,1);
      });
    },
    swatches(target,points) {
      points.forEach(([x,y],i)=>{
        target.fillStyle="rgb("+colors[bands[y*MAP+x]].join(",")+")";
        target.fillRect(i,0,1,1);
      });
    }
  };
}

const width=426,height=240,horizon=90,reachLength=216;
const canvas=document.querySelector("canvas"),paint=canvas.getContext("2d",{alpha:false});
const image=paint.createImageData(width,height),data=image.data;
const query=new URLSearchParams(location.search);
const reflections=!query.has("no-reflection");
const reaches=[
  ["苇汀",[75,65,82],[243,175,102],[105,87,102],[60,47,68],[61,48,53],[255,217,145]],
  ["花浅",[83,72,113],[222,153,156],[113,101,145],[63,58,91],[65,52,72],[255,210,171]],
  ["紫雾",[47,49,92],[171,151,175],[74,91,130],[41,52,90],[45,43,77],[227,200,190]],
  ["青峡",[32,70,86],[133,183,157],[57,116,119],[26,71,83],[28,63,70],[244,207,130]],
  ["雪湾",[86,111,149],[222,227,207],[123,155,177],[51,86,118],[75,92,112],[253,237,176]],
  ["石灯",[42,52,65],[144,154,137],[70,104,106],[37,63,70],[42,50,58],[249,202,128]],
  ["星滩",[22,28,66],[116,107,146],[61,78,123],[25,43,79],[35,35,63],[199,243,209]],
  ["晨浦",[75,95,122],[240,188,152],[100,134,146],[49,89,110],[60,72,84],[255,231,170]]
];
const boat={x:0,z:0,speed:.35,yaw:0,tick:0};
const keys=new Set(),end=reachLength*reaches.length;
const river=z=>5.4*Math.sin(z/49)+4.3*Math.sin(z/117);
const half=z=>13.2+1.9*Math.sin(z/39);
const clamp=(v,lo,hi)=>Math.max(lo,Math.min(hi,v));
const blend=(a,b,t)=>a.map((n,i)=>Math.round(n+(b[i]-n)*t));
const css=c=>"rgb("+c.join(",")+")";
let pointer=null,shown=-1;

function noise(x,y) {
  let n=(Math.imul(x+71,374761393)^Math.imul(y+97,668265263))>>>0;
  n=Math.imul(n^(n>>>13),1274126177)>>>0;
  return (n^(n>>>16))&255;
}
function step(steer,thrust) {
  const bound=half(boat.z)-.9,center=river(boat.z);
  boat.yaw+=.13*(steer-boat.yaw);
  boat.x+=boat.yaw*(.12+boat.speed*.17)+(center-boat.x)*.011;
  const rubbing=Math.abs(boat.x-center)>bound;
  boat.x=clamp(boat.x,center-bound,center+bound);
  const target=rubbing?.16:thrust>0?1.65:thrust<0?-.48:.35;
  boat.speed+=(target-boat.speed)*(rubbing?.30:.085);
  if(!query.has("frozen-course")) boat.z=clamp(boat.z+boat.speed,0,end);
  boat.tick++;
}
function pixel(color,x,y,w=1,h=1) {
  paint.fillStyle=color;
  paint.fillRect(Math.round(x),Math.round(y),w,h);
}
function polygon(color,points) {
  paint.fillStyle=color;
  paint.beginPath();
  paint.moveTo(points[0][0],points[0][1]);
  for(const [x,y] of points.slice(1)) paint.lineTo(x,y);
  paint.closePath();
  paint.fill();
}
function point(x,z) {
  const depth=z-boat.z;
  return {x:213+(x-boat.x)*155/depth,y:horizon+1550/depth,scale:155/depth};
}
function channels(i,color,delta=0) {
  data[i]=color[0]+delta;
  data[i+1]=color[1]+delta;
  data[i+2]=color[2]+delta;
  data[i+3]=255;
}
function background(theme) {
  const [,high,low,nearWater,deepWater,bank,glow]=theme;
  const light=282-(boat.x-river(boat.z))*1.8;
  for(let y=0;y<height;y++) {
    const drift=y-horizon;
    const d=1550/Math.max(1,drift),z=boat.z+d;
    const center=213+(river(z)-boat.x)*155/d,edge=half(z)*155/d;
    const riverColor=blend(nearWater,deepWater,clamp(drift/150,0,1));
    const sky=blend(high,low,Math.min(1,y/horizon));
    const shore=blend(bank,low,.26*Math.max(0,1-drift/72));
    const glint=blend(riverColor,glow,.73);
    const waterLight=blend(riverColor,glow,.47);
    const wave=Math.floor(Math.sin(y*.58+boat.tick*.16)*(2+drift*.025));
    for(let x=0;x<width;x++) {
      const at=4*(y*width+x);
      if(drift<0) {
        const dx=x-light,dy=y-53,r=dx*dx+dy*dy;
        const grain=noise(x>>1,y>>1);
        const band=r<23*23?glow:r<48*48 && grain>186?blend(sky,glow,.42):sky;
        channels(at,band,(grain&3)-1);
      } else {
        const wet=Math.abs(x-center)<edge,seed=noise((x+wave)>>2,Math.floor(z*.64));
        const reflected=reflections && Math.abs(x-light-wave)<3+drift*.23;
        if(!wet) channels(at,shore,((seed>>4)&7)-3);
        else if(reflected && seed>115) channels(at,seed>221?glint:waterLight,(seed&7)-4);
        else channels(at,riverColor,(seed>240?23:seed<19?-9:0)+(seed&3));
      }
    }
  }
  paint.putImageData(image,0,0);
}
function clouds(theme) {
  const [,high,low]=theme;
  for(const [cx,cy,rx,ry,seed] of [[54,40,62,15,2],[155,59,52,11,3],[356,26,75,17,4],[331,71,60,14,5]]) {
    for(let y=-ry;y<=ry;y+=2) {
      const span=rx*Math.sqrt(1-y*y/(ry*ry));
      for(let x=-span;x<span;x+=3) {
        const roll=noise(Math.floor(x/7)+seed,Math.floor(y/3));
        if(roll<70 || Math.abs(x)>span*(.87+(roll&7)/38)) continue;
        const color=blend(y>1?high:low,low,y>1?.5:.82);
        pixel(css(color),cx+x-boat.x*.4,cy+y,3,2);
      }
    }
  }
}
function ridges(theme,realm) {
  const [, ,fog, , ,bank,glow]=theme;
  for(let layer=0;layer<3;layer++) {
    const color=css(blend(bank,fog,.7-layer*.19));
    for(let x=0;x<width;x+=2) {
      const far=Math.max(0,Math.abs(x-213+boat.x*.8)-15-layer*9)/width;
      const curve=.62+Math.sin(x*.043+layer*2.7+realm)*.27+Math.cos(x*.016-layer)*.18;
      const rise=Math.round(far*(13+layer*15)*curve);
      if(rise>0) pixel(color,x,horizon-rise,2,rise);
    }
  }
  if(realm===6) for(let i=0;i<39;i++) {
    const x=(i*79+13)%426,y=6+noise(i,71)%66;
    pixel(css(blend(theme[1],glow,.43+(i%4)*.11)),x,y,i%9?1:2,1);
  }
}
function banks(theme) {
  const [, ,fog, ,water,ground,light]=theme;
  const first=Math.floor(boat.z/7)*7;
  for(let offset=343;offset>7;offset-=7) for(const side of [-1,1]) {
    const z=first+offset,d=z-boat.z;
    if(d<9) continue;
    const worldX=river(z)+side*(half(z)+1+noise(offset,side)%4);
    const p=point(worldX,z);
    const s=p.scale;
    if(p.x<-60 || p.x>486 || p.y>height+20) continue;
    const amount=clamp((d-22)/310,0,.86);
    const trunk=css(blend(ground,fog,amount)),leaf=css(blend(ground,light,.15+amount*.35));
    for(let reed=0;reed<6;reed++) {
      const n=noise(reed,Math.floor(z)+side);
      const x=p.x+side*(reed*.7*s);
      const len=Math.max(2,(1.6+n/82)*s);
      pixel(trunk,x,p.y-len,Math.max(1,s*.16),len);
      pixel(leaf,x+side,p.y-len-1,Math.max(1,s*.32),Math.max(1,s*.55));
      if(reflections && reed%2===0)
        pixel(css(blend(water,ground,.55)),x+(n%3)-1,p.y+1,1,len*.45);
    }
    if(Math.floor(z/7)%5) continue;
    const base=p.y-3.6*s,spread=3.1*s;
    pixel(trunk,p.x-s*.22,base,Math.max(1,s*.45),p.y-base);
    for(let row=-4;row<=4;row++) {
      const reach=spread*(1-Math.abs(row)*.13);
      pixel(row>0?trunk:leaf,p.x-reach,base+row*s*.58,reach*2,Math.max(1,s*.85));
    }
  }
}
function crossing(theme) {
  if(query.has("no-bridge")) return;
  const first=Math.floor(boat.z/reachLength);
  for(let segment=first+1;segment>=first;segment--) {
    const z=segment*reachLength+36,d=z-boat.z;
    if(d<9 || d>300) continue;
    const [, ,fog, ,water,bank,glow]=theme;
    const p=point(river(z),z), span=half(z)*p.scale;
    const top=p.y-5*p.scale,deck=Math.max(2,p.scale*.85);
    const stone=css(blend(bank,fog,clamp(d/210,0,.7)));
    const edge=css(blend(bank,glow,.28));
    for(let i=Math.floor(-span);i<span;i++) {
      const arch=Math.min(Math.abs(i/span),1);
      const foot=top+deck+p.scale*4*arch*arch;
      pixel(stone,p.x+i,top,1,foot-top);
      if(i%7===0) pixel(edge,p.x+i,top,1,1);
    }
    for(const side of [-1,1]) {
      const x=p.x+side*span*.77, size=Math.max(2,p.scale*.55);
      pixel(stone,x-size,p.y-p.scale*5,size*2,p.scale*5);
      pixel(css(glow),x,top-4,size,size);
      if(reflections) for(let stripe=0;stripe<5;stripe++) {
        const at=p.y+1+stripe*p.scale;
        pixel(css(blend(water,glow,.42-stripe*.05)),x+noise(stripe,boat.tick>>2)%3,at,size,1);
      }
    }
  }
}
function lilies(theme) {
  const [, , , ,water,bank,glow]=theme;
  for(let j=22;j>=0;j--) {
    const z=Math.floor(boat.z/9)*9+13+j*5,d=z-boat.z;
    if(d<13) continue;
    const n=noise(Math.floor(z),41),side=n%2?1:-1;
    const p=point(river(z)+side*(half(z)*(.51+(n%29)/100)),z);
    const size=Math.max(1,p.scale*(.35+(n%13)/20));
    pixel(css(blend(water,bank,.48)),p.x-size,p.y+1,size*2,1);
    pixel(css(blend(bank,[125,137,79],.55)),p.x-size,p.y,size*2,2);
    if(n>163) {
      pixel(css(blend(bank,[211,106,99],.75)),p.x,p.y-3,2,3);
      pixel(css(glow),p.x,p.y-4,1,1);
    }
  }
}
function prow() {
  const x=213+boat.yaw*9,y=188+Math.round(Math.sin(boat.tick*.17));
  polygon("#35252c",[[x,y],[x+27,y+14],[x+68,240],[x-68,240],[x-27,y+14]]);
  polygon("#63444a",[[x,y+4],[x+21,y+16],[x+54,240],[x-54,240],[x-21,y+16]]);
  polygon("#47323b",[[x,y+8],[x+16,y+20],[x+42,240],[x-42,240],[x-16,y+20]]);
  for(let n=0;n<5;n++) {
    const span=10+n*6,at=y+15+n*8;
    pixel(n%2?"#93634f":"#775149",x-span,at,span*2,2);
  }
  if(query.has("no-lamp")) return;
  pixel("#362933",x-2,y-30,3,32);
  pixel("#362933",x-1,y-34,11,3);
  pixel("#362933",x+9,y-32,3,6);
  pixel("#a66243",x+6,y-26,9,11);
  pixel("#ffb568",x+7,y-25,7,8);
  pixel("#fff0af",x+9,y-23,3,5);
  if(reflections) for(let n=0;n<9;n++) {
    const at=y+3+n*3;
    const ripple=noise(n,boat.tick>>1)%5;
    pixel(n%3?"#ac745b":"#e6ac72",x+31-n*.5+ripple,at,4+n%4,1);
  }
}
function draw() {
  const realm=Math.min(reaches.length-1,Math.floor(boat.z/reachLength));
  const phase=clamp((boat.z-realm*reachLength)/18,0,1);
  const theme=realm>0 && phase<1?reaches[realm].map((value,index)=>
    index?blend(reaches[realm-1][index],value,phase*phase*(3-2*phase)):value):reaches[realm];
  background(theme);
  clouds(theme);
  ridges(theme,realm);
  banks(theme);
  lilies(theme);
  crossing(theme);
  prow();
  if(realm!==shown) {
    shown=realm;
    document.querySelector("#reach").textContent=(realm+1)+" / "+reaches.length+"  "+theme[0];
  }
}
function steerInput() {
  const right=keys.has("KeyD")||keys.has("ArrowRight"),left=keys.has("KeyA")||keys.has("ArrowLeft");
  const up=keys.has("KeyW")||keys.has("ArrowUp"),down=keys.has("KeyS")||keys.has("ArrowDown");
  return [Number(right)-Number(left)||(pointer?.steer??0),Number(up)-Number(down)||(pointer?1:0)];
}
function pointInput(event) {
  if(pointer && event.pointerId===pointer.id)
    pointer.steer=clamp((event.clientX/canvas.clientWidth-.5)*3,-1,1);
}

const controls=["KeyW","KeyA","KeyS","KeyD","ArrowUp","ArrowLeft","ArrowDown","ArrowRight"];
addEventListener("keydown",event=>{
  if(!controls.includes(event.code)) return;
  event.preventDefault();
  keys.add(event.code);
});
addEventListener("keyup",event=>keys.delete(event.code));
addEventListener("blur",()=>{ keys.clear(); pointer=null; });
if(query.has("probe")) {
  dispatchEvent(new KeyboardEvent("keydown",{code:"KeyW"}));
  const samples=[];
  const probes=query.get("probe").split(";").map(p=>p.split(",").map(Number));
  let sampledTick=-1;
  for(const [tick,x,y] of probes) {
    while(boat.tick<tick) step(...steerInput());
    if(sampledTick!==boat.tick) {
      draw();
      sampledTick=boat.tick;
    }
    samples.push(...paint.getImageData(x,y,1,1).data.subarray(0,3));
  }
  const out=document.querySelector("output");
  out.id="pixels";
  out.textContent=samples.join(",");
} else if(query.has("still")) {
  const reach=clamp(Number(query.get("still")) || 1,1,reaches.length);
  const frames=reach>1?Math.ceil(((reach-1)*reachLength+28)/1.65)+16:0;
  for(let i=0;i<frames;i++) step(0,1);
  draw();
} else {
  canvas.addEventListener("pointerdown",event=>{
    if(pointer) return;
    pointer={id:event.pointerId,steer:0};
    canvas.setPointerCapture(event.pointerId);
    pointInput(event);
  });
  canvas.addEventListener("pointermove",pointInput);
  canvas.addEventListener("lostpointercapture",()=>pointer=null);
  let previous=performance.now();
  function animate(now) {
    if(now-previous>=32) {
      previous=now;
      step(...steerInput());
      draw();
    }
    requestAnimationFrame(animate);
  }
  draw();
  requestAnimationFrame(animate);
}

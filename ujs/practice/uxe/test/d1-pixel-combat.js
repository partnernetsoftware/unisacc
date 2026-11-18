import { FONT } from "../font5x7.js";

const course=[
  { x:-49,z:-116,y:55,vx:1,kind:0 },
  { x:4,z:-91,y:45,vx:0,kind:1 },
  { x:53,z:-135,y:50,vx:-1,kind:2 }
];
const marker="#ff746a", lock="#ffe46b";
const targets=[
  ["...W.......W...","...OW..B..WO...","..KOKKMBMKKOK..","KKKDKMMMMMKDKKK","...KKO.K.OKK..."],
  ["....W.....W....","...OW..B..WO...","..KOKKMBMKKOK..","KKKDKMMMMMKDKKK","...KKO.K.OKK..."],
  [".....W...W.....","....OW.B.WO....","..KOKKMBMKKOK..","KKKDKMMMMMKDKKK","...KKO.K.OKK..."]
];
const paint={W:"#bba5a0",O:"#c15748",K:"#2b2030",D:"#503347",M:"#8b5263",B:"#84c6bd"};
function point(context,color,x,y,w=1,h=1) {
  context.fillStyle=color;
  context.fillRect(Math.round(x),Math.round(y),w,h);
}
function label(context,text,x,y,color) {
  for(const letter of text) {
    const columns=FONT[letter]||[];
    for(let i=0;i<columns.length;i++) for(let j=0;j<7;j++)
      if(columns[i]&(1<<j)) point(context,color,x+i,y+j);
    x+=6;
  }
}
function wrap(delta) { return delta-512*Math.round(delta/512); }
function screen(flight,enemy,tick) {
  const angle=flight.heading*Math.PI/180,s=Math.sin(angle),c=Math.cos(angle);
  const x=wrap(enemy.x+enemy.vx*Math.trunc((tick%72)/6)-flight.x);
  const z=wrap(enemy.z-Math.trunc(tick/3)-flight.z);
  const depth=x*s-z*c;
  if(depth<9 || depth>345) return null;
  return {
    x:Math.round(213+151*(x*c+z*s)/depth),
    y:Math.round(113+Math.round(flight.pitch*.5)+(flight.altitude+13-enemy.y)*151/depth),
    depth
  };
}
function enemySprite(context,enemy,pose) {
  const size=pose.depth<57?2:1;
  const sprite=targets[enemy.kind];
  sprite.forEach((row,j)=>{
    [...row].forEach((glyph,i)=>{
      if(paint[glyph]) point(context,paint[glyph],pose.x+(i-7)*size,pose.y+(j-2)*size,size,size);
    });
  });
  for(let k=-3;k<=3;k++) point(context,marker,pose.x+k,pose.y-11+3-Math.abs(k));
}
function brackets(context,x,y) {
  for(const dx of [-1,1]) for(const dy of [-1,1]) {
    const cx=x+11*dx,cy=y+9*dy;
    point(context,lock,cx+Math.min(0,-dx*4),cy,5,1);
    point(context,lock,cx,cy+Math.min(0,-dy*3),1,4);
  }
}

export function combatModel(context,query) {
  const enemies=course.map(slot=>({...slot,hp:4,respawn:0}));
  const bursts=[], rounds=[];
  let ammo=96, missiles=3, cannonReady=0, missileReady=0, lastFire=0;
  function damage(enemy,power,tick,pose) {
    if(enemy.hp<=0) return;
    enemy.hp-=power;
    if(enemy.hp<=0) {
      enemy.respawn=tick+110;
      bursts.push({tick,x:pose.x,y:pose.y,kind:"burst"});
    }
  }
  function update(flight,tick,gun,missile) {
    for(const enemy of enemies) {
      if(enemy.hp<=0 && tick>=enemy.respawn) enemy.hp=4;
      const view=query.has("freeze-targets")?{...flight,x:0,z:-1.5}:flight;
      enemy.pose=screen(view,enemy,query.has("freeze-targets")?1:tick);
    }
    const visible=enemies.filter(enemy=>enemy.hp>0 && enemy.pose &&
      enemy.pose.x>=12 && enemy.pose.x<=413 && enemy.pose.y>=16 && enemy.pose.y<196);
    const aimY=101+Math.round(flight.pitch*.25);
    const offset=e=>(e.pose.x-213)**2+(e.pose.y-aimY)**2;
    const selected=visible.reduce((a,b)=>!a || offset(b)<offset(a)?b:a,null);
    const acquired=selected && offset(selected)<30*30;
    if(gun && ammo && tick>=cannonReady) {
      ammo--;
      lastFire=tick;
      cannonReady=tick+5;
      if(acquired) {
        bursts.push({tick,x:selected.pose.x,y:selected.pose.y,kind:"tracer"});
        damage(selected,1,tick,selected.pose);
      } else bursts.push({tick,x:213,y:aimY,kind:"tracer"});
    }
    if(missile && missiles && acquired && tick>=missileReady) {
      missiles--;
      lastFire=tick;
      missileReady=tick+18;
      rounds.push({start:tick,target:selected});
    }
    for(let i=rounds.length-1;i>=0;i--) {
      const shot=rounds[i];
      if(tick-shot.start<13) continue;
      if(shot.target.pose) damage(shot.target,4,tick,shot.target.pose);
      rounds.splice(i,1);
    }
    if(tick-lastFire>175) {
      if(!ammo) ammo=96;
      if(!missiles) missiles=3;
    }
    for(const enemy of visible) if(enemy.hp>0) enemySprite(context,enemy,enemy.pose);
    if(acquired && selected.hp>0 && !query.has("no-lock")) brackets(context,selected.pose.x,selected.pose.y);
    for(const shot of rounds) if(shot.target.pose) {
      const t=(tick-shot.start)/13;
      const x=213+(shot.target.pose.x-213)*t,y=166+(shot.target.pose.y-166)*t;
      point(context,"#f7f1cf",x,y,2,3);
      point(context,"#a9c3c9",x,y+4,1,3);
    }
    for(let i=bursts.length-1;i>=0;i--) {
      const event=bursts[i],age=tick-event.tick;
      if(age>(event.kind==="tracer"?4:30)) { bursts.splice(i,1); continue; }
      if(event.kind==="tracer") {
        for(const lane of [-1,1]) {
          const f=.2+age*.18;
          point(context,"#ffeaa3",213+lane*28+(event.x-213-lane*28)*f,162+(event.y-162)*f,1,2);
        }
      } else {
        for(let j=0;j<22;j++) {
          const x=event.x+(j%7-3)*(1+age*.23),y=event.y+(j%5-2)*(1+age*.20)-age*.20;
          point(context,age<9?(j%3===0?"#ffeb86":"#f7793d"):(j%3===0?"#6e6878":"#9790a0"),x,y,2+(j%3),2);
        }
      }
    }
    for(const wing of [-1,1]) for(let i=0;i<18;i++) {
      const life=(i+tick)%18;
      point(context,life<7?"#e7f1e8":"#94bec5",
        213+wing*(38+life*.68)-flight.bank*life*.044,163+life,life%4===0?2:1);
    }
  }
  function hud() {
    point(context,"#23344d",8,219,112,14);
    label(context,"AMMO "+String(ammo).padStart(2,"0"),12,223,"#e8eadb");
    label(context,"MSL "+missiles,74,223,"#e8eadb");
  }
  return {update,hud};
}

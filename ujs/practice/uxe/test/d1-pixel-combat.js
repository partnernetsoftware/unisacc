const course=[
  { x:-49,z:-116,y:55,vx:1,kind:0 },
  { x:4,z:-91,y:45,vx:0,kind:1 },
  { x:53,z:-135,y:50,vx:-1,kind:2 }
];
const marker="#ff746a", lock="#ffe46b";
const targets=[
  ["...OW.......WO...","...OW..BBB..WO...","..KOOKMBBBMKOOK..","KKODOOMMMMMOODOKK","KKKDKMMMMMMMDKKKK","...KKO.KKK.OKK..."],
  ["....OW.....WO....","...OW..BBB..WO...","..KOOKMBBBMKOOK..","KKODOOMMMMMOODOKK","KKKDKMMMMMMMDKKKK","...KKO.KKK.OKK..."],
  [".....OW...WO.....","....OW.BBB.WO....","..KOOKMBBBMKOOK..","KKODOOMMMMMOODOKK","KKKDKMMMMMMMDKKKK","...KKO.KKK.OKK..."]
];
const paint={W:"#f3b450",O:"#e47731",K:"#2b2030",D:"#70422d",M:"#a65b35",B:"#79c5bd"};
const small={
  A:["010","101","111","101","101"], M:["101","111","111","101","101"],
  O:["111","101","101","101","111"], S:["111","100","111","001","111"],
  L:["100","100","100","100","111"], "0":["111","101","101","101","111"],
  "1":["010","110","010","010","111"],"2":["111","001","111","100","111"],
  "3":["111","001","111","001","111"],"4":["101","101","111","001","001"],
  "5":["111","100","111","001","111"],"6":["011","100","111","101","111"],
  "7":["111","001","010","010","010"],"8":["111","101","111","101","111"],
  "9":["111","101","111","001","110"]
};
function point(context,color,x,y,w=1,h=1) {
  context.fillStyle=color;
  context.fillRect(Math.round(x),Math.round(y),w,h);
}
function label(context,text,x,y,color) {
  for(const letter of text) {
    const rows=small[letter]||[];
    for(let j=0;j<rows.length;j++) for(let i=0;i<3;i++)
      if(rows[j][i]==="1") point(context,color,x+i,y+j);
    x+=4;
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
function enemySprite(context,enemy,pose,fall=0) {
  const size=pose.depth<57?2:1;
  const sprite=targets[enemy.kind];
  sprite.forEach((row,j)=>{
    [...row].forEach((glyph,i)=>{
      if(paint[glyph]) point(context,paint[glyph],pose.x+(i-8)*size,
        pose.y+(j-2)*size+(i-8)*fall,size,size);
    });
  });
  if(!fall) for(let k=-3;k<=3;k++) point(context,marker,pose.x+k,pose.y-11+3-Math.abs(k));
}
function smoke(context,x,y,age,life) {
  const radius=1+Math.floor(age/3);
  const color=age<life/2?"#9ba2af":"#676778";
  point(context,color,x-radius,y,2*radius+1,2);
  point(context,color,x-radius+1,y-1,2*radius-1,4);
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
  let ammo=96, missiles=3, cannonReady=0, missileReady=0, lastFire=0, locked=false;
  function damage(enemy,power,tick,pose) {
    if(enemy.hp<=0) return;
    enemy.hp-=power;
    bursts.unshift({tick,x:pose.x,y:pose.y,kind:"smoke"});
    if(enemy.hp<=0) {
      enemy.fall=tick;
      enemy.respawn=tick+110;
      bursts.unshift({tick,x:pose.x,y:pose.y,kind:"burst"});
    }
  }
  function update(flight,tick,gun,missile,attitude) {
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
    if(!query.has("no-fall")) for(const enemy of enemies) {
      const age=tick-enemy.fall;
      if(enemy.hp>0 || !enemy.pose || age>=29) continue;
      const x=enemy.pose.x+age*.48, y=enemy.pose.y+age*age*.12;
      enemySprite(context,enemy,{...enemy.pose,x,y},.15+age*.025);
    }
    if(acquired && selected.hp>0 && !query.has("no-lock")) brackets(context,selected.pose.x,selected.pose.y);
    locked=Boolean(acquired && selected.hp>0);
    for(const shot of rounds) if(shot.target.pose) {
      const t=(tick-shot.start)/13;
      const x=213+(shot.target.pose.x-213)*t,y=166+(shot.target.pose.y-166)*t;
      point(context,"#f7f1cf",x,y,2,3);
      point(context,"#a9c3c9",x,y+4,1,3);
      if(!query.has("no-plume")) bursts.push({tick,x,y:y+6,kind:"plume"});
    }
    for(let i=bursts.length-1;i>=0;i--) {
      const event=bursts[i],age=tick-event.tick;
      const life={tracer:4,burst:30,smoke:18,plume:11}[event.kind];
      if(age>life) { bursts.splice(i,1); continue; }
      if(event.kind==="tracer") {
        for(const lane of [-1,1]) {
          const f=.2+age*.18;
          point(context,"#ffeaa3",213+lane*28+(event.x-213-lane*28)*f,162+(event.y-162)*f,1,2);
        }
      } else if(event.kind==="burst") {
        for(let j=0;j<22;j++) {
          const x=event.x+(j%7-3)*(1+age*.23),y=event.y+(j%5-2)*(1+age*.20)-age*.20;
          point(context,age<9?(j%3===0?"#ffeb86":"#f7793d"):(j%3===0?"#6e6878":"#9790a0"),x,y,2+(j%3),2);
        }
      } else if(event.kind==="plume" || !query.has("no-hit-smoke"))
        smoke(context,event.x+age*.19,event.y-age*.43,age,life);
    }
    const sine=Math.sin(attitude.roll),cosine=Math.cos(attitude.roll);
    for(const wing of [-1,1]) for(let i=0;i<18;i++) {
      const life=(i+tick)%18;
      const x=wing*38,y=5;
      const drift=query.has("straight-trail")?0:flight.bank*life*life*.004;
      point(context,life<7?"#e7f1e8":"#94bec5",
        213+x*cosine-y*sine+wing*life*.68-drift,
        attitude.y+x*sine+y*cosine+life,life%4===0?2:1);
    }
  }
  function hud() {
    point(context,"#23344d",8,224,65,11);
    label(context,"AMMO "+String(ammo).padStart(2,"0"),11,227,"#e8eadb");
    label(context,"MSL "+missiles,50,227,"#e8eadb");
    return {ammo,missiles,locked};
  }
  return {update,hud};
}

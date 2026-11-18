const course=[
  { x:-49,z:-116,y:55,vx:1,kind:0,hull:4,sweep:.65,fire:20 },
  { x:4,z:-91,y:45,vx:0,kind:1,hull:2,sweep:-1.35,fire:0 },
  { x:53,z:-135,y:50,vx:-1,kind:2,hull:8,sweep:.28,fire:0 }
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
  L:["100","100","100","100","111"], H:["101","101","111","101","101"],
  P:["110","101","110","100","100"], K:["101","101","110","101","101"],
  W:["101","101","111","111","101"], V:["101","101","101","101","010"], E:["111","100","110","100","111"],
  "0":["111","101","101","101","111"],
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
function screen(flight,enemy,tick,maneuver) {
  const angle=flight.heading*Math.PI/180,s=Math.sin(angle),c=Math.cos(angle);
  const phase=Math.max(0,tick-30)%96;
  const sweep=maneuver?(phase<24?phase:phase<72?48-phase:phase-96):0;
  const x=wrap(enemy.x+enemy.vx*Math.trunc((tick%72)/6)+sweep*enemy.sweep-flight.x);
  const z=wrap(enemy.z-Math.trunc(tick/3)-flight.z);
  const depth=x*s-z*c;
  if(depth<9 || depth>345) return null;
  return {
    x:Math.round(213+151*(x*c+z*s)/depth),
    y:Math.round(113+Math.round(flight.pitch*.5)+(flight.altitude+13-enemy.y+sweep*.18)*151/depth),
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
  if(enemy.kind===2) {
    for(const side of [-1,1]) {
      point(context,"#523b72",pose.x+side*5*size-size,pose.y+2*size,3*size,size);
      point(context,"#9e84ad",pose.x+side*5*size-size,pose.y+3*size,3*size,size);
    }
  }
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

export function combatModel(context,query,event) {
  const bursts=[], rounds=[], incoming=[];
  const counterfire=!query.has("no-return-fire"), maneuver=!query.has("no-maneuver");
  let ammo=96, missiles=3, cannonReady=0, missileReady=0, lastFire=0, locked=false;
  let health=100,kills=0,hitUntil=0;
  let wave=1,waveStart=0,remaining,nextWave=0,enemies;
  function formation(flight) {
    const angle=flight.heading*Math.PI/180,c=Math.cos(angle),s=Math.sin(angle);
    remaining=Math.min(8,wave+2);
    return Array.from({length:remaining},(_,index)=>{
      const slot=course[index%course.length],group=Math.floor(index/course.length);
      const x=slot.x+group*30,z=slot.z-group*41;
      const hull=query.has("light-as-heavy") && slot.kind===1?8:
        query.has("weak-heavy") && slot.kind===2?4:slot.hull;
      return {...slot,x:flight.x+x*c-z*s,z:flight.z+x*s+z*c,y:slot.y+flight.altitude-25+group*3,
        hp:hull+Math.min(4,wave-1),fireReady:waveStart+40+group*9};
    });
  }
  enemies=formation({heading:0,x:0,z:0,altitude:25});
  function damage(enemy,power,tick,pose) {
    if(enemy.hp<=0) return;
    enemy.hp-=power;
    bursts.unshift({tick,x:pose.x,y:pose.y,kind:"smoke"});
    if(enemy.hp<=0) {
      kills++;
      remaining--;
      event("explosion");
      enemy.fall=tick;
      bursts.unshift({tick,x:pose.x,y:pose.y,kind:"burst"});
      if(!remaining) nextWave=tick+36;
    }
  }
  function returnFire(visible,flight,tick,attitude) {
    if(counterfire) for(const enemy of visible) {
      if(enemy.hp<=0 || !enemy.fire || tick<enemy.fireReady) continue;
      enemy.fireReady=tick+Math.max(14,enemy.fire-wave+1);
      incoming.push({start:tick,x:enemy.pose.x,y:enemy.pose.y,heading:flight.heading,pitch:flight.pitch});
    }
    for(let i=incoming.length-1;i>=0;i--) {
      const shot=incoming[i],age=tick-shot.start;
      const headingDelta=Math.abs(flight.heading-shot.heading);
      const shift=2*Math.min(headingDelta,360-headingDelta)+Math.abs(flight.pitch-shot.pitch);
      if(age>=22) {
        if(shift<=18) {
          health=Math.max(0,health-20);
          hitUntil=tick+6;
        }
        incoming.splice(i,1);
        continue;
      }
      const t=age/22;
      const x=shot.x+(213+shift-shot.x)*t, y=shot.y+(attitude.y-shot.y)*t;
      point(context,"#e2529f",x,y,age>11?3:2,3);
      point(context,"#ffd1de",x,y+2,1,2);
    }
    if(tick<hitUntil) {
      point(context,"#e2529f",0,0,426,2);
      point(context,"#e2529f",0,238,426,2);
    }
  }
  function update(flight,tick,gun,missile,attitude) {
    if(nextWave && tick>=nextWave && !query.has("no-wave")) {
      wave++;
      waveStart=tick;
      nextWave=0;
      enemies=formation(flight);
      rounds.length=incoming.length=0;
      ammo=96;
      missiles=3;
      locked=false;
    }
    const frozen=query.has("freeze-targets");
    const view=frozen || query.has("hold-course")?{...flight,x:0,z:-1.5}:flight;
    for(const enemy of enemies) {
      enemy.pose=screen(view,enemy,frozen?1:tick-waveStart,maneuver);
    }
    const visible=enemies.filter(enemy=>enemy.hp>0 && enemy.pose &&
      enemy.pose.x>=12 && enemy.pose.x<=413 && enemy.pose.y>=16 && enemy.pose.y<196);
    const aimY=101+Math.round(flight.pitch*.25);
    const offset=e=>(e.pose.x-213)**2+(e.pose.y-aimY)**2;
    const selected=visible.reduce((a,b)=>!a || offset(b)<offset(a)?b:a,null);
    const acquired=selected && offset(selected)<30*30;
    if(gun && ammo && tick>=cannonReady) {
      ammo--;
      event("gun");
      lastFire=tick;
      cannonReady=tick+5;
      if(acquired) {
        bursts.push({tick,x:selected.pose.x,y:selected.pose.y,kind:"tracer"});
        damage(selected,1,tick,selected.pose);
      } else bursts.push({tick,x:213,y:aimY,kind:"tracer"});
    }
    if(missile && missiles && acquired && tick>=missileReady) {
      missiles--;
      event("missile");
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
    returnFire(visible,flight,tick,attitude);
    if(!query.has("no-fall")) for(const enemy of enemies) {
      const age=tick-enemy.fall;
      if(enemy.hp>0 || !enemy.pose || age>=29) continue;
      const x=enemy.pose.x+age*.48, y=enemy.pose.y+age*age*.12;
      enemySprite(context,enemy,{...enemy.pose,x,y},.15+age*.025);
    }
    if(acquired && selected.hp>0 && !query.has("no-lock")) brackets(context,selected.pose.x,selected.pose.y);
    const hasLock=Boolean(acquired && selected.hp>0);
    if(hasLock && !locked) event("lock");
    locked=hasLock;
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
    point(context,"#23344d",8,213,48,10);
    label(context,"WAVE "+wave,11,216,"#e8eadb");
    point(context,"#23344d",8,224,65,11);
    label(context,"AMMO "+String(ammo).padStart(2,"0"),11,227,"#e8eadb");
    label(context,"MSL "+missiles,50,227,"#e8eadb");
    point(context,"#23344d",339,224,79,11);
    label(context,"HP",342,227,"#e8eadb");
    point(context,"#3e4b5c",352,227,32,5);
    point(context,health<=40?"#ff746a":"#79dca2",352,227,Math.floor(32*health/100),5);
    label(context,"KO "+String(kills%100).padStart(2,"0"),392,227,"#e8eadb");
    return {ammo,missiles,locked,lost:health===0,kills};
  }
  return {update,hud};
}

async function loadTown(name) {
  const response=await fetch(new URL(name,import.meta.url));
  if(!response.ok) throw new Error("城镇规格读取失败");
  return (await response.text()).trim().split("\n").slice(1).map(line=>{
    const [kind,...columns]=line.split("\t");
    const values=columns.map(value=>Number(value)/1000);
    if(values.length!==6 || !values.every(Number.isFinite)) throw new Error("城镇规格格式错误");
    return [kind,...values];
  });
}

export async function townLayout(dense=false) {
  const old=await loadTown("d4-town.tsv");
  if(!dense) return old;
  return [...old.filter(row=>row[0]!=="house"),...await loadTown("d4-dense-town.tsv")];
}

// Each unit contributes a silhouette at a different elevation and depth.
export function buildTown(layout,{block,add,round},mode,details=true) {
  const reduced=mode==="small-town";
  const factor=reduced?.60:1, deck=6.37;
  const tile=(color,x,y,z,w,h,d,roll=0)=>{
    block(color,x*factor,deck+(y-deck)*factor,z*factor,w*factor,h*factor,d*factor,roll);
  };
  const ball=(color,x,y,z,r)=>add(color,round,x*factor,deck+(y-deck)*factor,z*factor,r*factor);

  const count={houses:0,windows:0,eaves:0,chimneys:0,arcades:0,gears:0,pipes:0};
  let stack=0,building=0;
  for(const [kind,x,y,z,w,h,d] of layout) {
    if(kind.startsWith("house-")) {
      const front=z+d/2,roof=y+h, number=building++;
      tile(number%5===0?"arcade":"white",x,y+h/2,z,w,h,d);
      count.houses++;
      if(kind==="house-flat") {
        tile("wood",x,roof+.12,z,w+.14,.24,d+.13);
        tile("orange",x,roof+.28,z,w+.07,.12,d+.09);
      } else {
        for(const side of [-1,1]) tile("orange",x+side*w*.27,roof+.13,z,w*.65,.10,d+.18,-side*.48);
      }
      if(details) {
        tile("wood",x,roof-.04,front,w+.08,.075,.105);
        count.eaves++;
        for(let floor=y+.47;floor<roof-.23;floor+=.54) for(const side of [-1,1]) {
          tile("wood",x+side*w*.24,floor,front+.015,.18,.24,.075);
          tile("glass",x+side*w*.24,floor,front+.058,.12,.155,.035);
          tile("white",x+side*w*.24,floor-.13,front+.064,.20,.035,.065);
          count.windows++;
        }
        for(let floor=y+.63;floor<roof-.23;floor+=.65) {
          tile("glass",x+w/2+.021,floor,z,.07,.20,.16);
          count.windows++;
        }
        if(kind==="house-arch") {
          tile("alcove",x,y+.245,front+.055,.29,.48,.075);
          tile("arcade",x,y+.51,front+.071,.34,.09,.09);
          for(const side of [-1,1]) {
            tile("arcade",x+side*.193,y+.21,front+.07,.072,.42,.10);
            tile("arcade",x+side*.15,y+.446,front+.07,.13,.09,.10,side*.51);
          }
          count.arcades++;
        }
        if(number%3!==1) {
          const chimney=roof+(number%2?.26:.39);
          tile("red",x+w*.25,chimney,z-d*.17,.16,.42,.18);
          tile("wood",x+w*.25,chimney+.22,z-d*.17,.22,.08,.24);
          count.chimneys++;
        }
      }
    } else if(kind==="terrace") {
      tile("wood",x,y,z,w,h,d);
      tile("chassis",x,y+h*.4,z+d*.49,w+.02,.13,.09);
    } else if(kind==="pipe" && details) {
      tile("pipe",x,y,z,w,.086,d);
      tile("pipe",x+w*.43,y+h*.5,z,.086,h,d);
      ball("gearGold",x+w*.43,y,z,.077);
      ball("pipe",x-w*.42,y,z,.074);
      count.pipes++;
    } else if(kind==="house" || kind==="clock") {
      if(kind==="house") count.houses++;
      tile("white",x,y+h/2,z,w,h,d);
      for(const side of [-1,1]) tile("orange",x+side*w*.28,y+h+.25,z,w*.68,.17,d+.52,-side*.51);
      const front=z+d/2;
      for(let windowY=y+.63;windowY<y+h-.38;windowY+=.74) {
        for(const offset of [-.24,.24]) tile("glass",x+w*offset,windowY,front+.02,.23,.31,.085);
      }
      if(kind==="clock") {
        const dial=y+h-.92;
        tile("clockFace",x,dial,front+.095,.88,.88,.14);
        tile("wood",x+.10,dial+.04,front+.178,.25,.055,.025,.1);
        tile("wood",x-.06,dial+.09,front+.178,.32,.052,.025,2.19);
      }
    } else if(kind==="gear" && mode!=="no-gear" && (details || w>1)) {
      const radius=w/2;
      const scale=w/2.06;
      for(let i=0;i<16;i++) {
        const angle=2*Math.PI*i/16;
        tile("gearGold",x+Math.cos(angle)*radius*.81,y+Math.sin(angle)*radius*.81,z,
          .40*scale,.30*scale,d,angle);
      }
      for(let i=0;i<3;i++) tile("gearGold",x,y,z,1.52*scale,.19*scale,d*.85,Math.PI*i/3);
      ball("gearGold",x,y,z+.15*scale,.30*scale);
      count.gears++;
    } else if(kind==="crane") {
      const mast=x-w*.30, end=x+w*.42;
      tile("lift",mast,y-h*.33,z,.20,h*1.44,d);
      tile("lift",x,y,z,w,.17,d);
      tile("lift",x-.27,y-.42,z,1.7,.11,d*.8,.64);
      tile("wood",end,y-.62,z,.055,1.17,.059);
      tile("lift",end,y-1.14,z,.27,.25,.27);
    } else if(kind==="smoke") {
      if (mode==="single-smoke" && stack>0) continue;
      tile("wood",x-1.78,y-3.10,z+.45,.51,2.2,.51);
      for(let i=0;i<6;i++) {
        const t=i/5;
        for(let layer=0;layer<(mode==="single-smoke"?1:6);layer++) {
          const angle=2.37*layer+i+stack*.61;
          const spread=mode==="single-smoke"?.06:(.27+.63*t);
          ball("smoke",x-1.80+t*w+Math.sin(angle)*spread,
            y-h*(1-t)+Math.cos(angle)*spread*.76,z+.5-.2*t+Math.sin(angle*2)*d*.38,
            .23+t*(mode==="single-smoke"?.24:.72)+(layer%2)*.12);
        }
      }
      stack++;
    }
  }
  return count;
}

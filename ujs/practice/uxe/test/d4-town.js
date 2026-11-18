export async function townLayout() {
  const response=await fetch(new URL("d4-town.tsv",import.meta.url));
  if(!response.ok) throw new Error("城镇规格读取失败");
  return (await response.text()).trim().split("\n").slice(1).map(line=>{
    const [kind,...columns]=line.split("\t");
    const values=columns.map(value=>Number(value)/1000);
    if(values.length!==6 || !values.every(Number.isFinite)) throw new Error("城镇规格格式错误");
    return [kind,...values];
  });
}

// Each unit contributes a silhouette at a different elevation and depth.
export function buildTown(layout,{block,add,round},mode) {
  const reduced=mode==="small-town";
  const factor=reduced?.60:1, deck=6.37;
  const tile=(color,x,y,z,w,h,d,roll=0)=>{
    block(color,x*factor,deck+(y-deck)*factor,z*factor,w*factor,h*factor,d*factor,roll);
  };
  const ball=(color,x,y,z,r)=>add(color,round,x*factor,deck+(y-deck)*factor,z*factor,r*factor);

  for(const [kind,x,y,z,w,h,d] of layout) {
    if(kind==="house" || kind==="clock") {
      tile("white",x,y+h/2,z,w,h,d);
      for(const side of [-1,1]) tile("orange",x+side*w*.28,y+h+.22,z,w*.63,.16,d+.35,-side*.51);
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
    } else if(kind==="gear" && mode!=="no-gear") {
      const radius=w/2;
      for(let i=0;i<16;i++) {
        const angle=2*Math.PI*i/16;
        tile("gearGold",x+Math.cos(angle)*radius*.81,y+Math.sin(angle)*radius*.81,z,
          .40,.30,d,angle);
      }
      for(let i=0;i<3;i++) tile("gearGold",x,y,z,1.52,.19,d*.85,Math.PI*i/3);
      ball("gearGold",x,y,z+.15,.30);
    } else if(kind==="crane") {
      const mast=x-w*.30, end=x+w*.42;
      tile("lift",mast,y-h*.33,z,.20,h*1.44,d);
      tile("lift",x,y,z,w,.17,d);
      tile("lift",x-.27,y-.42,z,1.7,.11,d*.8,.64);
      tile("wood",end,y-.62,z,.055,1.17,.059);
      tile("lift",end,y-1.14,z,.27,.25,.27);
    } else if(kind==="smoke") {
      tile("wood",x-1.78,y-3.10,z+.45,.51,2.2,.51);
      for(let i=0;i<6;i++) {
        const t=i/5;
        for(let layer=0;layer<5;layer++) {
          const angle=2.37*layer+i;
          ball("smoke",x-1.80+t*w+Math.sin(angle)*(.12+.46*t),
            y-h*(1-t)+Math.cos(angle)*(.11+.49*t),z+.5-.2*t+Math.sin(angle*2)*d*.25,
            .21+t*.53+(layer%2)*.075);
        }
      }
    }
  }
}

export async function loadScaleStudy(scene) {
  const response=await fetch(new URL("uxe-u2b.tsv",import.meta.url));
  if(!response.ok) throw new Error("构图尺寸表读取失败");
  const row=(await response.text()).trim().split("\n").slice(1)
    .map(line=>line.split("\t")).find(fields=>fields[0]===scene);
  const numbers=row?.slice(1).map(Number);
  if(numbers?.length!==7 || !numbers.every(Number.isSafeInteger)) throw new Error("构图尺寸表格式错误");
  return numbers;
}

// Project the corners of an object-space bounding box to the viewport.
export function screenBox(camera, lo, hi) {
  const [ex,ey,ez]=camera.eye, [tx,ty,tz]=camera.target;
  const length=Math.hypot(tx-ex,ty-ey,tz-ez);
  const fx=(tx-ex)/length, fy=(ty-ey)/length, fz=(tz-ez)/length;
  const horizontal=Math.hypot(fx,fz), rx=-fz/horizontal, rz=fx/horizontal;
  const focal=360/Math.tan(camera.fovy/2), xs=[],ys=[];
  for(const x of [lo[0],hi[0]]) for(const y of [lo[1],hi[1]]) for(const z of [lo[2],hi[2]]) {
    const dx=x-ex,dy=y-ey,dz=z-ez, depth=dx*fx+dy*fy+dz*fz;
    if(depth<=0) throw new Error("构图边界超出相机前方");
    xs.push(640+focal*(dx*rx+dz*rz)/depth);
    ys.push(360-focal*(-dx*rz*fy+dy*horizontal+dz*rx*fy)/depth);
  }
  const extent=(axis,size)=>Math.max(0,Math.min(size,Math.max(...axis))-Math.max(0,Math.min(...axis)));
  return [Math.round(1000*extent(xs,1280)/1280),Math.round(1000*extent(ys,720)/720)];
}

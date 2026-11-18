// A rounded, slightly flattened lobe. Normals use the ellipsoid gradient.
export function leafMesh() {
  const positions = [], normals = [];
  const rings = 8, slices = 14, flatten = 0.79;
  function vertex(ring, slice) {
    const angle = Math.PI * ring / rings;
    const azimuth = 2 * Math.PI * slice / slices;
    const x = Math.sin(angle) * Math.cos(azimuth);
    const y = Math.cos(angle);
    const z = Math.sin(angle) * Math.sin(azimuth);
    positions.push(x, flatten * y, z);
    normals.push(x, y / flatten, z);
  }
  for (let ring=0; ring<rings; ring++) for (let slice=0; slice<slices; slice++) {
    vertex(ring,slice); vertex(ring+1,slice); vertex(ring+1,slice+1);
    vertex(ring,slice); vertex(ring+1,slice+1); vertex(ring,slice+1);
  }
  return {positions:new Float32Array(positions),normals:new Float32Array(normals),vCount:positions.length/3};
}

export async function crownLobes() {
  const response = await fetch(new URL("d3-crowns.tsv", import.meta.url));
  if (!response.ok) throw new Error("树冠模板读取失败");
  return (await response.text()).trim().split("\n").slice(1).map(row => {
    const [x,y,z,r,material] = row.split("\t");
    return {x:Number(x),y:Number(y),z:Number(z),r:Number(r),material};
  });
}

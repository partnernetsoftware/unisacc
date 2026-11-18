// A rounded, slightly flattened lobe. Normals use the ellipsoid gradient.
export function leafMesh(flatten = 0.79) {
  const positions = [], normals = [];
  const rings = 8, slices = 14;
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

export function grassMesh() {
  const positions = [];
  for (let i=0;i<7;i++) {
    const angle = 2.4*i;
    const x = Math.sin(angle), z = Math.cos(angle);
    const base = .03 + i*.021;
    positions.push(x*base-z*.043,0,z*base+x*.043,
      x*base+z*.043,0,z*base-x*.043,
      x*(base+.22),.34+(i%3)*.08,z*(base+.22));
  }
  const normals = new Float32Array(positions.length);
  for (let i=1;i<normals.length;i+=3) normals[i] = 1;
  return {positions:new Float32Array(positions),normals,vCount:positions.length/3};
}

export function foliagePatchMesh() {
  const points = [[-.61,-.15,0],[-.34,-.55,0],[.1,-.51,0],[.58,-.30,0],
    [.63,.10,0],[.28,.43,0],[.03,.33,0],[-.27,.53,0],[-.54,.25,0]];
  const positions = [], normals = [];
  for (let i=0;i<points.length;i++) {
    positions.push(0,0,.12,...points[i],...points[(i+1)%points.length]);
    normals.push(0,.28,1,0,.28,1,0,.28,1);
  }
  return {positions:new Float32Array(positions),normals:new Float32Array(normals),vCount:positions.length/3};
}

export function cutoutLeafMesh() {
  const positions = new Float32Array([
    -.74,-.62,0, .74,-.62,0, .74,.62,0,
    -.74,-.62,0, .74,.62,0, -.74,.62,0
  ]);
  const normals = new Float32Array([0,.28,1,0,.28,1,0,.28,1,0,.28,1,0,.28,1,0,.28,1]);
  const uvs = new Float32Array([0,0,1,0,1,1,0,0,1,1,0,1]);
  return {positions,normals,uvs,vCount:6};
}

export function canopyPatches(lobes, count, x, y, z, radius, warm, emit) {
  for(let i=0;i<count;i++) {
    const lobe=lobes[i%lobes.length];
    const height=2*((i*.61803398875)%1)-1;
    const angle=2.3999632297*i+x;
    const spread=.47*Math.sqrt(1-height*height);
    const sunward=height>.48 && Math.cos(angle)<.08;
    const material=height<-.17?"leafDark":warm && sunward?"leaf.light":height>.40?"leafLight":"leaf";
    emit(material,x+radius*(lobe.x+spread*Math.cos(angle)),
      y+radius*(lobe.y+height*.55),z+radius*(lobe.z+spread*Math.sin(angle)),
      radius*(.47+.22*lobe.r),angle,.39*Math.sin(i),.45*Math.cos(i*2));
  }
}

export async function crownLobes() {
  const response = await fetch(new URL("d3-crowns.tsv", import.meta.url));
  if (!response.ok) throw new Error("树冠模板读取失败");
  return (await response.text()).trim().split("\n").slice(1).map(row => {
    const [x,y,z,r,material] = row.split("\t");
    return {x:Number(x),y:Number(y),z:Number(z),r:Number(r),material};
  });
}

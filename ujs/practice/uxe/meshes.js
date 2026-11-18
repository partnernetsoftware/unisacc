/**
 * UXE preset meshes (unit size; scaled per instance).
 * mesh_id in UXEP → index here.
 */
export const MESH_OCTA = 0;
export const MESH_SHIP = 1;

/** Regular octahedron — rocks. */
export const OCTA_MESH = new Float32Array([
  0, 1, 0,  1, 0, 0,  0, 0, 1,
  0, 1, 0,  0, 0, 1, -1, 0, 0,
  0, 1, 0, -1, 0, 0,  0, 0,-1,
  0, 1, 0,  0, 0,-1,  1, 0, 0,
  0,-1, 0,  0, 0, 1,  1, 0, 0,
  0,-1, 0, -1, 0, 0,  0, 0, 1,
  0,-1, 0,  0, 0,-1, -1, 0, 0,
  0,-1, 0,  1, 0, 0,  0, 0,-1,
]);

/** Wedge ship: nose −Z, flat top (matches chase cam). */
export const SHIP_MESH = new Float32Array([
  // top fan
  0, 0.35, -1.2,   0.7, 0.1, 0.6,   -0.7, 0.1, 0.6,
  // bottom
  0, -0.25, -1.2,  -0.7, -0.15, 0.6,  0.7, -0.15, 0.6,
  // left
  0, 0.35, -1.2,  -0.7, 0.1, 0.6,  -0.7, -0.15, 0.6,
  0, 0.35, -1.2,  -0.7, -0.15, 0.6,  0, -0.25, -1.2,
  // right
  0, 0.35, -1.2,   0.7, -0.15, 0.6,  0.7, 0.1, 0.6,
  0, 0.35, -1.2,   0, -0.25, -1.2,  0.7, -0.15, 0.6,
  // rear
  -0.7, 0.1, 0.6,  0.7, 0.1, 0.6,  0.7, -0.15, 0.6,
  -0.7, 0.1, 0.6,  0.7, -0.15, 0.6,  -0.7, -0.15, 0.6,
]);

/** Unit box centered at origin (tile / building). */
export const BOX_MESH = new Float32Array([
  // +Y
  -0.5, 0.5, -0.5,  0.5, 0.5, -0.5,  0.5, 0.5, 0.5,
  -0.5, 0.5, -0.5,  0.5, 0.5, 0.5,  -0.5, 0.5, 0.5,
  // -Y
  -0.5, -0.5, -0.5,  0.5, -0.5, 0.5,  0.5, -0.5, -0.5,
  -0.5, -0.5, -0.5,  -0.5, -0.5, 0.5,  0.5, -0.5, 0.5,
  // +Z
  -0.5, -0.5, 0.5,  0.5, 0.5, 0.5,  0.5, -0.5, 0.5,
  -0.5, -0.5, 0.5,  -0.5, 0.5, 0.5,  0.5, 0.5, 0.5,
  // -Z
  -0.5, -0.5, -0.5,  0.5, -0.5, -0.5,  0.5, 0.5, -0.5,
  -0.5, -0.5, -0.5,  0.5, 0.5, -0.5,  -0.5, 0.5, -0.5,
  // +X
  0.5, -0.5, -0.5,  0.5, -0.5, 0.5,  0.5, 0.5, 0.5,
  0.5, -0.5, -0.5,  0.5, 0.5, 0.5,  0.5, 0.5, -0.5,
  // -X
  -0.5, -0.5, -0.5,  -0.5, 0.5, -0.5,  -0.5, 0.5, 0.5,
  -0.5, -0.5, -0.5,  -0.5, 0.5, 0.5,  -0.5, -0.5, 0.5,
]);

export const MESH_BOX = 2;
const BOX_NORMALS = new Float32Array(
  [[0,1,0],[0,-1,0],[0,0,1],[0,0,-1],[1,0,0],[-1,0,0]]
    .flatMap(normal => Array.from({length:6}, () => normal).flat())
);

/** XZ unit quad (water). */
export const PLANE_MESH = new Float32Array([
  -0.5, 0, -0.5,  0.5, 0, -0.5,  0.5, 0, 0.5,
  -0.5, 0, -0.5,  0.5, 0, 0.5,  -0.5, 0, 0.5,
]);
export const PLANE_UV = new Float32Array([
  0, 0,  1, 0,  1, 1,
  0, 0,  1, 1,  0, 1,
]);
export const MESH_PLANE = 3;
export const MESH_POINTS = 4;
export const POINT_MESH = new Float32Array([
  -0.5, -0.5, 0,  0.5, -0.5, 0,  0.5, 0.5, 0,
  -0.5, -0.5, 0,  0.5, 0.5, 0,  -0.5, 0.5, 0,
]);
export const POINT_UV = new Float32Array([
  0, 0,  1, 0,  1, 1,
  0, 0,  1, 1,  0, 1,
]);

function zeros2(n) { return new Float32Array(n * 2); }

const TABLE = [
  { id: MESH_OCTA, positions: OCTA_MESH, uvs: zeros2(OCTA_MESH.length / 3), vCount: OCTA_MESH.length / 3 },
  { id: MESH_SHIP, positions: SHIP_MESH, uvs: zeros2(SHIP_MESH.length / 3), vCount: SHIP_MESH.length / 3 },
  { id: MESH_BOX, positions: BOX_MESH, normals: BOX_NORMALS, uvs: zeros2(BOX_MESH.length / 3), vCount: BOX_MESH.length / 3 },
  { id: MESH_PLANE, positions: PLANE_MESH, normals: new Float32Array([0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0, 0,1,0]), uvs: PLANE_UV, vCount: PLANE_MESH.length / 3 },
  { id: MESH_POINTS, positions: POINT_MESH, uvs: POINT_UV, vCount: POINT_MESH.length / 3 },
];


export function meshById(id) {
  return TABLE[id] || TABLE[0];
}

export function allMeshes() {
  return TABLE;
}

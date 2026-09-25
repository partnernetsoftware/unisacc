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

const TABLE = [
  { id: MESH_OCTA, positions: OCTA_MESH, vCount: OCTA_MESH.length / 3 },
  { id: MESH_SHIP, positions: SHIP_MESH, vCount: SHIP_MESH.length / 3 },
  { id: MESH_BOX, positions: BOX_MESH, vCount: BOX_MESH.length / 3 },
];

export function meshById(id) {
  return TABLE[id] || TABLE[0];
}

export function allMeshes() {
  return TABLE;
}

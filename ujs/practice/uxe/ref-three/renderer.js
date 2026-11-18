import * as THREE from "../../web/game/three.module.js";
import {allMeshes,MESH_PLANE} from "../meshes.js";

const params=new URLSearchParams(window.parent.location.search);
const d4=params.get("scene")==="d4";
const cast=!params.has("no-shadow");
const atmosphere=!params.has("no-air");
const grade=!params.has("no-grade");
const converted=([r,g,b])=>new THREE.Color().setRGB(r,g,b,THREE.SRGBColorSpace);
const linear=(channels)=>new THREE.Color().setRGB(...channels);

function geometry(mesh) {
  const vertices=new Float32Array(mesh.positions);
  const normals=mesh.normals && new Float32Array(mesh.normals);
  const uv=new Float32Array(mesh.uvs ?? mesh.uv ?? vertices.length/3*2);
  if(!mesh.uvs && !mesh.uv) for(let i=0;i<vertices.length;i+=3) {
    const index=i/3*2;
    const up=Math.abs(normals?.[i+1]??0)>.65;
    uv[index]=vertices[i]*.48+.5;
    uv[index+1]=vertices[i+(up?2:1)]*.48+.5;
  }
  if(normals) for(let i=0;i<vertices.length;i+=9) {
    const a=new THREE.Vector3().fromArray(vertices,i);
    const b=new THREE.Vector3().fromArray(vertices,i+3).sub(a);
    const c=new THREE.Vector3().fromArray(vertices,i+6).sub(a);
    if(b.cross(c).dot(new THREE.Vector3().fromArray(normals,i))>=0) continue;
    for(const a of [vertices,normals]) for(let k=0;k<3;k++)
      [a[i+3+k],a[i+6+k]]=[a[i+6+k],a[i+3+k]];
    const j=i/3*2;
    for(let k=0;k<2;k++) [uv[j+2+k],uv[j+4+k]]=[uv[j+4+k],uv[j+2+k]];
  }
  const g=new THREE.BufferGeometry();
  g.setAttribute("position",new THREE.BufferAttribute(vertices,3));
  g.setAttribute("uv",new THREE.BufferAttribute(uv,2));
  if(normals) g.setAttribute("normal",new THREE.BufferAttribute(normals,3));
  else g.computeVertexNormals();
  return g;
}

function ramp(levels) {
  const values=Array.from({length:levels},(_,i)=>Math.round(92+163*i/(levels-1)));
  const map=new THREE.DataTexture(new Uint8Array(values),levels,1,THREE.RedFormat);
  map.minFilter=map.magFilter=THREE.NearestFilter;
  map.needsUpdate=true;
  return map;
}

function texture(source) {
  const map=new THREE.DataTexture(source.rgba,source.w,source.h,THREE.RGBAFormat);
  map.wrapS=map.wrapT=THREE.RepeatWrapping;
  map.magFilter=source.filter?THREE.LinearFilter:THREE.NearestFilter;
  map.minFilter=THREE.LinearFilter;
  map.colorSpace=THREE.SRGBColorSpace;
  map.needsUpdate=true;
  return map;
}

function surface(source,role) {
  const data=new Uint8Array(source.rgba);
  for(let p=0;p<data.length;p+=4) {
    const l=data[p]/255;
    const detail=role==="leaf-cutout"?.48:role==="masonry"?.34:.22;
    const value=Math.round(255*(1-detail+detail*l));
    data[p]=data[p+1]=data[p+2]=value;
  }
  const map=texture({...source,rgba:data});
  if(role==="leaf-cutout") map.wrapS=map.wrapT=THREE.ClampToEdgeWrapping;
  return map;
}

function reverseHull(original) {
  const shell=original.clone();
  const v=shell.attributes.position,n=shell.attributes.normal;
  for(let i=0;i<v.count;i++) v.setXYZ(i,
    v.getX(i)+.018*n.getX(i),v.getY(i)+.018*n.getY(i),v.getZ(i)+.018*n.getZ(i));
  return shell;
}

function cloudPuffs(scene) {
  const canvas=document.createElement("canvas");
  canvas.width=256; canvas.height=128;
  const ctx=canvas.getContext("2d");
  for(let i=0;i<26;i++) {
    const x=32+i*7,y=67+13*Math.sin(i*1.4);
    const r=14+11*Math.sin(i*.62)**2;
    const gradient=ctx.createRadialGradient(x,y,2,x,y,r);
    gradient.addColorStop(0,"rgba(255,246,224,.71)");
    gradient.addColorStop(1,"rgba(255,246,224,0)");
    ctx.fillStyle=gradient;
    ctx.fillRect(x-r,y-r,2*r,2*r);
  }
  const map=new THREE.CanvasTexture(canvas);
  map.colorSpace=THREE.SRGBColorSpace;
  for(const [x,y,z,w,h] of [[-27,27,-35,21,9],[18,23,-40,29,11],[-8,30,-60,36,12],[43,22,-66,26,10]]) {
    const puff=new THREE.Sprite(new THREE.SpriteMaterial({map,opacity:.9,depthWrite:false,fog:false}));
    puff.position.set(x,y,z);
    puff.scale.set(w,h,1);
    scene.add(puff);
  }
}

export function createWebGLRenderer(canvas,{extraMeshes=[]}={}) {
  const renderer=new THREE.WebGLRenderer({canvas,antialias:true,preserveDrawingBuffer:true});
  renderer.setSize(1280,720,false);
  renderer.outputColorSpace=THREE.SRGBColorSpace;
  renderer.toneMapping=grade?THREE.ACESFilmicToneMapping:THREE.NoToneMapping;
  renderer.toneMappingExposure=1.18;
  renderer.shadowMap.enabled=cast;
  renderer.shadowMap.type=THREE.PCFSoftShadowMap;
  const meshes=[...allMeshes(),...extraMeshes];
  const trace={three:THREE.REVISION,cast,atmosphere,grade};
  return {render(state,view,textures) {
    const scene=new THREE.Scene();
    const camera=new THREE.PerspectiveCamera(THREE.MathUtils.radToDeg(view.fovy),1280/720,view.near,view.far);
    camera.position.fromArray(view.eye);
    camera.lookAt(...view.target);
    scene.background=converted(state.clearColor);
    if(state.backdrop) {
      const src=textures.find(t=>t.id===state.backdrop.mapId);
      if(src) {
        const bg=texture(src);
        bg.wrapS=bg.wrapT=THREE.ClampToEdgeWrapping;
        bg.flipY=true;
        bg.needsUpdate=true;
        scene.background=bg;
      }
    }
    if(atmosphere) {
      scene.fog=new THREE.Fog(converted(d4?[.54,.70,.77]:[.62,.76,.82]),d4?21:13,d4?96:65);
      if(d4) cloudPuffs(scene);
    }
    const sun=new THREE.DirectionalLight(converted([1,.91,.79]),1.85);
    const axis=state.lights[0].dir;
    sun.position.set(...axis.map(a=>a*40));
    sun.castShadow=cast;
    Object.assign(sun.shadow.camera,{left:-28,right:28,top:25,bottom:-25,near:.5,far:100});
    sun.shadow.camera.updateProjectionMatrix();
    sun.shadow.mapSize.set(2048,2048);
    sun.shadow.bias=-.00052;
    sun.shadow.normalBias=.027;
    scene.add(sun,sun.target);
    scene.add(new THREE.HemisphereLight(converted([.78,.89,1]),converted([.55,.54,.38]),1.14));
    scene.add(new THREE.AmbientLight(linear([.85,.90,1]),.32));
    const geometryCache=new Map(),shellCache=new Map(),maps=new Map(),ramps=new Map();
    let drawn=0;
    const transform=new THREE.Object3D();
    const ink=new THREE.MeshBasicMaterial({color:converted([.13,.16,.20]),side:THREE.BackSide});
    for(const cloud of state.clouds.values()) {
      if(cloud.screenSpace || !cloud.count) continue;
      if(!geometryCache.has(cloud.meshId)) geometryCache.set(cloud.meshId,geometry(meshes[cloud.meshId]));
      const g=geometryCache.get(cloud.meshId);
      const levels=Math.max(3,cloud.toonLevels||4);
      if(!ramps.has(levels)) ramps.set(levels,ramp(levels));
      const src=textures.find(t=>t.id===cloud.mapId);
      let map=null;
      if(src) {
        const key=cloud.mapId+"/"+cloud.surfaceMap;
        if(!maps.has(key)) maps.set(key,surface(src,cloud.surfaceMap));
        map=maps.get(key);
      }
      const material=new THREE.MeshToonMaterial({color:converted(cloud.color),map,gradientMap:ramps.get(levels),
        side:THREE.DoubleSide,alphaTest:cloud.surfaceMap==="leaf-cutout"?.35:0});
      const instances=new THREE.InstancedMesh(g,material,cloud.count);
      instances.castShadow=cast && cloud.meshId!==MESH_PLANE;
      instances.receiveShadow=cast;
      let outlines=null;
      if(!cloud.outlineExcluded && cloud.meshId!==MESH_PLANE) {
        if(!shellCache.has(cloud.meshId)) shellCache.set(cloud.meshId,reverseHull(g));
        outlines=new THREE.InstancedMesh(shellCache.get(cloud.meshId),ink,cloud.count);
      }
      for(let i=0;i<cloud.count;i++) {
        transform.position.fromArray(cloud.xyz,i*3);
        transform.rotation.set(cloud.pitch[i],cloud.yaw[i],cloud.roll[i],"ZXY");
        transform.scale.setScalar(cloud.scale[i]);
        if(cloud.scaleY) transform.scale.y=cloud.scaleY[i];
        transform.updateMatrix();
        instances.setMatrixAt(i,transform.matrix);
        if(outlines) outlines.setMatrixAt(i,transform.matrix);
      }
      scene.add(instances);
      if(outlines) scene.add(outlines);
      drawn+=cloud.count;
    }
    renderer.render(scene,camera);
    const result={...trace,drawn,eye:view.eye,target:view.target,
      calls:renderer.info.render.calls,triangles:renderer.info.render.triangles};
    canvas.dataset.comparison=JSON.stringify(result);
    window.parent.postMessage(result,"*");
    document.title="THREE_REFERENCE_READY";
  }};
}

import {createBrowserHost} from "../host-browser.js";

const channels=[[0,0],[404,947],[377,936],[675,946],[700,700],[500,200],[800,320],[80,900]];

function expandMesh(bin,layout) {
  const floats=(i,width)=>new Float32Array(bin,layout.bufferViews[i].byteOffset,800*width);
  const p=floats(0,3),n=floats(1,3),uv=floats(2,2);
  const indices=new Uint16Array(bin,layout.bufferViews[3].byteOffset,1200);
  const q=layout.nodes[0].rotation;
  const positions=new Float32Array(indices.length*3),normals=new Float32Array(positions.length);
  const uvs=new Float32Array(indices.length*2);
  function turn(source,index,target,i) {
    const [x,y,z]=source.subarray(index*3,index*3+3);
    const a=2*(q[1]*z-q[2]*y),b=2*(q[2]*x-q[0]*z),c=2*(q[0]*y-q[1]*x);
    target.set([x+q[3]*a+q[1]*c-q[2]*b,y+q[3]*b+q[2]*a-q[0]*c,z+q[3]*c+q[0]*b-q[1]*a],i*3);
  }
  let minY=Infinity;
  for(let i=0;i<indices.length;i++) {
    const index=indices[i];
    turn(p,index,positions,i);
    turn(n,index,normals,i);
    minY=Math.min(minY,positions[i*3+1]);
    uvs.set(uv.subarray(index*2,index*2+2),i*2);
  }
  for(let i=1;i<positions.length;i+=3) positions[i]-=minY;
  return {positions,normals,uvs,vCount:indices.length};
}

export async function loadBush() {
  const started=performance.now();
  const baseURL=new URL("file:///tmp/");
  const scratch=document.createElement("canvas");
  const host=await createBrowserHost(scratch,{prefer:"webgl",baseURL});
  const [bin,meta,png]=await Promise.all(["cdx2-bush-mesh.bin","cdx2-bush-layout.json","cdx2-bush-atlas.png"]
    .map(path=>host.host_asset_read(path)));
  const loaded=performance.now();
  const mesh=expandMesh(bin.buffer,JSON.parse(new TextDecoder().decode(meta)));
  const image=await createImageBitmap(new Blob([png],{type:"image/png"}),
    {premultiplyAlpha:"none",colorSpaceConversion:"none"});
  const gl=scratch.getContext("webgl"),tex=gl.createTexture(),fb=gl.createFramebuffer();
  gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,0);
  gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL,gl.NONE);
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,0);
  gl.bindTexture(gl.TEXTURE_2D,tex);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);
  gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,image);
  gl.bindFramebuffer(gl.FRAMEBUFFER,fb);
  gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,tex,0);
  if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE) throw Error("PNG 试验读回帧缓冲失败");
  const w=image.width,h=image.height,rgba=new Uint8Array(w*h*4);
  gl.readPixels(0,0,w,h,gl.RGBA,gl.UNSIGNED_BYTE,rgba);
  const decodeMs=performance.now()-loaded;
  image.close();
  gl.deleteFramebuffer(fb);
  gl.deleteTexture(tex);
  return {mesh,texture:{id:31,w,h,rgba,filter:0},readMs:loaded-started,decodeMs};
}

export async function createStudyRenderer(canvas,extraMeshes,bush) {
  const sourceURL=new URL("../renderer-webgl.js",import.meta.url);
  const response=await fetch(sourceURL);
  if(!response.ok) throw Error("UXE 渲染器读取失败");
  let source=await response.text();
  const edit=(anchor,next)=>{
    if(source.split(anchor).length!==2) throw Error("UXE 素材实验锚点变化");
    source=source.replace(anchor,()=>next);
  };
  source=source.replace(/from "(\.\/[^\"]+)"/g,(_,path)=>`from ${JSON.stringify(new URL(path,sourceURL).href)}`);
  edit('cloud.surfaceMap === "metal-stain" ? 4 :',
    'cloud.surfaceMap === "base-color-mask" ? 6 : cloud.surfaceMap === "metal-stain" ? 4 :');
  edit('  if (uShadowPass > 0.5) {',`  bool bush = uHasMap > 0.5 && uSurfaceMap > 5.5;
  if (bush && texture2D(uMap, vUV).a < 0.5) discard;
  if (uShadowPass > 0.5) {`);
  edit('vec3 n = normalize(vN);','vec3 n = normalize(bush && !gl_FrontFacing ? -vN : vN);');
  edit('  if (uHasMap > 0.5 && uSurfaceMap > 0.5) {',`  if (bush) base = texture2D(uMap, vUV).rgb;
  if (uHasMap > 0.5 && uSurfaceMap > 0.5 && !bush) {`);
  edit('          texCache[id] = { hash: h, tex: tex };',
    '          texCache[id] = { hash: h, tex: tex }; if (id === 31) opts.readBushTexture(gl,tex);');
  const moduleURL=URL.createObjectURL(new Blob([source],{type:"text/javascript"}));
  let factory;
  try { factory=(await import(moduleURL)).createWebGLRenderer; }
  finally { URL.revokeObjectURL(moduleURL); }
  let samples=[];
  const setup=performance.now();
  const renderer=factory(canvas,{antialias:false,extraMeshes,readBushTexture(gl,tex){
    const saved=gl.getParameter(gl.FRAMEBUFFER_BINDING),fb=gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER,fb);
    gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,tex,0);
    if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE) throw Error("v4 贴图读回帧缓冲失败");
    samples=channels.flatMap(([x,y])=>{
      const rgba=new Uint8Array(4);
      gl.readPixels(x,y,1,1,gl.RGBA,gl.UNSIGNED_BYTE,rgba);
      return [...rgba];
    });
    gl.bindFramebuffer(gl.FRAMEBUFFER,saved);
    gl.deleteFramebuffer(fb);
  }});
  const initialized=performance.now();
  const render=renderer.render.bind(renderer);
  renderer.render=(scene,camera,textures)=>{
    const first=!document.querySelector("output").dataset.bush;
    const start=performance.now();
    render(scene,camera,textures);
    if(first && samples.length) {
      const gl=canvas.getContext("webgl"),pixel=new Uint8Array(4);
      gl.readPixels(640,360,1,1,gl.RGBA,gl.UNSIGNED_BYTE,pixel);
      document.querySelector("output").dataset.bush=[bush.texture.w,bush.texture.h,...samples].join(",");
      document.querySelector("output").dataset.timing=[bush.readMs,bush.decodeMs,initialized-setup,performance.now()-start].join(",");
    }
  };
  return renderer;
}

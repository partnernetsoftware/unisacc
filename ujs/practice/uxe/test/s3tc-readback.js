// Browser fixture for reading an r160 CompressedTexture through WebGL2.
export function readS3tc(texture, extensionToken, width, height) {
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const gl = canvas.getContext("webgl2", { alpha: true, premultipliedAlpha: false });
  const ext = gl?.getExtension("WEBGL_compressed_texture_s3tc");
  if (!ext || ext[extensionToken] !== texture.format) throw Error("r160 S3TC GPU format unavailable");
  const gpu = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, gpu);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  const mip = texture.mipmaps[0];
  gl.compressedTexImage2D(gl.TEXTURE_2D, 0, texture.format,
    Math.ceil(mip.width / 4) * 4, Math.ceil(mip.height / 4) * 4, 0, mip.data);
  if (gl.getError() !== gl.NO_ERROR) throw Error("r160 S3TC upload failed");
  const program = gl.createProgram();
  const stages = [
    [gl.VERTEX_SHADER, "#version 300 es\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0.0,1.0);}"],
    [gl.FRAGMENT_SHADER, "#version 300 es\nprecision highp float;uniform sampler2D image;out vec4 rgba;void main(){rgba=texelFetch(image,ivec2(gl_FragCoord.xy),0);}"],
  ];
  for (const [stage, source] of stages) {
    const shader = gl.createShader(stage);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) throw Error(gl.getShaderInfoLog(shader));
    gl.attachShader(program, shader);
    gl.deleteShader(shader);
  }
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw Error(gl.getProgramInfoLog(program));
  gl.useProgram(program);
  gl.drawArrays(gl.TRIANGLES, 0, 3);
  const pixels = new Uint8Array(width * height * 4);
  gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
  if (gl.getError() !== gl.NO_ERROR) throw Error("r160 S3TC readback failed");
  gl.deleteProgram(program);
  gl.deleteTexture(gpu);
  return new DataView(pixels.buffer);
}

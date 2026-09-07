// Vischeck Animal Vision: live-camera / still-image simulator.
// GPU pipeline (WebGL2):
//   source -> [colour: sRGB decode, species matrix, rod mix] -> [Gaussian blur H] -> [Gaussian blur V]
//          -> [final: optional ommatidial (hex) resampling, split view, sRGB encode]
// All blur and sampling is done in SCENE degrees (camera field of view), not screen pixels.
// Model constants come from model/derive_animal_models.py via animal_models.js.

import { ANIMAL_MODELS } from './animal_models.js';

const $ = (id) => {
  const el = document.getElementById(id);
  if (!el) throw new Error(`Missing element: ${id}`);
  return el;
};

const VS = `#version 300 es
in vec2 aPos; out vec2 vUv;
void main(){ vUv = aPos*0.5+0.5; gl_Position = vec4(aPos,0.0,1.0); }`;

// Pass 1: colour transform in linear light.
const FS_COLOR = `#version 300 es
precision highp float;
uniform sampler2D uSrc; uniform mat3 uSim; uniform vec3 uRod; uniform float uLight; uniform float uCrop;
in vec2 vUv; out vec4 o;
vec3 dec(vec3 c){ return mix(c/12.92, pow((c+0.055)/1.055, vec3(2.4)), step(vec3(0.04045), c)); }
void main(){
  vec2 uv = 0.5 + (vUv-0.5)*uCrop;
  vec3 c = dec(texture(uSrc, uv).rgb);
  vec3 photopic = uSim*c;
  vec3 scotopic = vec3(dot(uRod, c)) * 0.6;   // rod-weighted grey, dimmed (illustrative)
  o = vec4(mix(scotopic, photopic, uLight), 1.0);
}`;

// Pass 2/3: separable Gaussian, sigma in pixels of the working texture.
const FS_BLUR = `#version 300 es
precision highp float;
uniform sampler2D uSrc; uniform vec2 uDir; uniform float uSigma;
in vec2 vUv; out vec4 o;
void main(){
  if (uSigma < 0.2) { o = texture(uSrc, vUv); return; }
  int r = min(int(ceil(3.0*uSigma)), 24);
  float wsum = 0.0; vec3 acc = vec3(0.0);
  for (int i = -24; i <= 24; i++) {
    if (abs(i) > r) continue;
    float w = exp(-float(i*i)/(2.0*uSigma*uSigma));
    acc += w*texture(uSrc, clamp(vUv + float(i)*uDir, 0.0, 1.0)).rgb; wsum += w;
  }
  o = vec4(acc/wsum, 1.0);
}`;

// Pass 4: ommatidial mosaic (nearest hex centre), split view, sRGB encode.
const FS_FINAL = `#version 300 es
precision highp float;
uniform sampler2D uSim; uniform sampler2D uOrig; uniform vec2 uRes;
uniform float uSplit, uHex, uHexR, uEdges, uCrop;
in vec2 vUv; out vec4 o;
vec3 enc(vec3 c){ c = clamp(c, 0.0, 1.0); return mix(c*12.92, 1.055*pow(c, vec3(1.0/2.4))-0.055, step(vec3(0.0031308), c)); }
vec2 hexCenter(vec2 p, float R){
  float q = (0.57735027*p.x - 0.33333333*p.y)/R;
  float r = (0.66666667*p.y)/R;
  float x = q, z = r, y = -x-z;
  float rx = floor(x+0.5), ry = floor(y+0.5), rz = floor(z+0.5);
  float dx = abs(rx-x), dy = abs(ry-y), dz = abs(rz-z);
  if (dx > dy && dx > dz) rx = -ry-rz; else if (dy > dz) ry = -rx-rz; else rz = -rx-ry;
  return vec2(R*(1.7320508*rx + 0.8660254*rz), R*1.5*rz);
}
void main(){
  vec2 px = vUv*uRes;
  bool line = abs(vUv.x-uSplit)*uRes.x < 1.0 && uSplit > 0.0 && uSplit < 1.0;
  if (vUv.x < uSplit) {
    vec2 uv = 0.5 + (vUv-0.5)*uCrop;
    o = vec4(line ? vec3(1.0) : texture(uOrig, uv).rgb, 1.0); return;
  }
  vec3 c;
  if (uHex > 0.5) {
    vec2 ctr = hexCenter(px, uHexR);
    c = texture(uSim, clamp(ctr/uRes, 0.0, 1.0)).rgb;
    if (uEdges > 0.5) { float d = length(px-ctr)/uHexR; c *= 1.0 - 0.3*smoothstep(0.78, 0.92, d); }
  } else {
    c = texture(uSim, vUv).rgb;
  }
  o = vec4(line ? vec3(1.0) : enc(c), 1.0);
}`;

// ---------------------------------------------------------------- WebGL helpers
function compile(gl, type, src) {
  const sh = gl.createShader(type);
  gl.shaderSource(sh, src); gl.compileShader(sh);
  if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(sh));
  return sh;
}
function program(gl, fs) {
  const p = gl.createProgram();
  gl.attachShader(p, compile(gl, gl.VERTEX_SHADER, VS));
  gl.attachShader(p, compile(gl, gl.FRAGMENT_SHADER, fs));
  gl.linkProgram(p);
  if (!gl.getProgramParameter(p, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(p));
  const u = {};
  const n = gl.getProgramParameter(p, gl.ACTIVE_UNIFORMS);
  for (let i = 0; i < n; i++) { const info = gl.getActiveUniform(p, i); u[info.name] = gl.getUniformLocation(p, info.name); }
  return { p, u };
}
function makeTex(gl, filter = gl.LINEAR) {
  const t = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, t);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  return t;
}
function makeFbo(gl, w, h, halfFloat) {
  const tex = makeTex(gl);
  gl.bindTexture(gl.TEXTURE_2D, tex);
  if (halfFloat) gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA16F, w, h, 0, gl.RGBA, gl.HALF_FLOAT, null);
  else gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  const fbo = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
  const ok = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  return { fbo, tex, w, h, ok };
}

// ---------------------------------------------------------------- app state
const state = {
  species: 'dog',
  fovDeg: 60,
  trueScale: false,
  screenCm: 7,
  distCm: 30,
  light: 1.0,
  split: 0.5,
  hex: true,
  edges: true,
  blurGain: 1.0,
  source: null,        // HTMLVideoElement | HTMLImageElement
  sourceIsVideo: false,
  facing: 'environment',
  stream: null,
  dirty: true,
};

let gl, canvas, progColor, progBlur, progFinal, srcTex, fboA, fboB, fboC, halfFloat;
let fps = { t: performance.now(), n: 0 };

function setStatus(msg) { $('status').textContent = msg; }

function init() {
  canvas = $('canvasOut');
  gl = canvas.getContext('webgl2', { preserveDrawingBuffer: true, antialias: false });
  if (!gl) { setStatus('WebGL2 is not available in this browser.'); return false; }
  halfFloat = !!gl.getExtension('EXT_color_buffer_float');
  progColor = program(gl, FS_COLOR);
  progBlur = program(gl, FS_BLUR);
  progFinal = program(gl, FS_FINAL);
  const vao = gl.createVertexArray(); gl.bindVertexArray(vao);
  const buf = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, buf);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(0); gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
  srcTex = makeTex(gl, gl.LINEAR_MIPMAP_LINEAR);
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
  return true;
}

function ensureFbos(w, h) {
  if (fboA && fboA.w === w && fboA.h === h) return;
  for (const f of [fboA, fboB, fboC]) if (f) { gl.deleteFramebuffer(f.fbo); gl.deleteTexture(f.tex); }
  fboA = makeFbo(gl, w, h, halfFloat); fboB = makeFbo(gl, w, h, halfFloat); fboC = makeFbo(gl, w, h, halfFloat);
  if (!fboA.ok && halfFloat) { halfFloat = false; fboA = fboB = fboC = null; ensureFbos(w, h); }
}

function sourceSize() {
  const s = state.source;
  if (!s) return [0, 0];
  return state.sourceIsVideo ? [s.videoWidth, s.videoHeight] : [s.naturalWidth, s.naturalHeight];
}

// Geometry: how many pixels of the OUTPUT canvas correspond to one degree of the SCENE.
function geometry() {
  const crop = state.trueScale
    ? Math.min(1, Math.max(0.05, (2 * Math.atan(state.screenCm / (2 * state.distCm)) * 180 / Math.PI) / state.fovDeg))
    : 1;
  const ppd = canvas.width / (state.fovDeg * crop);
  return { crop, ppd };
}

function currentModel() { return ANIMAL_MODELS.species[state.species]; }

function blurSigmaDeg(m) {
  let s = m.blur_sigma_deg || 0;
  if (m.blur_sigma_dim_deg !== undefined) {
    // interpolate (in variance) between bright and dim acuity as light level drops
    const sd = m.blur_sigma_dim_deg;
    s = Math.sqrt(state.light * s * s + (1 - state.light) * sd * sd);
  }
  return s * state.blurGain;
}

function render() {
  const [sw, sh] = sourceSize();
  if (!sw || !sh) return;
  if (canvas.width !== sw || canvas.height !== sh) { canvas.width = sw; canvas.height = sh; }
  const m = currentModel();
  const { crop, ppd } = geometry();
  const sigmaPx = blurSigmaDeg(m) * ppd;
  // Work at reduced resolution when the blur is large (mipmapped source keeps it alias-free).
  const ds = Math.max(1, Math.floor(sigmaPx / 2.5));
  const ww = Math.max(2, Math.ceil(sw / ds)), wh = Math.max(2, Math.ceil(sh / ds));
  ensureFbos(ww, wh);

  // upload source
  gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, srcTex);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, state.source);
  gl.generateMipmap(gl.TEXTURE_2D);

  // pass 1: colour
  const T = m.sim_matrix; // row-major; GLSL wants column-major
  const colMajor = new Float32Array([T[0][0], T[1][0], T[2][0], T[0][1], T[1][1], T[2][1], T[0][2], T[1][2], T[2][2]]);
  const rod = m.rod_weights || [0.2126, 0.7152, 0.0722];
  gl.bindFramebuffer(gl.FRAMEBUFFER, fboA.fbo); gl.viewport(0, 0, ww, wh);
  gl.useProgram(progColor.p);
  gl.uniform1i(progColor.u.uSrc, 0);
  gl.uniformMatrix3fv(progColor.u.uSim, false, colMajor);
  gl.uniform3fv(progColor.u.uRod, rod);
  gl.uniform1f(progColor.u.uLight, state.light);
  gl.uniform1f(progColor.u.uCrop, crop);
  gl.drawArrays(gl.TRIANGLES, 0, 3);

  // pass 2/3: blur
  const sWork = sigmaPx / ds;
  gl.useProgram(progBlur.p); gl.uniform1i(progBlur.u.uSrc, 0); gl.uniform1f(progBlur.u.uSigma, sWork);
  gl.bindFramebuffer(gl.FRAMEBUFFER, fboB.fbo); gl.bindTexture(gl.TEXTURE_2D, fboA.tex);
  gl.uniform2f(progBlur.u.uDir, 1 / ww, 0); gl.drawArrays(gl.TRIANGLES, 0, 3);
  gl.bindFramebuffer(gl.FRAMEBUFFER, fboC.fbo); gl.bindTexture(gl.TEXTURE_2D, fboB.tex);
  gl.uniform2f(progBlur.u.uDir, 0, 1 / wh); gl.drawArrays(gl.TRIANGLES, 0, 3);

  // pass 4: final
  gl.bindFramebuffer(gl.FRAMEBUFFER, null); gl.viewport(0, 0, sw, sh);
  gl.useProgram(progFinal.p);
  gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, fboC.tex);
  gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, srcTex);
  gl.uniform1i(progFinal.u.uSim, 0); gl.uniform1i(progFinal.u.uOrig, 1);
  gl.uniform2f(progFinal.u.uRes, sw, sh);
  gl.uniform1f(progFinal.u.uSplit, state.split);
  const useHex = state.hex && m.interommatidial_deg;
  gl.uniform1f(progFinal.u.uHex, useHex ? 1 : 0);
  gl.uniform1f(progFinal.u.uHexR, useHex ? (m.interommatidial_deg * ppd) / Math.sqrt(3) : 1);
  gl.uniform1f(progFinal.u.uEdges, state.edges ? 1 : 0);
  gl.uniform1f(progFinal.u.uCrop, crop);
  gl.drawArrays(gl.TRIANGLES, 0, 3);
  gl.activeTexture(gl.TEXTURE0);

  $('geomInfo').textContent =
    `${sw}×${sh} px, ${ppd.toFixed(1)} px/deg of scene` +
    (state.trueScale ? `, showing central ${(state.fovDeg * crop).toFixed(1)}° at true scale` : '') +
    `, blur σ = ${blurSigmaDeg(m).toFixed(3)}° (${sigmaPx.toFixed(1)} px)` +
    (useHex ? `, facet spacing ${m.interommatidial_deg}° (${(m.interommatidial_deg * ppd).toFixed(1)} px)` : '') +
    (halfFloat ? '' : ' [8-bit FBO]');
}

function loop() {
  if (state.source && (state.sourceIsVideo || state.dirty)) {
    if (!state.sourceIsVideo || state.source.readyState >= 2) { render(); state.dirty = false; }
    fps.n++;
    const now = performance.now();
    if (now - fps.t > 1000) { $('fpsInfo').textContent = state.sourceIsVideo ? `${(fps.n * 1000 / (now - fps.t)).toFixed(0)} fps` : ''; fps.t = now; fps.n = 0; }
  }
  requestAnimationFrame(loop);
}

// ---------------------------------------------------------------- sources
async function startCamera() {
  stopCamera();
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) { setStatus('Camera API not available (needs HTTPS and a modern browser).'); return; }
  try {
    const stream = await navigator.mediaDevices.getUserMedia({
      video: { facingMode: state.facing, width: { ideal: 1280 }, height: { ideal: 720 } }, audio: false,
    });
    const v = $('video');
    v.srcObject = stream; v.setAttribute('playsinline', ''); v.muted = true;
    await v.play();
    state.stream = stream; state.source = v; state.sourceIsVideo = true; state.dirty = true;
    if (!$('fov').dataset.userSet) { state.fovDeg = 70; $('fov').value = 70; $('fovVal').textContent = '70'; }
    setStatus(`Camera running (${state.facing}). Camera field of view is assumed, not measured: adjust the FOV slider for your phone.`);
  } catch (e) {
    setStatus(`Camera error: ${e.message || e}`);
  }
}
function stopCamera() {
  if (state.stream) { for (const t of state.stream.getTracks()) t.stop(); state.stream = null; }
  if (state.sourceIsVideo) { state.source = null; state.sourceIsVideo = false; }
}
function loadImage(url) {
  return new Promise((resolve, reject) => {
    const img = new Image(); img.crossOrigin = 'anonymous';
    img.onload = () => resolve(img); img.onerror = reject; img.src = url;
  });
}
async function useImage(url) {
  stopCamera();
  try {
    const img = await loadImage(url);
    // keep GPU textures modest on phones
    let src = img;
    const maxDim = 1600;
    if (Math.max(img.naturalWidth, img.naturalHeight) > maxDim) {
      const s = maxDim / Math.max(img.naturalWidth, img.naturalHeight);
      const c = document.createElement('canvas'); c.width = Math.round(img.naturalWidth * s); c.height = Math.round(img.naturalHeight * s);
      c.getContext('2d').drawImage(img, 0, 0, c.width, c.height);
      src = await loadImage(c.toDataURL('image/png'));
    }
    state.source = src; state.sourceIsVideo = false; state.dirty = true;
    setStatus('Still image loaded. Set the field of view the photo covers (phone main camera ≈ 65–75°).');
  } catch (e) { setStatus(`Could not load image: ${e.message || e}`); }
}

// ---------------------------------------------------------------- UI
function fillSpecies() {
  const sel = $('species');
  for (const [k, v] of Object.entries(ANIMAL_MODELS.species)) {
    const o = document.createElement('option'); o.value = k; o.textContent = v.label; sel.appendChild(o);
  }
  sel.value = state.species;
}
function showInfo() {
  const m = currentModel();
  $('infoTitle').textContent = m.label;
  const facts = [];
  if (m.receptors) facts.push('Receptors simulated (λmax): ' + Object.entries(m.receptors).map(([k, v]) => `${k} ${v} nm`).join(', '));
  if (m.uv_receptors) facts.push('UV receptors NOT simulated: ' + Object.entries(m.uv_receptors).map(([k, v]) => `${k} ${v} nm`).join(', ') +
    ` (a display drives them at ${(m.uv_display_catch_relative * 100).toFixed(1)}% of the visible channels)`);
  if (m.rod_nm) facts.push(`Rod peak ${m.rod_nm} nm (used for the low-light view)`);
  if (m.acuity_cpd) facts.push(`Acuity cut-off ${m.acuity_cpd} cycles/deg (range ${m.acuity_range_cpd.join('–')})` + (m.acuity_dim_cpd ? `; dim light ${m.acuity_dim_cpd}` : ''));
  if (m.interommatidial_deg) facts.push(`Interommatidial angle ${m.interommatidial_deg}°, acceptance angle ${m.acceptance_deg}°`);
  $('infoFacts').innerHTML = facts.map((f) => `<li>${f}</li>`).join('');
  $('infoNotes').textContent = m.notes;
  $('infoRefs').innerHTML = (m.refs || []).map((r) => `<li>${ANIMAL_MODELS.references[r] || r}</li>`).join('');
  const compound = !!m.interommatidial_deg;
  $('hexRow').hidden = !compound; $('edgesRow').hidden = !compound;
}
function bind() {
  fillSpecies(); showInfo();
  const mobile = window.matchMedia('(max-width: 700px)').matches;
  if (!mobile) { state.screenCm = 30; state.distCm = 60; }
  $('screenCm').value = state.screenCm; $('distCm').value = state.distCm;
  $('species').addEventListener('change', (e) => { state.species = e.target.value; showInfo(); state.dirty = true; });
  const slider = (id, key, scale = 1, fmt = (v) => v) => {
    $(id).addEventListener('input', (e) => { state[key] = Number(e.target.value) * scale; $(id + 'Val').textContent = fmt(e.target.value); state.dirty = true; e.target.dataset.userSet = '1'; });
  };
  slider('fov', 'fovDeg'); slider('light', 'light', 0.01); slider('split', 'split', 0.01); slider('blurGain', 'blurGain');
  for (const id of ['screenCm', 'distCm']) $(id).addEventListener('input', (e) => { state[id] = Number(e.target.value) || state[id]; state.dirty = true; });
  const check = (id, key) => $(id).addEventListener('change', (e) => { state[key] = e.target.checked; state.dirty = true; });
  check('trueScale', 'trueScale'); check('hex', 'hex'); check('edges', 'edges');
  $('camBtn').addEventListener('click', startCamera);
  $('flipBtn').addEventListener('click', () => { state.facing = state.facing === 'environment' ? 'user' : 'environment'; if (state.stream) startCamera(); });
  $('stopBtn').addEventListener('click', () => { stopCamera(); setStatus('Camera stopped.'); });
  $('fileInput').addEventListener('change', (e) => { const f = e.target.files && e.target.files[0]; if (f) useImage(URL.createObjectURL(f)); });
  $('sampleBtn').addEventListener('click', () => useImage('assets/img/apples/orig.jpg'));
  $('snapBtn').addEventListener('click', () => {
    const a = document.createElement('a'); a.download = `animal-vision-${state.species}.png`; a.href = canvas.toDataURL('image/png'); a.click();
  });
  window.addEventListener('keydown', (e) => { if (e.key === ' ' && state.source) { e.preventDefault(); state.split = state.split > 0 ? 0 : 0.5; $('split').value = state.split * 100; $('splitVal').textContent = $('split').value; state.dirty = true; } });
}

if (init()) {
  bind();
  requestAnimationFrame(loop);
  useImage('assets/img/apples/orig.jpg');
}

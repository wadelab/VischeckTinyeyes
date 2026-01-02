import { tinyEyesImageData, TINYEYES_AGES } from './tinyeyes.js';

function $(id) {
  const el = document.getElementById(id);
  if (!el) throw new Error(`Missing element: ${id}`);
  return el;
}

function setStatus(msg) {
  $('status').textContent = msg;
}

function clamp(x, lo, hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

function fitSquareSize(naturalW, naturalH, maxSize) {
  const s = Math.min(naturalW, naturalH);
  return Math.min(s, maxSize);
}

async function loadImageFromFile(file) {
  const url = URL.createObjectURL(file);
  try {
    return await loadImageFromUrl(url);
  } finally {
    URL.revokeObjectURL(url);
  }
}

function loadImageFromUrl(url) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.onload = () => resolve(img);
    img.onerror = (e) => reject(e);
    img.src = url;
  });
}

function drawImageToSquareCanvas(img, canvas, maxSize) {
  const nw = img.naturalWidth || img.width;
  const nh = img.naturalHeight || img.height;
  const sideSrc = Math.min(nw, nh);
  const sx = Math.floor((nw - sideSrc) / 2);
  const sy = Math.floor((nh - sideSrc) / 2);
  const sideDst = fitSquareSize(nw, nh, maxSize);

  canvas.width = sideDst;
  canvas.height = sideDst;
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  if (!ctx) throw new Error('Canvas 2D not available');
  ctx.drawImage(img, sx, sy, sideSrc, sideSrc, 0, 0, sideDst, sideDst);
}

function getImageData(canvas) {
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  if (!ctx) throw new Error('Canvas 2D not available');
  return ctx.getImageData(0, 0, canvas.width, canvas.height);
}

function putImageData(canvas, imageData) {
  canvas.width = imageData.width;
  canvas.height = imageData.height;
  const ctx = canvas.getContext('2d');
  if (!ctx) throw new Error('Canvas 2D not available');
  ctx.putImageData(imageData, 0, 0);
}

function downloadCanvas(canvas, filename) {
  const link = document.createElement('a');
  link.download = filename;
  link.href = canvas.toDataURL('image/png');
  link.click();
}

function populateAgeSelect() {
  const sel = $('age');
  sel.innerHTML = '';
  for (const a of TINYEYES_AGES) {
    const opt = document.createElement('option');
    opt.value = a;
    opt.textContent = a;
    sel.appendChild(opt);
  }
  sel.value = 'week12';
}

function getParams() {
  return {
    age: $('age').value,
    widthCm: clamp(Number($('displayWidthCm').value), 1, 500),
    distCm: clamp(Number($('viewDistCm').value), 1, 5000),
    maxSize: clamp(Number($('maxSize').value), 64, 4096),
  };
}

async function loadIntoOriginal(img) {
  const p = getParams();
  drawImageToSquareCanvas(img, $('canvasOriginal'), p.maxSize);
  const w = $('canvasOriginal').width;
  const h = $('canvasOriginal').height;
  const c = $('canvasTinyEyes');
  c.width = w;
  c.height = h;
  setStatus('Image loaded (center-cropped to square).');
}

async function run() {
  const p = getParams();
  const cOrig = $('canvasOriginal');
  if (cOrig.width === 0 || cOrig.height === 0) {
    setStatus('Load an image first.');
    return;
  }

  setStatus('Processing…');
  const t0 = performance.now();

  const original = getImageData(cOrig);
  const out = tinyEyesImageData(original, {
    age: p.age,
    widthCm: p.widthCm,
    distCm: p.distCm,
  });

  putImageData($('canvasTinyEyes'), out);

  const t1 = performance.now();
  setStatus(`Done in ${Math.round(t1 - t0)}ms (${out.width}×${out.height}).`);
}

function reset() {
  for (const id of ['canvasOriginal', 'canvasTinyEyes']) {
    const c = $(id);
    const ctx = c.getContext('2d');
    if (ctx) ctx.clearRect(0, 0, c.width, c.height);
    c.width = 0;
    c.height = 0;
  }
  $('fileInput').value = '';
  $('sampleSelect').value = '';
  setStatus('');
}

function wireUp() {
  populateAgeSelect();

  $('fileInput').addEventListener('change', async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    try {
      setStatus('Loading…');
      const img = await loadImageFromFile(file);
      await loadIntoOriginal(img);
    } catch {
      setStatus('Failed to load image.');
    }
  });

  $('sampleSelect').addEventListener('change', async (e) => {
    const url = e.target.value;
    if (!url) return;
    try {
      setStatus('Loading…');
      const img = await loadImageFromUrl(url);
      await loadIntoOriginal(img);
    } catch {
      setStatus('Failed to load sample.');
    }
  });

  $('maxSize').addEventListener('change', async () => {
    const c = $('canvasOriginal');
    if (c.width === 0 || c.height === 0) return;
    const url = c.toDataURL('image/png');
    const img = await loadImageFromUrl(url);
    await loadIntoOriginal(img);
  });

  $('runBtn').addEventListener('click', () => void run());
  $('resetBtn').addEventListener('click', () => reset());

  $('downloadBtn').addEventListener('click', () => {
    const c = $('canvasTinyEyes');
    if (!c || c.width === 0 || c.height === 0) {
      setStatus('Nothing to download yet.');
      return;
    }
    downloadCanvas(c, 'tinyeyes.png');
  });
}

wireUp();

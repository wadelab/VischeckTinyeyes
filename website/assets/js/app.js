import { brettelLMS, rgbLinToLms, lmsToRgbLin } from './vischeck.js';
import { daltonizeRgbLin } from './daltonize.js';

function $(id) {
  const el = document.getElementById(id);
  if (!el) throw new Error(`Missing element: ${id}`);
  return el;
}

function clamp01(x) {
  return x < 0 ? 0 : (x > 1 ? 1 : x);
}

function srgbToLinear(u8) {
  const x = u8 / 255;
  return x <= 0.04045 ? x / 12.92 : Math.pow((x + 0.055) / 1.055, 2.4);
}

function linearToSrgbByte(x) {
  x = clamp01(x);
  const y = x <= 0.0031308 ? x * 12.92 : 1.055 * Math.pow(x, 1 / 2.4) - 0.055;
  return Math.max(0, Math.min(255, Math.round(y * 255)));
}

function setStatus(msg) {
  $('status').textContent = msg;
}

function updateSliderLabels() {
  $('lmStretchVal').textContent = $('lmStretch').value;
  $('lumScaleVal').textContent = $('lumScale').value;
  $('sScaleVal').textContent = $('sScale').value;
}

function fitSize(w, h, maxSize) {
  const maxDim = Math.max(w, h);
  if (maxDim <= maxSize) return { w, h };
  const scale = maxSize / maxDim;
  return { w: Math.round(w * scale), h: Math.round(h * scale) };
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

function drawImageToCanvas(img, canvas, maxSize) {
  const { w, h } = fitSize(img.naturalWidth || img.width, img.naturalHeight || img.height, maxSize);
  canvas.width = w;
  canvas.height = h;
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  if (!ctx) throw new Error('Canvas 2D not available');
  ctx.drawImage(img, 0, 0, w, h);
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

function imageDataToLinearRgb(imageData) {
  const { data, width, height } = imageData;
  const n = width * height;
  const rgbLin = new Float32Array(n * 3);
  for (let i = 0; i < n; i++) {
    const r = data[i * 4];
    const g = data[i * 4 + 1];
    const b = data[i * 4 + 2];
    rgbLin[i * 3] = srgbToLinear(r);
    rgbLin[i * 3 + 1] = srgbToLinear(g);
    rgbLin[i * 3 + 2] = srgbToLinear(b);
  }
  return rgbLin;
}

function linearRgbToImageData(rgbLin, width, height, alphaFrom) {
  const out = new ImageData(width, height);
  const n = width * height;
  for (let i = 0; i < n; i++) {
    out.data[i * 4] = linearToSrgbByte(rgbLin[i * 3]);
    out.data[i * 4 + 1] = linearToSrgbByte(rgbLin[i * 3 + 1]);
    out.data[i * 4 + 2] = linearToSrgbByte(rgbLin[i * 3 + 2]);
    out.data[i * 4 + 3] = alphaFrom ? alphaFrom.data[i * 4 + 3] : 255;
  }
  return out;
}

function simulateRgbLin(rgbLin, width, height, simType) {
  const n = width * height;
  const out = new Float32Array(rgbLin.length);

  for (let i = 0; i < n; i++) {
    const r = rgbLin[i * 3];
    const g = rgbLin[i * 3 + 1];
    const b = rgbLin[i * 3 + 2];

    const [L, M, S] = rgbLinToLms(r, g, b);
    const [L2, M2, S2] = brettelLMS(L, M, S, simType);
    const [rr, gg, bb] = lmsToRgbLin(L2, M2, S2);

    out[i * 3] = rr;
    out[i * 3 + 1] = gg;
    out[i * 3 + 2] = bb;
  }

  return out;
}

function getParams() {
  return {
    simType: $('simType').value,
    daltonizeEnabled: $('daltonizeEnabled').checked,
    lmStretch: Number($('lmStretch').value),
    lumScale: Number($('lumScale').value),
    sScale: Number($('sScale').value),
    maxSize: Number($('maxSize').value),
  };
}

function downloadCanvas(canvas, filename) {
  const link = document.createElement('a');
  link.download = filename;
  link.href = canvas.toDataURL('image/png');
  link.click();
}

let lastCanvases = {
  original: null,
  sim: null,
  dalt: null,
  daltSim: null,
};

async function run() {
  const params = getParams();
  updateSliderLabels();

  const cOrig = $('canvasOriginal');
  if (cOrig.width === 0 || cOrig.height === 0) {
    setStatus('Load an image first.');
    return;
  }

  const t0 = performance.now();
  setStatus('Processing…');

  const originalImageData = getImageData(cOrig);
  const { width, height } = originalImageData;

  const rgbLin = imageDataToLinearRgb(originalImageData);

  const simLin = simulateRgbLin(rgbLin, width, height, params.simType);
  putImageData($('canvasSim'), linearRgbToImageData(simLin, width, height, originalImageData));

  let daltLin = null;
  let daltSimLin = null;

  if (params.daltonizeEnabled) {
    daltLin = daltonizeRgbLin(rgbLin, width, height, {
      lmStretch: params.lmStretch,
      lumScale: params.lumScale,
      sScale: params.sScale,
    });
    putImageData($('canvasDalt'), linearRgbToImageData(daltLin, width, height, originalImageData));

    daltSimLin = simulateRgbLin(daltLin, width, height, params.simType);
    putImageData($('canvasDaltSim'), linearRgbToImageData(daltSimLin, width, height, originalImageData));
  } else {
    // Clear the daltonize canvases if disabled.
    putImageData($('canvasDalt'), new ImageData(width, height));
    putImageData($('canvasDaltSim'), new ImageData(width, height));
  }

  lastCanvases = {
    original: $('canvasOriginal'),
    sim: $('canvasSim'),
    dalt: $('canvasDalt'),
    daltSim: $('canvasDaltSim'),
  };

  const t1 = performance.now();
  setStatus(`Done in ${Math.round(t1 - t0)}ms (${width}×${height}).`);
}

async function loadIntoOriginal(img) {
  const maxSize = Number($('maxSize').value);
  drawImageToCanvas(img, $('canvasOriginal'), maxSize);
  // Mirror dimensions into other canvases.
  const w = $('canvasOriginal').width;
  const h = $('canvasOriginal').height;
  for (const id of ['canvasSim', 'canvasDalt', 'canvasDaltSim']) {
    const c = $(id);
    c.width = w;
    c.height = h;
  }
  setStatus('Image loaded.');
}

function reset() {
  for (const id of ['canvasOriginal', 'canvasSim', 'canvasDalt', 'canvasDaltSim']) {
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
  updateSliderLabels();

  $('lmStretch').addEventListener('input', updateSliderLabels);
  $('lumScale').addEventListener('input', updateSliderLabels);
  $('sScale').addEventListener('input', updateSliderLabels);

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
    // Reload current original from the last drawn canvas by converting it to an image.
    const c = $('canvasOriginal');
    if (c.width === 0 || c.height === 0) return;
    const url = c.toDataURL('image/png');
    const img = await loadImageFromUrl(url);
    await loadIntoOriginal(img);
  });

  $('runBtn').addEventListener('click', () => {
    void run();
  });

  $('resetBtn').addEventListener('click', () => {
    reset();
  });

  $('downloadBtn').addEventListener('click', () => {
    const which = $('downloadWhich').value;
    const c = lastCanvases[which];
    if (!c || c.width === 0 || c.height === 0) {
      setStatus('Nothing to download yet.');
      return;
    }
    downloadCanvas(c, `vischeck-${which}.png`);
  });
}

wireUp();

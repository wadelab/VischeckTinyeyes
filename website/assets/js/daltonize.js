// Daltonize-style correction based on the original Vischeck C++ approach in opponent space.
// This implementation computes opponent-space statistics (means/variances) and builds
// a simple per-pixel correction that boosts the LM opponent channel and projects it
// into other channels.

import { RGB_TO_LMS, LMS_TO_RGB, rgbLinToLms, lmsToRgbLin } from './vischeck.js';

// LMS <-> Opponent matrices (same structure used in the TinyEyes transform).
const LMS_TO_OPP = [
  0.5, 0.5, 0.0,
  -0.6690, 0.7420, -0.0270,
  -0.2120, -0.3540, 0.9110,
];

function invert3x3(m) {
  const a = m[0], b = m[1], c = m[2];
  const d = m[3], e = m[4], f = m[5];
  const g = m[6], h = m[7], i = m[8];

  const A = (e * i - f * h);
  const B = -(d * i - f * g);
  const C = (d * h - e * g);
  const D = -(b * i - c * h);
  const E = (a * i - c * g);
  const F = -(a * h - b * g);
  const G = (b * f - c * e);
  const H = -(a * f - c * d);
  const I = (a * e - b * d);

  const det = a * A + b * B + c * C;
  if (Math.abs(det) < 1e-12) {
    throw new Error('Non-invertible 3x3 matrix');
  }

  const invDet = 1 / det;
  return [
    A * invDet, D * invDet, G * invDet,
    B * invDet, E * invDet, H * invDet,
    C * invDet, F * invDet, I * invDet,
  ];
}

const OPP_TO_LMS = invert3x3(LMS_TO_OPP);

function mulMat3Vec3(m, x, y, z) {
  return [
    m[0] * x + m[1] * y + m[2] * z,
    m[3] * x + m[4] * y + m[5] * z,
    m[6] * x + m[7] * y + m[8] * z,
  ];
}

function rgbLinToOpp(r, g, b) {
  const [L, M, S] = rgbLinToLms(r, g, b, RGB_TO_LMS);
  return mulMat3Vec3(LMS_TO_OPP, L, M, S);
}

function oppToRgbLin(o0, o1, o2) {
  const [L, M, S] = mulMat3Vec3(OPP_TO_LMS, o0, o1, o2);
  return lmsToRgbLin(L, M, S, LMS_TO_RGB);
}

/**
 * Apply Daltonize correction to a linear-RGB image.
 *
 * @param {Float32Array} rgbLin  length = width*height*3
 * @param {number} width
 * @param {number} height
 * @param {{lmStretch:number, lumScale:number, sScale:number}} params 0..100 UI values
 * @returns {Float32Array} corrected linear-RGB
 */
export function daltonizeRgbLin(rgbLin, width, height, params) {
  const n = width * height;

  const lmStretch01 = (params.lmStretch ?? 50) / 100;
  const lumScale01 = (params.lumScale ?? 50) / 100;
  const sScale01 = (params.sScale ?? 50) / 100;

  // Make the top end of the sliders stronger without over-amplifying low values.
  // x in [0,1] -> x*(0.6 + 1.0*x) in [0,1.6]
  const lumScaleEff = lumScale01 * (0.6 + 1.0 * lumScale01);
  const sScaleEff = sScale01 * (0.6 + 1.0 * sScale01);

  // Compute means and variances in opponent space.
  let sum0 = 0, sum1 = 0, sum2 = 0;
  let sumSq0 = 0, sumSq1 = 0, sumSq2 = 0;

  for (let i = 0; i < n; i++) {
    const r = rgbLin[i * 3];
    const g = rgbLin[i * 3 + 1];
    const b = rgbLin[i * 3 + 2];
    const [o0, o1, o2] = rgbLinToOpp(r, g, b);

    sum0 += o0; sum1 += o1; sum2 += o2;
    sumSq0 += o0 * o0;
    sumSq1 += o1 * o1;
    sumSq2 += o2 * o2;
  }

  const mean0 = sum0 / n;
  const mean1 = sum1 / n;
  const mean2 = sum2 / n;

  const var0 = Math.max(0, (sumSq0 / n) - mean0 * mean0);
  const var1 = Math.max(0, (sumSq1 / n) - mean1 * mean1);
  const var2 = Math.max(0, (sumSq2 / n) - mean2 * mean2);

  // Normalization:
  // The original Vischeck Daltonize uses opponent-space statistics to determine
  // how much LM contrast to project into luminance (L+M) and S.
  // A naive inverse-variance scaling can be extremely image-dependent: images
  // with low luminance variance (e.g. some FlCells stimuli) can become overly
  // sensitive, while high-variance images can look unchanged.
  //
  // Here we scale by standard deviation ratios so the injected component is
  // proportional to existing channel contrast, making results more consistent
  // across images.
  const EPS = 1e-6;
  const sigma0 = Math.sqrt(var0 + EPS);
  const sigma1 = Math.sqrt(var1 + EPS);
  const sigma2 = Math.sqrt(var2 + EPS);

  // Small additional gain so the default web effect is visible.
  const GAIN = 1.15;
  const lmStretchScaled = lmStretch01 * 2.0 + 1.0;
  const lmScale = (lmStretchScaled - 1) / 4 + 1;

  // Project LM contrast (o1) into L+M (o0) and S (o2).
  // Coefficients are in opponent units; clamp to avoid pathological extremes.
  const LUM_PROJECT = 0.9;
  const S_PROJECT = 0.6;
  const amountToLM = clampAbs(GAIN * -lumScaleEff * LUM_PROJECT * (sigma0 / sigma1), 3.5);
  const amountToS = clampAbs(GAIN * -sScaleEff * S_PROJECT * (sigma2 / sigma1), 3.5);

  const out = new Float32Array(rgbLin.length);

  for (let i = 0; i < n; i++) {
    const r = rgbLin[i * 3];
    const g = rgbLin[i * 3 + 1];
    const b = rgbLin[i * 3 + 2];

    let [o0, o1, o2] = rgbLinToOpp(r, g, b);

    // Mean-center the LM axis before stretching/projection, then add mean back.
    // The original Vischeck Daltonize does three things in opponent space:
    // 1) stretch the LM contrast, and
    // 2) add LM contrast into the L+M axis, and
    // 3) add LM contrast into the S axis.
    const o1c = o1 - mean1;
    const o0n = o0 + amountToLM * o1c;
    const o1n = lmScale * o1c + mean1;
    const o2n = o2 + amountToS * o1c;

    const [rr, gg, bb] = oppToRgbLin(o0n, o1n, o2n);

    out[i * 3] = rr;
    out[i * 3 + 1] = gg;
    out[i * 3 + 2] = bb;
  }

  return out;
}

function clampAbs(x, maxAbs) {
  if (x > maxAbs) return maxAbs;
  if (x < -maxAbs) return -maxAbs;
  return x;
}

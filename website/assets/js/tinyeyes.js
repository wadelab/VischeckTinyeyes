// TinyEyes infant-vision simulation (opponent-space blur), ported from
// pytorch_implementation/TinyEyesTransform.py (CPU path).

// Matrices from TinyEyesTransform.py
const RGB_TO_LMS = [
  0.05059983, 0.08585369, 0.00952420,
  0.01893033, 0.08925308, 0.01370054,
  0.00292202, 0.00975732, 0.07145979,
];

const LMS_TO_OPP = [
  0.5, 0.5, 0.0,
  -0.6690, 0.7420, -0.0270,
  -0.2120, -0.3540, 0.9110,
];

// np.linalg.pinv(RGB_TO_LMS)
const LMS_TO_RGB = [
  30.83086094, -29.83266362, 1.61047654,
  -6.48147088, 17.71557918, -2.53264355,
  -0.37568830, -1.19906503, 14.27384504,
];

// np.linalg.inv(LMS_TO_OPP)
const OPP_TO_LMS = [
  1.0399668536, -0.7108374227, -0.0210676294,
  0.9600331464, 0.7108374227, 0.0210676294,
  0.6150655398, 0.1108001252, 1.1009787084,
];

const MODEL_BY_AGE = {
  week0: [0.6821, 100, 1000],
  week4: [0.48, 4.77, 100],
  week8: [0.24, 2.4, 4],
  week12: [0.1, 0.53, 2],
  week24: [0.04, 0.12, 1],
  adult: [0.01, 0.015, 0.02],
};

function clamp(x, lo, hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

function mulMat3Vec3(m, x, y, z) {
  return [
    m[0] * x + m[1] * y + m[2] * z,
    m[3] * x + m[4] * y + m[5] * z,
    m[6] * x + m[7] * y + m[8] * z,
  ];
}

function calcPixelsPerDegree(pxSize, widthCm, distCm) {
  const ratio = (widthCm / 2) / distCm;
  const tDeg = 2 * (Math.atan(ratio) * 180 / Math.PI);
  return pxSize / (tDeg || 1e-12);
}

function gaussianKernel1d(sigma) {
  const radius = Math.max(1, Math.ceil(3 * sigma));
  const size = radius * 2 + 1;
  const k = new Float32Array(size);
  const s2 = sigma * sigma;
  let sum = 0;
  for (let i = -radius; i <= radius; i++) {
    const v = Math.exp(-(i * i) / (2 * s2));
    k[i + radius] = v;
    sum += v;
  }
  for (let i = 0; i < size; i++) k[i] /= sum;
  return { k, radius };
}

function blurSeparable(src, width, height, sigma) {
  if (sigma <= 0.01) return src;

  const maxDim = Math.min(width, height);
  // For extremely large sigmas, the output approaches the global mean.
  if (sigma >= maxDim * 0.25) {
    let mean = 0;
    for (let i = 0; i < src.length; i++) mean += src[i];
    mean /= src.length;
    const out = new Float32Array(src.length);
    out.fill(mean);
    return out;
  }

  const { k, radius } = gaussianKernel1d(sigma);
  const tmp = new Float32Array(src.length);
  const out = new Float32Array(src.length);

  // Horizontal
  for (let y = 0; y < height; y++) {
    const row = y * width;
    for (let x = 0; x < width; x++) {
      let acc = 0;
      for (let j = -radius; j <= radius; j++) {
        const xx = clamp(x + j, 0, width - 1);
        acc += k[j + radius] * src[row + xx];
      }
      tmp[row + x] = acc;
    }
  }

  // Vertical
  for (let y = 0; y < height; y++) {
    const row = y * width;
    for (let x = 0; x < width; x++) {
      let acc = 0;
      for (let j = -radius; j <= radius; j++) {
        const yy = clamp(y + j, 0, height - 1);
        acc += k[j + radius] * tmp[yy * width + x];
      }
      out[row + x] = acc;
    }
  }

  return out;
}

/**
 * TinyEyes transform.
 * Input/output are in sRGB byte space (0..255), matching the Python CPU path.
 *
 * @param {ImageData} imageData square image
 * @param {{age:'week0'|'week4'|'week8'|'week12'|'week24'|'adult', widthCm:number, distCm:number}} params
 * @returns {ImageData}
 */
export function tinyEyesImageData(imageData, params) {
  const { width, height, data } = imageData;
  if (width !== height) throw new Error('TinyEyes requires a square image');

  const age = params.age;
  const model = MODEL_BY_AGE[age];
  if (!model) throw new Error(`Unknown age: ${age}`);

  const widthCm = params.widthCm;
  const distCm = params.distCm;
  const ppd = calcPixelsPerDegree(width, widthCm, distCm);

  const sigmaOpp = [
    model[0] * ppd,
    model[1] * ppd,
    model[2] * ppd,
  ];

  const n = width * height;

  // Flatten RGB to 3 x N and normalize to [-1,1] using (v-128)/128.
  const rgbN = new Float32Array(n * 3);
  for (let i = 0; i < n; i++) {
    rgbN[i * 3] = (data[i * 4] - 128) / 128;
    rgbN[i * 3 + 1] = (data[i * 4 + 1] - 128) / 128;
    rgbN[i * 3 + 2] = (data[i * 4 + 2] - 128) / 128;
  }

  // Convert to opponent space.
  const opp0 = new Float32Array(n);
  const opp1 = new Float32Array(n);
  const opp2 = new Float32Array(n);

  for (let i = 0; i < n; i++) {
    const r = rgbN[i * 3];
    const g = rgbN[i * 3 + 1];
    const b = rgbN[i * 3 + 2];

    const [L, M, S] = mulMat3Vec3(RGB_TO_LMS, r, g, b);
    const [o0, o1, o2] = mulMat3Vec3(LMS_TO_OPP, L, M, S);
    opp0[i] = o0;
    opp1[i] = o1;
    opp2[i] = o2;
  }

  const b0 = blurSeparable(opp0, width, height, sigmaOpp[0]);
  const b1 = blurSeparable(opp1, width, height, sigmaOpp[1]);
  const b2 = blurSeparable(opp2, width, height, sigmaOpp[2]);

  const out = new ImageData(width, height);
  for (let i = 0; i < n; i++) {
    const [L, M, S] = mulMat3Vec3(OPP_TO_LMS, b0[i], b1[i], b2[i]);
    const [rr, gg, bb] = mulMat3Vec3(LMS_TO_RGB, L, M, S);

    out.data[i * 4] = clamp(Math.round(rr * 128 + 128), 0, 255);
    out.data[i * 4 + 1] = clamp(Math.round(gg * 128 + 128), 0, 255);
    out.data[i * 4 + 2] = clamp(Math.round(bb * 128 + 128), 0, 255);
    out.data[i * 4 + 3] = data[i * 4 + 3];
  }

  return out;
}

export const TINYEYES_AGES = Object.freeze(['week0', 'week4', 'week8', 'week12', 'week24', 'adult']);

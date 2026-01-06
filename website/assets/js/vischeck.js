// Brettel/Vienot/Mollon color vision deficiency simulation in LMS space.
// Ported from the original Vischeck C++ implementation (img::brettelTransform).

/**
 * RGB<->LMS matrices.
 * These are commonly used sRGB(D65) -> LMS (Hunt-Pointer-Estevez-like) matrices.
 * The important Vischeck-specific part is the Brettel piecewise projection in LMS.
 */
export const RGB_TO_LMS = [
  0.31399022, 0.63951294, 0.04649755,
  0.15537241, 0.75789446, 0.08670142,
  0.01775239, 0.10944209, 0.87256922,
];

export const LMS_TO_RGB = [
  5.47221206, -4.6419601, 0.16963708,
  -1.1252419, 2.29317094, -0.1678952,
  0.02980165, -0.19318073, 1.16364789,
];

function mulMat3Vec3(m, x, y, z) {
  return [
    m[0] * x + m[1] * y + m[2] * z,
    m[3] * x + m[4] * y + m[5] * z,
    m[6] * x + m[7] * y + m[8] * z,
  ];
}

function computeEqualEnergyAnchor(rgb2lms) {
  // LMS for RGB=(1,1,1)
  return [
    rgb2lms[0] + rgb2lms[1] + rgb2lms[2],
    rgb2lms[3] + rgb2lms[4] + rgb2lms[5],
    rgb2lms[6] + rgb2lms[7] + rgb2lms[8],
  ];
}

/**
 * Apply the Brettel piecewise transform to a single LMS triplet.
 * @param {number} L
 * @param {number} M
 * @param {number} S
 * @param {'normal'|'protanope'|'deuteranope'|'tritanope'} type
 * @param {number[]} rgb2lms 3x3 row-major
 * @returns {[number, number, number]}
 */
export function brettelLMS(L, M, S, type, rgb2lms = RGB_TO_LMS) {
  if (type === 'normal') return [L, M, S];

  // Anchors from Brettel/Vienot/Mollon JOSA 14/10 1997.
  // LMS for lambda = 475,485,575,660
  const anchor = [
    0.08008, 0.1579, 0.5897,
    0.1284, 0.2237, 0.3636,
    0.9856, 0.7325, 0.001079,
    0.0914, 0.007009, 0.0,
  ];

  const e = computeEqualEnergyAnchor(rgb2lms);

  let a1, b1, c1, a2, b2, c2, inflectionVal, tmp;

  if (type === 'deuteranope') {
    a1 = e[1] * anchor[8] - e[2] * anchor[7];
    b1 = e[2] * anchor[6] - e[0] * anchor[8];
    c1 = e[0] * anchor[7] - e[1] * anchor[6];

    a2 = e[1] * anchor[2] - e[2] * anchor[1];
    b2 = e[2] * anchor[0] - e[0] * anchor[2];
    c2 = e[0] * anchor[1] - e[1] * anchor[0];

    inflectionVal = e[2] / e[0];
    tmp = S / (L || 1e-12);

    const M2 = tmp < inflectionVal
      ? (-(a1 * L + c1 * S) / b1)
      : (-(a2 * L + c2 * S) / b2);

    return [L, M2, S];
  }

  if (type === 'protanope') {
    a1 = e[1] * anchor[8] - e[2] * anchor[7];
    b1 = e[2] * anchor[6] - e[0] * anchor[8];
    c1 = e[0] * anchor[7] - e[1] * anchor[6];

    a2 = e[1] * anchor[2] - e[2] * anchor[1];
    b2 = e[2] * anchor[0] - e[0] * anchor[2];
    c2 = e[0] * anchor[1] - e[1] * anchor[0];

    inflectionVal = e[2] / e[1];
    tmp = S / (M || 1e-12);

    const L2 = tmp < inflectionVal
      ? (-(b1 * M + c1 * S) / a1)
      : (-(b2 * M + c2 * S) / a2);

    return [L2, M, S];
  }

  if (type === 'tritanope') {
    a1 = e[1] * anchor[11] - e[2] * anchor[10];
    b1 = e[2] * anchor[9] - e[0] * anchor[11];
    c1 = e[0] * anchor[10] - e[1] * anchor[9];

    a2 = e[1] * anchor[5] - e[2] * anchor[4];
    b2 = e[2] * anchor[3] - e[0] * anchor[5];
    c2 = e[0] * anchor[4] - e[1] * anchor[3];

    inflectionVal = e[1] / e[0];
    tmp = M / (L || 1e-12);

    const S2 = tmp < inflectionVal
      ? (-(a1 * L + b1 * M) / c1)
      : (-(a2 * L + b2 * M) / c2);

    return [L, M, S2];
  }

  return [L, M, S];
}

export function rgbLinToLms(r, g, b, rgb2lms = RGB_TO_LMS) {
  return mulMat3Vec3(rgb2lms, r, g, b);
}

export function lmsToRgbLin(L, M, S, lms2rgb = LMS_TO_RGB) {
  return mulMat3Vec3(lms2rgb, L, M, S);
}

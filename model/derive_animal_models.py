#!/usr/bin/env python3
"""Derive animal-vision colour transforms for the Vischeck animal simulator.

Pipeline (all linear, per pixel):

  camera / sRGB pixel  ->  "stimulus" = that colour shown on a measured display
                       ->  animal photoreceptor excitations (2 or 3 channels)
                       ->  a human-visible rendering that the ANIMAL would find
                           indistinguishable from the original (metamer preserving)

The rendering step is the same idea as Brettel, Vienot & Mollon (1997): the
animal's colour space has fewer dimensions than ours, so we choose a canonical
2-D surface of display colours (here: the plane spanned by display white and
display blue) and project every colour onto it along the animal's confusion
lines.  Formally, with M (n_animal x 3) mapping linear RGB to animal receptor
excitations and B = [white, blue] (3 x 2):

      T = B @ inv(M @ B) @ M          (3 x 3, idempotent)

and M @ T == M, i.e. the animal cannot tell T(rgb) from rgb.

Photoreceptor spectral sensitivities use the Govardovskii et al. (2000) A1
visual-pigment template (alpha + beta bands) at the species' published
lambda-max values.  Ocular-media transmission is ignored (see docs).

Inputs (from the Vischeck repo, `displays/`):
  SPconeSpectra.txt   370-730 nm human cone fundamentals (L, M, S)
  chesnutSpectra.txt  370-730 nm spectral power of a measured display (R, G, B)

Outputs:
  animal_models.json  all matrices + parameters + references
  animal_models.js    the same as an ES module / global for the web app
"""
from __future__ import annotations

import argparse
import json
import os
import sys

import numpy as np

# --------------------------------------------------------------------------
# Govardovskii et al. 2000, Vis Neurosci 17:509-528, A1 template
# --------------------------------------------------------------------------

def govardovskii_a1(wl_nm: np.ndarray, lmax: float) -> np.ndarray:
    """Normalised absorbance of an A1 visual pigment with peak at lmax (nm)."""
    x = lmax / wl_nm
    A, B, C, D = 69.7, 28.0, -14.9, 0.674
    a = 0.8795 + 0.0459 * np.exp(-((lmax - 300.0) ** 2) / 11940.0)
    b, c = 0.922, 1.104
    alpha = 1.0 / (np.exp(A * (a - x)) + np.exp(B * (b - x)) + np.exp(C * (c - x)) + D)
    # beta band
    lmb = 189.0 + 0.315 * lmax
    bb = -40.5 + 0.195 * lmax
    A_beta = 0.26
    beta = A_beta * np.exp(-(((wl_nm - lmb) / bb) ** 2))
    s = alpha + beta
    return s / s.max()


# --------------------------------------------------------------------------
# Species definitions.  Every number here has a citation in docs/ANIMAL_VISION.md
# --------------------------------------------------------------------------

SPECIES = {
    "human": {
        "label": "Human (reference)",
        "kind": "trichromat",
        "receptors": {"L": 560.0, "M": 530.0, "S": 420.0},   # only used for labelling; matrix is identity
        "rod": 498.0,
        "acuity_cpd": 50.0,
        "acuity_range_cpd": [30.0, 60.0],
        "acuity_dim_cpd": 8.0,          # 5.9-9.9 cpd at 0.0087 cd/m2 (Lind et al. 2017)
        "refs": ["lind2017"],
        "notes": "Reference view. No colour transform. Dim-light acuity from the human comparison group in Lind et al. 2017.",
    },
    "dog": {
        "label": "Dog",
        "kind": "dichromat",
        "receptors": {"S": 429.0, "L": 555.0},
        "rod": 508.0,
        # Cut-off spatial frequency (cycles/deg) for the acuity blur.  Snellen
        # 20/75 (Miller & Murphy 1995) ~ 8 cpd; pattern ERG/VEP ~ 12 cpd (Odom
        # et al. 1983); behavioural 5.5-19.5 cpd (Lind et al. 2017).
        "acuity_cpd": 8.0,
        "acuity_range_cpd": [5.5, 19.5],
        "acuity_dim_cpd": 2.6,          # 1.8-3.5 cpd at 0.0087 cd/m2 (Lind et al. 2017)
        "refs": ["neitz1989", "jacobs1993", "miller1995", "odom1983", "lind2017", "mowat2008"],
        "notes": "Cone peaks 429/555 nm (Neitz, Geist & Jacobs 1989; Jacobs et al. 1993). Rod ~508 nm (Jacobs et al. 1993). Rod-dominated retina with tapetum; bright-light acuity roughly 1/3 of a human's.",
    },
    "cat": {
        "label": "Cat",
        "kind": "dichromat",
        "receptors": {"S": 450.0, "L": 556.0},
        "rod": 500.0,
        "acuity_cpd": 8.5,
        "acuity_range_cpd": [6.0, 9.0],
        "refs": ["guenther1993", "jacobson1976"],
        "notes": "Cone peaks 450/556 nm (Guenther & Zrenner 1993). Behavioural grating acuity 8-9 cpd (Jacobson, Franklin & McDonald 1976). Rod peak 500 nm is a generic mammalian rhodopsin value, not a cat-specific measurement.",
    },
    "fruitfly": {
        "label": "Fruit fly (Drosophila)",
        "kind": "compound",
        # Only the display-visible channels can be driven by an RGB camera.
        # R7 (Rh3 331 nm, Rh4 355 nm) are UV and receive ~nothing from a display.
        # In vivo, screening pigments shift the R8y (Rh6) sensitivity peak from
        # ~508-515 nm (isolated pigment, Salcedo et al. 1999) to ~600 nm
        # (Sharkey et al. 2020).  We use the in-vivo value because the
        # simulation is about what the photoreceptor actually catches.
        "receptors": {"R8p_Rh5": 442.0, "R8y_Rh6_invivo": 600.0},
        "achromatic": {"R1-6_Rh1": 478.0},
        "uv_receptors": {"R7p_Rh3": 331.0, "R7y_Rh4": 355.0},
        "interommatidial_deg": 4.6,
        "acceptance_deg": 5.0,
        "refs": ["salcedo1999", "sharkey2020", "gonzalezbellido2011"],
        "notes": "Rh5 442 nm and Rh1 478 nm from Salcedo et al. 1999; R8y taken at its in-vivo peak of ~600 nm (Sharkey et al. 2020), which is red-shifted from the isolated Rh6 pigment. Interommatidial angle ~4.5-4.6 deg, acceptance angle ~1.1x that (Gonzalez-Bellido et al. 2011). The UV receptors (R7) receive essentially nothing from a display or an RGB camera and are not simulated.",
    },
    "blowfly": {
        "label": "Blowfly (Calliphora)",
        "kind": "compound",
        "receptors": {"R8p": 442.0, "R8y": 600.0},
        "achromatic": {"R1-6": 478.0},
        "uv_receptors": {"R7p": 331.0, "R7y": 355.0},
        "interommatidial_deg": 1.5,
        "acceptance_deg": 1.2,
        "refs": ["land1997"],
        "notes": "Opsin peaks borrowed from Drosophila (not Calliphora-specific). Acceptance angle ~1.0-1.24 deg; interommatidial angle ~1.5 deg is approximate (Land 1997) and should be checked against the primary source.",
    },
    "honeybee": {
        "label": "Honeybee",
        "kind": "compound",
        "receptors": {"M_blue": 436.0, "L_green": 544.0},
        "uv_receptors": {"S_UV": 344.0},
        "interommatidial_deg": None,
        "acceptance_deg": None,
        "refs": ["peitsch1992"],
        "notes": "Trichromat with UV/blue/green receptors at 344/436/544 nm (Peitsch et al. 1992). The UV channel cannot be driven by an RGB camera, so only the blue/green pair is simulated. Spatial sampling not implemented.",
    },
}

REFERENCES = {
    "neitz1989": "Neitz J, Geist T, Jacobs GH (1989). Color vision in the dog. Visual Neuroscience 3(2):119-125. PMID 2487095.",
    "jacobs1993": "Jacobs GH, Deegan JF, Crognale MA, Fenwick JA (1993). Photopigments of dogs and foxes and their implications for canid vision. Visual Neuroscience 10(1):173-180. PMID 8424924.",
    "miller1995": "Miller PE, Murphy CJ (1995). Vision in dogs. J Am Vet Med Assoc 207(12):1623-1634. PMID 7493905.",
    "odom1983": "Odom JV, Bromberg NM, Dawson WW (1983). Canine visual acuity: retinal and cortical field potentials evoked by pattern stimulation. Am J Physiol 245(5):R637-R641. PMID 6638211.",
    "lind2017": "Lind O, Milton I, Andersson E, Jensen P, Roth LSV (2017). High visual acuity revealed in dogs. PLoS ONE 12(12):e0188557.",
    "mowat2008": "Mowat FM et al. (2008). Topographical characterization of cone photoreceptors and the area centralis of the canine retina. Molecular Vision 14:2518-2527. PMID 19112529.",
    "coile1989": "Coile DC, Pollitz CH, Smith JC (1989). Behavioral determination of critical flicker fusion in dogs. Physiology & Behavior 45(6):1087-1092. PMID 2813532.",
    "guenther1993": "Guenther E, Zrenner E (1993). The spectral sensitivity of dark- and light-adapted cat retinal ganglion cells. J Neurosci 13(4):1543-1550.",
    "jacobson1976": "Jacobson SG, Franklin KB, McDonald WI (1976). Visual acuity of the cat. Vision Research 16(10):1141-1143.",
    "salcedo1999": "Salcedo E et al. (1999). Blue- and green-absorbing visual pigments of Drosophila: ectopic expression and physiological characterization of the R8 photoreceptor cell-specific Rh5 and Rh6 rhodopsins. J Neurosci 19(24):10716-10726. PMID 10594055.",
    "sharkey2020": "Sharkey CR, Blanco J, Leibowitz MM, Pinto-Benito D, Wardill TJ (2020). The spectral sensitivity of Drosophila photoreceptors. Scientific Reports 10:18242.",
    "gonzalezbellido2011": "Gonzalez-Bellido PT, Wardill TJ, Juusola M (2011). Compound eyes and retinal information processing in miniature dipteran species match their specific ecological demands. PNAS 108(10):4224-4229.",
    "land1997": "Land MF (1997). Visual acuity in insects. Annual Review of Entomology 42:147-177. PMID 15012311.",
    "peitsch1992": "Peitsch D et al. (1992). The spectral input systems of hymenopteran insects and their receptor-based colour vision. J Comp Physiol A 170:23-40.",
    "brettel1997": "Brettel H, Vienot F, Mollon JD (1997). Computerized simulation of color appearance for dichromats. J Opt Soc Am A 14(10):2647-2655. PMID 9316278.",
    "govardovskii2000": "Govardovskii VI, Fyhrquist N, Reuter T, Kuzmin DG, Donner K (2000). In search of the visual pigment template. Visual Neuroscience 17(4):509-528. PMID 11016572.",
    "kelber2003": "Kelber A, Vorobyev M, Osorio D (2003). Animal colour vision - behavioural tests and physiological concepts. Biological Reviews 78(1):81-118. PMID 12620062.",
}

HUMAN_ACUITY_CPD = 50.0   # nominal foveal cut-off used for the "extra blur" calculation
MTF_AT_CUTOFF = 0.1       # Gaussian MTF value we equate with "acuity cut-off"


def gaussian_sigma_deg(cutoff_cpd: float) -> float:
    """sigma (deg) of a Gaussian PSF whose MTF equals MTF_AT_CUTOFF at cutoff_cpd."""
    return np.sqrt(2.0 * np.log(1.0 / MTF_AT_CUTOFF)) / (2.0 * np.pi * cutoff_cpd)


def load_spectra(displays_dir: str):
    cones = np.loadtxt(os.path.join(displays_dir, "SPconeSpectra.txt"))
    disp = np.loadtxt(os.path.join(displays_dir, "chesnutSpectra.txt"))
    assert np.allclose(cones[:, 0], disp[:, 0]), "wavelength grids differ"
    wl = cones[:, 0]
    return wl, cones[:, 1:4], disp[:, 1:4]


def projection(M: np.ndarray, basis: np.ndarray) -> np.ndarray:
    """T = B inv(M B) M ; M (n x 3), B (3 x n)."""
    MB = M @ basis
    T = basis @ np.linalg.inv(MB) @ M
    assert np.allclose(M @ T, M, atol=1e-9)
    return T


def build(displays_dir: str):
    wl, human_lms, prim = load_spectra(displays_dir)
    # Human LMS for the display primaries, normalised so white has unit L+M.
    rgb2lms = human_lms.T @ prim                      # 3x3
    rgb2lms /= (rgb2lms[0].sum() + rgb2lms[1].sum())

    basis = np.array([[1.0, 0.0], [1.0, 0.0], [1.0, 1.0]])   # columns: white, blue

    out = {"meta": {
        "generated_by": "model/derive_animal_models.py",
        "display": "chesnutSpectra.txt (measured display primaries, 370-730 nm, Vischeck repo)",
        "human_cones": "SPconeSpectra.txt (Vischeck repo)",
        "template": "Govardovskii et al. 2000 A1 (alpha+beta)",
        "colour_space": "matrices act on LINEAR sRGB (decode gamma first, re-encode after)",
        "mtf_at_cutoff": MTF_AT_CUTOFF,
        "human_acuity_cpd": HUMAN_ACUITY_CPD,
    }, "species": {}, "references": REFERENCES}

    for key, sp in SPECIES.items():
        rec_names = list(sp["receptors"].keys())
        if key == "human":
            sens = human_lms.T                                   # measured fundamentals, not template
        else:
            sens = np.stack([govardovskii_a1(wl, sp["receptors"][n]) for n in rec_names])  # n x 361
        M = sens @ prim                                          # n x 3 (rgb -> receptor)
        # normalise each receptor so display white gives 1.0
        M = M / M.sum(axis=1, keepdims=True)
        n = M.shape[0]
        if n == 2:
            T = projection(M, basis)
        elif n == 3:
            T = np.eye(3)
        else:
            raise ValueError(n)
        entry = {
            "label": sp["label"], "kind": sp["kind"], "receptors": sp["receptors"],
            "rgb_to_receptors": np.round(M, 6).tolist(),
            "sim_matrix": np.round(T, 6).tolist(),
            "notes": sp["notes"], "refs": sp["refs"],
        }
        if "uv_receptors" in sp:
            uv = np.stack([govardovskii_a1(wl, v) for v in sp["uv_receptors"].values()]) @ prim
            entry["uv_receptors"] = sp["uv_receptors"]
            # how much of the display's light each UV receptor actually catches,
            # relative to the visible receptors (should be ~0: proves the point)
            entry["uv_display_catch_relative"] = float(np.round(uv.sum() / (sens @ prim).sum() * n, 4))
        if "achromatic" in sp:
            ach = np.stack([govardovskii_a1(wl, v) for v in sp["achromatic"].values()]) @ prim
            ach = ach / ach.sum()
            entry["achromatic_weights"] = np.round(ach[0], 6).tolist()
        if sp.get("rod"):
            rod = govardovskii_a1(wl, sp["rod"]) @ prim
            entry["rod_nm"] = sp["rod"]
            entry["rod_weights"] = np.round(rod / rod.sum(), 6).tolist()
        if sp.get("acuity_cpd"):
            s_a = gaussian_sigma_deg(sp["acuity_cpd"])
            s_h = gaussian_sigma_deg(HUMAN_ACUITY_CPD)
            entry["acuity_cpd"] = sp["acuity_cpd"]
            entry["acuity_range_cpd"] = sp["acuity_range_cpd"]
            # blur to ADD to a (nominally sharp) camera image, in scene degrees
            entry["blur_sigma_deg"] = float(np.round(np.sqrt(max(s_a**2 - s_h**2, 0.0)), 5))
            if sp.get("acuity_dim_cpd"):
                s_d = gaussian_sigma_deg(sp["acuity_dim_cpd"])
                entry["acuity_dim_cpd"] = sp["acuity_dim_cpd"]
                entry["blur_sigma_dim_deg"] = float(np.round(np.sqrt(max(s_d**2 - s_h**2, 0.0)), 5))
        if sp.get("interommatidial_deg"):
            entry["interommatidial_deg"] = sp["interommatidial_deg"]
            entry["acceptance_deg"] = sp["acceptance_deg"]
            # Gaussian acceptance function: FWHM = acceptance angle
            entry["blur_sigma_deg"] = float(np.round(sp["acceptance_deg"] / 2.3548, 5))
        out["species"][key] = entry

    # human reference matrices (for completeness / sanity)
    out["human"] = {"rgb_to_lms": np.round(rgb2lms, 6).tolist(),
                    "acuity_cpd": HUMAN_ACUITY_CPD}
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--displays", default=os.path.join(os.path.dirname(__file__), "..", "displays"))
    ap.add_argument("--out", default=os.path.dirname(__file__))
    args = ap.parse_args()
    data = build(args.displays)
    with open(os.path.join(args.out, "animal_models.json"), "w") as f:
        json.dump(data, f, indent=1)
    js = "// GENERATED by model/derive_animal_models.py - do not edit by hand.\n"
    js += "export const ANIMAL_MODELS = " + json.dumps(data, indent=1) + ";\n"
    with open(os.path.join(args.out, "animal_models.js"), "w") as f:
        f.write(js)
    for k, v in data["species"].items():
        print(k, "sim_matrix=", np.round(np.array(v["sim_matrix"]), 3).tolist(),
              "blur_sigma_deg=", v.get("blur_sigma_deg"),
              "uv_catch=", v.get("uv_display_catch_relative"))


if __name__ == "__main__":
    main()

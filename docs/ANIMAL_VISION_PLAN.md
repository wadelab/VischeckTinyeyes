# Animal Vision Simulator: plan

*Drafted 2026-09-07 for vischeck.com. Status: working web prototype on the `animal-vision` branch (`website/animal.html`), model derivation in `model/`, nothing deployed yet.*

## 1. Summary and honest assessment

**The idea is not new.** There are at least half a dozen live-camera "animal vision" phone apps (Animal Vision Simulator and CatLens on Google Play; Animal Eyes and Pawspective on the App Store; tembrica.com's online simulator; the Dog Vision Simulator Chrome extension) plus András Péter's still-image Dog Vision tool, which most of them copy. All of them claim to be "science-based"; none that I could see publish their transforms, their parameter sources, or handle viewing geometry correctly.

**What Vischeck can add is rigour, not novelty.** The defensible differentiators are:

1. A published, reproducible method in the Brettel/Viénot/Mollon lineage: every pixel is treated as a colour on a measured display, receptor catches are computed from the published spectral peaks with the Govardovskii template, and the rendering is *metamer-preserving* (the animal could not tell the simulation from the original). Same logic as Vischeck, extended to non-human receptor sets.
2. Correct viewing geometry. Acuity and compound-eye sampling are computed in **scene degrees**, not screen pixels. This matters more than it sounds (section 4).
3. Every number on screen carries its citation, and every limitation is stated in the app rather than in a footnote.
4. Open source (GPL-3 like the rest of the repo), so the transforms can be checked and reused.

**Where the science is weak, say so.** Dogs and cats are on firm ground (two well-measured cone pigments, behavioural acuity, rod-dominated retinas). Flies and bees are much weaker as a *simulation*: their most distinctive channel is ultraviolet, which an RGB camera cannot see, and their most distinctive property is temporal (flicker fusion far above ours), which a 60 Hz display cannot show. The prototype includes them as an honest "compound-eye sampling plus the visible receptor channels" view with the UV caveat in the app. I would not lead with the fly. Birds should not be added without a UV-capable camera.

**Recommendation.** Ship dog and cat as the flagship, keep bee and fly as clearly-labelled educational modes, build it as a progressive web app on vischeck.com first (no app-store gate, one codebase, already runs at camera frame rate on WebGL2), and wrap it natively only if there is demand.

## 2. What already exists in this repo and is reused

| Asset | Where | Reused for |
|---|---|---|
| Brettel/Viénot/Mollon JS pipeline, sRGB decode/encode | `website/assets/js/vischeck.js`, `app.js` | Same colour-space conventions; the animal transforms are the same idea with a different receptor set |
| TinyEyes blur-in-degrees model (display width + viewing distance to pixels/degree) | `website/assets/js/tinyeyes.js`, `pytorch_implementation/` | Geometry handling; the "true scale" mode is the camera-side equivalent |
| Measured display primaries 370–730 nm | `displays/chesnutSpectra.txt` | The "stimulus" spectra behind each RGB pixel |
| Human cone fundamentals | `displays/SPconeSpectra.txt` | Human reference; sanity checks |
| Jekyll site on GitHub Pages | `website/`, `.github/workflows/deploy-pages.yml` | Hosting; the page is already wired into the nav |

## 3. The model

### 3.1 Colour (all species)

Let `P` (361×3) be the display primaries and `S_a` (361×n) the animal's receptor sensitivities. Then `M = S_aᵀ P` (n×3) maps linear RGB to receptor catches. For a dichromat (n = 2) we choose the plane spanned by display white and display blue as the rendering surface, and

    T = B · inv(M B) · M        with B = [white, blue],  so that  M T = M.

`T` is a 3×3 matrix on linear sRGB, idempotent, and the animal's catches for `T(rgb)` equal those for `rgb`. It runs as one matrix multiply in the fragment shader. This is exactly how `runVischeck3` treats human dichromats; the only difference is that the receptor set is not a subset of ours, so the human "what colour is it" question has no answer and the rendering plane is a convention. We state that in the app.

Receptor sensitivities use the Govardovskii et al. (2000) A1 template (α and β bands) at the published λmax. Ocular-media filtering is ignored; for a display-driven stimulus (≥ 380 nm) this is a small effect for dogs and cats (Douglas & Jeffery 2014 show canid lenses transmit well into the UV, which matters for real scenes, not for display metamers).

Verified in `model/derive_animal_models.py` (asserts `M T = M`) and in the headless test, which compares GPU output with the Python reference on colour swatches: max error 0/255 for dog, cat and bee.

Resulting transforms (linear sRGB, row-major):

| Species | Receptors used | `T` (rounded) | What happens to pure red |
|---|---|---|---|
| Dog | 429, 555 nm | [[.18,.82,0],[.18,.82,0],[.01,-.01,1]] | dim yellow (18 % of green's brightness) |
| Cat | 450, 556 nm | [[.19,.81,0],[.19,.81,0],[-.01,.01,1]] | dim yellow |
| Honeybee | 436, 544 nm (UV 344 omitted) | [[.13,.87,0],[.13,.87,0],[.01,-.01,1]] | dark yellow |
| Fruit fly | 442, 600 nm in vivo (UV 331/355 omitted) | [[.44,.56,0],[.44,.56,0],[-.03,.03,1]] | bright yellow |

### 3.2 Acuity

A Gaussian point-spread function whose modulation transfer falls to 0.1 at the species' cut-off frequency; the blur *added* to the camera image is √(σ_animal² − σ_human²) with a nominal 50 cycles/deg human cut-off. Dim-light acuity is interpolated (in variance) as the light slider drops.

| Species | Bright-light cut-off (cpd) | Dim (cpd) | Source |
|---|---|---|---|
| Human | 50 (30–60) | 8 (5.9–9.9) | Lind et al. 2017 for the dim value |
| Dog | 8 (Snellen 20/75); range 5.5–19.5 | 2.6 (1.8–3.5) | Miller & Murphy 1995; Odom et al. 1983 (11.6–12.6 cpd, pattern ERG/VEP); Lind et al. 2017 |
| Cat | 8.5 (8–9) | not modelled | Jacobson, Franklin & McDonald 1976 |

The dog default is deliberately the conservative review value; the pattern-ERG figure is ~12 cpd and behavioural whippets reached 19.5 cpd, so the slider range is exposed.

### 3.3 Compound eyes

Each ommatidium contributes one value. We blur with a Gaussian acceptance function (FWHM = acceptance angle Δρ), then sample on a hexagonal lattice with spacing Δφ and paint flat facets. The popular "thousands of tiny images" picture is wrong and we do not draw it.

| Species | Δφ | Δρ | Source |
|---|---|---|---|
| Drosophila | 4.6° | ≈ 1.1 Δφ ≈ 5° | Gonzalez-Bellido, Wardill & Juusola 2011 |
| Calliphora | ≈ 1.5° (approximate) | ≈ 1.0–1.24° | Land 1997; needs checking against the primary source before release |

At a 70° phone camera field of view this gives ~15 facets across the image for Drosophila, which is the honest answer. Bee optics are not implemented in the prototype.

### 3.4 Low light

The light slider mixes the photopic transform with a rod-weighted grey (rod λmax 508 nm dog, Jacobs et al. 1993; 500 nm generic for cat; 498 nm human) and widens the acuity blur to the dim-light value. No tapetal gain is applied, because I did not find a clean number to cite; the dog and cat advantage in dim light is therefore *understated* in the prototype. Retinal noise, adaptation and the Purkinje shift are not modelled.

### 3.5 Deliberately not simulated

- Ultraviolet (flies, bees, birds; and possibly some UV sensitivity in dogs via lens transmission). Requires a UV-capable camera. See Phase 3.
- Temporal resolution. Dog CFF is well above human (Coile, Pollitz & Smith 1989; Miller & Murphy 1995), flies far higher again. A display cannot render this; at most it is an information card.
- Field of view and binocular overlap (dog ≈ 240° total per Miller & Murphy 1995). Could be faked with a wide-angle lens and a mask; not a priority.
- Anything about experience. The app says "the animal could not distinguish this from the original", never "this is what the animal sees".

## 4. The geometry problem, and why it is the product

A phone shows a 70° camera field on a screen that subtends about 13° at 30 cm: the world is minified ~5×. A dog's 8 cpd cut-off in scene degrees becomes ~40 cpd on the screen, right at the viewer's own limit. Measured in the prototype: at 5.8 px/deg (the 350 px sample image) the dog blur is 0.2 px; at 720p with a 70° field it is ~0.4 px. **A physically correct dog-acuity blur is invisible on a phone.** Apps that show an obvious blur are either blurring in screen pixels (which answers "what would a dog see looking at this phone", not "what does a dog see in the world") or exaggerating.

Two consequences for the design:

1. **True-scale mode** (implemented): crop the centre of the camera image so the screen subtends the same angle as the scene it shows, given displayed width and viewing distance. Then blur, facets and everything else are 1:1. On a phone this is a ~5× zoom, which is fine for a demo and exactly what a VR passthrough headset does natively.
2. **Label exaggeration as exaggeration** (implemented: an "exaggerate blur ×" slider that says "not physical").

This is the point I would put on the About page. It is simple, correct, and none of the competitors do it.

## 5. Phased plan

**Phase 0, done on this branch.** Model derivation script with citations; WebGL2 live-camera page with species, light level, split view, true scale, mosaic; headless test that checks the GPU against the CPU reference; nav and manifest.

**Phase 1, web release (1–2 weeks of work).**
- Camera field-of-view calibration: the browser does not report it. Add a one-off calibration (hold the phone at a known distance from a ruler or an A4 sheet, drag two markers) and remember it. Default 70°.
- Service worker for offline use and a real install prompt; proper icons (PNG 192/512).
- Performance pass on old phones: cap camera at 720p, keep the blur working resolution small (already done), test on a 2019-era Android.
- Copy: About panel with the section-4 argument, references with links, "how this differs from other apps".
- Review by a colleague who works on comparative vision; fix the cat rod value and the Calliphora Δφ from primary sources.
- Merge to `main` (that deploys to vischeck.com).

**Phase 2, phone app.** Only if wanted for discoverability. Wrap the same page with Capacitor for App Store / Play Store. Cost is mostly store admin, icons and privacy text (camera only, no upload). No native code needed; camera and WebGL2/WebGPU are available in the web views. Keep the PWA as the canonical version.

**Phase 3, things that would be genuinely new.**
- UV stills. A full-spectrum converted camera or a Raspberry Pi HQ camera with UV-pass/visible filters (the Tedore & Nilsson 2019 approach) gives a real UV channel; then bees, flies and birds become proper simulations, offline, from stills.
- VR passthrough (Quest-class headsets) for a true 1:1 field: this is the only display on which dog and cat acuity, field of view and the fly mosaic all look right without cropping.
- A "dog looking at your screen" mode using the TinyEyes geometry (screen size and distance), which is the correct way to blur in screen pixels.

## 6. Validation and testing

- `model/derive_animal_models.py` asserts metamer preservation for every dichromat transform.
- `model/test_animal_vision.mjs` (Playwright, headless Chromium with SwiftShader) loads the page, feeds colour swatches, reads the canvas back and compares with the CPU reference; then screenshots every species. Run with `npx http-server website -p 8080` in one shell and `node model/test_animal_vision.mjs` in another.
- Before release: compare dog output against dog-vision.andraspeter.com on the same image (expect similar hue, different blur handling), and have the parameter table checked by a comparative-vision colleague.

## 7. Risks and open questions

- **Overclaiming** is the main reputational risk; the caveat box is in the UI for that reason. Do not add species without a primary-source λmax and an acuity figure.
- **Separate repo or this one?** I could not create a new repository from this session (the GitHub integration returned 403 on repository creation), so everything is on a branch here. A separate `wadelab/animal-vision` repo makes sense if the phone wrapper happens; until then keeping it next to Vischeck keeps the shared spectra and pipeline in one place.
- **Name.** The page is "Animal Vision Simulator"; a product name is your call.
- **Licence.** GPL-3 inherited from the repo. Fine for a web app; a store wrapper is also fine under GPL but check if any store-specific closed component would be bundled.
- **Camera in embedded contexts.** getUserMedia needs HTTPS and a top-level page or an iframe with camera permission; vischeck.com on GitHub Pages is HTTPS, so fine.

## 8. References

PMIDs and page numbers were checked against publisher pages or PubMed listings found by web search during drafting (PubMed itself was not reachable from this session). Items marked † were confirmed from abstracts only.

- Brettel H, Viénot F, Mollon JD (1997). Computerized simulation of color appearance for dichromats. *J Opt Soc Am A* 14(10):2647–2655. PMID 9316278.
- Coile DC, Pollitz CH, Smith JC (1989). Behavioral determination of critical flicker fusion in dogs. *Physiol Behav* 45(6):1087–1092. PMID 2813532.
- Douglas RH, Jeffery G (2014). The spectral transmission of ocular media suggests ultraviolet sensitivity is widespread among mammals. *Proc R Soc B* 281(1780):20132995. PMID 24552839.
- Gonzalez-Bellido PT, Wardill TJ, Juusola M (2011). Compound eyes and retinal information processing in miniature dipteran species match their specific ecological demands. *PNAS* 108(10):4224–4229.
- Govardovskii VI, Fyhrquist N, Reuter T, Kuzmin DG, Donner K (2000). In search of the visual pigment template. *Vis Neurosci* 17(4):509–528. PMID 11016572.
- Guenther E, Zrenner E (1993). The spectral sensitivity of dark- and light-adapted cat retinal ganglion cells. *J Neurosci* 13(4):1543–1550.
- Jacobs GH, Deegan JF, Crognale MA, Fenwick JA (1993). Photopigments of dogs and foxes and their implications for canid vision. *Vis Neurosci* 10(1):173–180. PMID 8424924.
- Jacobson SG, Franklin KB, McDonald WI (1976). Visual acuity of the cat. *Vision Res* 16(10):1141–1143. †
- Kelber A, Vorobyev M, Osorio D (2003). Animal colour vision: behavioural tests and physiological concepts. *Biol Rev* 78(1):81–118. PMID 12620062.
- Land MF (1997). Visual acuity in insects. *Annu Rev Entomol* 42:147–177. PMID 15012311.
- Lind O, Milton I, Andersson E, Jensen P, Roth LSV (2017). High visual acuity revealed in dogs. *PLoS ONE* 12(12):e0188557.
- Miller PE, Murphy CJ (1995). Vision in dogs. *J Am Vet Med Assoc* 207(12):1623–1634. PMID 7493905.
- Mowat FM, Petersen-Jones SM, Williamson H, Williams DL, Luthert PJ, Ali RR, Bainbridge JW (2008). Topographical characterization of cone photoreceptors and the area centralis of the canine retina. *Mol Vis* 14:2518–2527. PMID 19112529.
- Neitz J, Geist T, Jacobs GH (1989). Color vision in the dog. *Vis Neurosci* 3(2):119–125. PMID 2487095.
- Odom JV, Bromberg NM, Dawson WW (1983). Canine visual acuity: retinal and cortical field potentials evoked by pattern stimulation. *Am J Physiol* 245(5):R637–R641. PMID 6638211.
- Peitsch D, Fietz A, Hertel H, de Souza J, Ventura DF, Menzel R (1992). The spectral input systems of hymenopteran insects and their receptor-based colour vision. *J Comp Physiol A* 170:23–40. PMID 1573568.
- Salcedo E, Huber A, Henrich S, Chadwell LV, Chou WH, Paulsen R, Britt SG (1999). Blue- and green-absorbing visual pigments of Drosophila: ectopic expression and physiological characterization of the R8 photoreceptor cell-specific Rh5 and Rh6 rhodopsins. *J Neurosci* 19(24):10716–10726. PMID 10594055.
- Sharkey CR, Blanco J, Leibowitz MM, Pinto-Benito D, Wardill TJ (2020). The spectral sensitivity of Drosophila photoreceptors. *Sci Rep* 10:18242. PMID 33106518.
- Tedore C, Nilsson D-E (2019). Avian UV vision enhances leaf surface contrasts in forest environments. *Nat Commun* 10:238.

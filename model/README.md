# Animal vision model

`derive_animal_models.py` turns published photoreceptor peaks into the 3×3 linear-sRGB
matrices, rod weights, blur widths and compound-eye parameters used by
`website/animal.html`. It reads the display primaries and human cone fundamentals from
`../displays/` and writes `animal_models.json` here and `animal_models.js` next to the web app.

    pip install numpy
    python3 derive_animal_models.py            # regenerates model/animal_models.json
    cp animal_models.js ../website/assets/js/  # (the script writes it here; copy or --out)

Every species entry carries its references; see `docs/ANIMAL_VISION_PLAN.md` for the method
and the caveats. `test_animal_vision.mjs` is a Playwright check that the GPU pipeline reproduces
the CPU reference (needs `npm i playwright` and `npx http-server website -p 8080`).

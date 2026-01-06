# VischeckTinyeyes

This repo contains:

- **Vischeck**: color vision deficiency simulation (Brettel/Vienot/Mollon)
- **Daltonize**: compensates L-M opponent loss by remapping into other opponent channels
- **TinyEyes**: infant vision simulation over a range of ages

## Ubuntu quickstart (Vischeck + Daltonize)

Install dependencies:

`sudo apt-get update && sudo apt-get install -y build-essential libfftw3-dev imagemagick netpbm libjpeg-progs`

Build the CLI:

`cd CSource && make clean && make`

Run help:

`./runVischeck3 -h`

Example (JPEG -> simulated JPEG):

`convert testImage.jpg RGB:- | ./runVischeck3 -m 640,512 -t deuteranope -d 200 -r 90 | rawtoppm -rgb 640 512 - | ppmtojpeg --quality=80 > out_deut.jpg`

Example with Daltonize enabled:

`convert testImage.jpg RGB:- | ./runVischeck3 -a -s 50 -l 50 -y 50 -m 640,512 -t deuteranope -d 200 -r 90 | rawtoppm -rgb 640 512 - | ppmtojpeg --quality=80 > out_daltonized.jpg`

## TinyEyes (Python)

The TinyEyes implementation in `pytorch_implementation/` supports a lightweight **CPU/PIL path** (no torch required) and an optional tensor/GPU path.

Create/activate a venv and install deps using `pip` (or `uv pip` if you prefer):

- `python3 -m venv .venv && source .venv/bin/activate`
- `python -m pip install -r pytorch_implementation/requirements.txt`

Then run:

`cd pytorch_implementation && python run_TinyEyes_with_Pytorch.py`

## License

GPL-3.0 (see `LICENSE`).

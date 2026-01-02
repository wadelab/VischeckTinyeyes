# The New Vischeck Website

## Overview

The new Vischeck website is a modern, browser-based rebuild of the original Vischeck tool that was first launched in 2000 by Robert Dougherty and Alex Wade at Stanford University. The website provides three powerful vision simulation tools:

1. **Vischeck** - Color vision deficiency (color blindness) simulation
2. **Daltonize** - Color correction for individuals with color vision deficiencies
3. **TinyEyes** - Infant vision simulation across different developmental ages

## What's New?

### Client-Side JavaScript Implementation

The biggest change in this rebuild is the shift from server-side processing to a **fully client-side JavaScript implementation**. This means:

- **Privacy**: Your images never leave your browser - all processing happens locally on your device
- **Speed**: No server round-trips means instant processing
- **Offline capability**: The website can work without an internet connection once loaded
- **No server costs**: The site can be hosted as static files on GitHub Pages
- **Better user experience**: Real-time previews and immediate feedback

### Modern Web Interface

The new website features:

- Clean, modern design with an intuitive user interface
- Responsive layout that works on desktop and mobile devices
- Real-time preview of all simulation results
- Side-by-side comparison views
- Easy image upload and sample image selection
- Adjustable parameters for fine-tuning results

## Features

### 1. Vischeck - Color Blindness Simulator

Located at `/run.html`, this tool simulates how images appear to individuals with different types of color vision deficiencies.

**Supported Vision Types:**
- **Normal** - Standard color vision (trichromatic)
- **Protanope** - Red-blind (missing L cones)
- **Deuteranope** - Green-blind (missing M cones)
- **Tritanope** - Blue-blind (missing S cones)

**Algorithm:**
The simulator implements the Brettel/Vienot/Mollon LMS projection algorithm for dichromatic vision, published in their 1997 paper "Computerized simulation of color appearance for dichromats."

**Features:**
- Upload your own images or use sample images
- Configurable maximum image size (512px, 1024px, or 2048px)
- Four simultaneous preview panels:
  - Original image
  - Simulated (how it appears to someone with the selected deficiency)
  - Daltonized (corrected image)
  - Daltonized + Simulated (verify the correction)
- Download any of the four views as PNG

### 2. Daltonize - Color Correction

Daltonize is an optional correction feature built into the color blindness simulator. It was originally created in May 2002 and has been rebuilt here with modern JavaScript.

**How It Works:**
Daltonize applies an opponent-space correction that:
1. Detects L-M (red-green) contrast that would be lost to color vision deficiencies
2. Projects this information into other perceptual channels (L+M luminance and S blue-yellow)
3. Preserves visual information that would otherwise be imperceptible

**Adjustable Parameters:**
- **LM stretch** (0-100): Controls how much L-M opponent information is enhanced
- **Lum scale** (0-100): Controls luminance channel correction strength
- **S scale** (0-100): Controls S (blue-yellow) channel correction strength

**Use Cases:**
- Making images and graphics more accessible to color blind viewers
- Designing inclusive visual materials
- Testing whether color-coded information remains distinguishable

### 3. TinyEyes - Infant Vision Simulator

Located at `/tinyeyes.html`, this tool simulates how images appear to infants at different developmental stages.

**How It Works:**
TinyEyes applies age-dependent Gaussian blur in opponent color space (L+M, L-M, S), based on developmental vision research estimates.

**Parameters:**
- **Age selection**: Choose from multiple infant age ranges (the specific ages are loaded from the JavaScript module)
- **Display width (cm)**: Physical width of the display in centimeters
- **Viewing distance (cm)**: Distance between viewer and screen in centimeters
- **Max size**: Image resolution (256px, 512px, or 1024px square)

**Features:**
- Upload images or use samples
- Real-time comparison between original and infant vision simulation
- Images are center-cropped to square format
- Accounts for viewing distance and display size for accurate spatial frequency filtering
- Download simulated results

**Team:**
The TinyEyes feature was developed by Alex Wade and Heidi Baseler at the University of York, with PyTorch implementation by Áine Dineen at Trinity College Dublin.

## Technical Architecture

### Technology Stack

- **Pure HTML5/CSS3/JavaScript** - No frameworks required for the core functionality
- **Canvas API** - For image processing and rendering
- **ES6 Modules** - Modern JavaScript module system
- **Static site hosting** - Can be deployed to GitHub Pages or any static host

### File Structure

```
website/
├── index.html          # Homepage
├── about.html          # About page with history and algorithms
├── run.html            # Vischeck/Daltonize simulator
├── tinyeyes.html       # TinyEyes infant vision simulator
├── assets/
│   ├── css/
│   │   └── styles.css  # Unified styles
│   ├── js/
│   │   ├── app.js      # Vischeck/Daltonize application
│   │   ├── tinyeyes_app.js  # TinyEyes application
│   │   ├── vischeck.js      # Vischeck core algorithm
│   │   ├── daltonize.js     # Daltonize core algorithm
│   │   └── tinyeyes.js      # TinyEyes core algorithm
│   └── img/            # Sample images and assets
└── _config.yml         # Jekyll configuration for GitHub Pages
```

### Processing Pipeline

**Vischeck/Daltonize:**
1. Load image into Canvas element
2. Extract pixel data (RGBA array)
3. Convert RGB → XYZ → LMS color space
4. Apply dichromatic simulation (Brettel algorithm)
5. Optionally apply Daltonize correction in opponent space
6. Convert LMS → XYZ → RGB
7. Render to output canvas

**TinyEyes:**
1. Load and center-crop image to square
2. Convert RGB → LMS → Opponent space (L+M, L-M, S)
3. Apply age-dependent Gaussian blur to each opponent channel
4. Convert back: Opponent → LMS → RGB
5. Render blurred result

## Usage Guide

### Using the Color Blindness Simulator

1. **Navigate** to the Colorblindness page (`run.html`)
2. **Upload an image** using the file input, or select a sample image
3. **Choose the simulation type** (Protanope, Deuteranope, or Tritanope)
4. **Click "Run"** to process the image
5. **View results** in the four preview panels
6. **Optional**: Enable Daltonize and adjust parameters for correction
7. **Click "Run"** again to see the corrected version
8. **Download** any of the four views using the download options

### Using TinyEyes

1. **Navigate** to the TinyEyes page (`tinyeyes.html`)
2. **Upload an image** or select a sample
3. **Choose an infant age** from the dropdown
4. **Adjust display parameters** (width and viewing distance) for accurate simulation
5. **Click "Run"** to see how an infant at that age would perceive the image
6. **Download** the simulated result

## Historical Context

### Original Vischeck (2000-2023)

The original Vischeck website launched in August 2000 and became one of the first and most trusted color blindness simulators on the web. It used server-side processing and required uploading images to be processed.

The Daltonize feature was added in May 2002, making Vischeck one of the first tools to not just simulate color blindness, but also provide correction.

### Legacy and Impact

- The Vischeck algorithm was incorporated into **GIMP** (GNU Image Manipulation Program) as a built-in filter
- The site has been used by designers, researchers, and educators worldwide
- It has helped make countless websites, documents, and graphics more accessible
- The original site served millions of image simulations over two decades

### The Rebuild (2024-2026)

This new implementation maintains the accuracy of the original algorithms while leveraging modern web technologies for a better user experience. The client-side approach also addresses privacy concerns and reduces infrastructure costs.

## Credits

### Original Authors
- **Robert Dougherty** - Now at Soundtrip Health ([LinkedIn](https://www.linkedin.com/in/rfdougherty/))
- **Alex Wade** - University of York ([Profile](https://www.york.ac.uk/psychology/staff/academicstaff/alex-wade/))

### TinyEyes Team
- **Alex Wade** - University of York
- **Heidi Baseler** - University of York ([Profile](https://www.york.ac.uk/psychology/staff/academicstaff/hb554/))
- **Áine Dineen** - Trinity College Dublin (PyTorch implementation) ([LinkedIn](https://www.linkedin.com/in/aine-travers-dineen/))

### Original Research
Both post-docs worked in the Wandell Lab at Stanford University ([Wandell Lab](https://wandell.vista.su.domains/blog/)).

## Scientific References

### Vischeck Algorithm
- Brettel, H., Viénot, F., & Mollon, J. D. (1997). Computerized simulation of color appearance for dichromats. *Journal of the Optical Society of America A*, 14(10), 2647-2655. [PubMed](https://pubmed.ncbi.nlm.nih.gov/9316278/)

### Color Vision Deficiency
- Affects approximately 8% of men and 0.5% of women of Northern European descent
- Most common types: Deuteranopia (green-blind) and Protanopia (red-blind)
- Tritanopia (blue-blind) is much rarer

## Source Code

The complete source code for this project is available on GitHub:
- **Repository**: [github.com/wadelab/VischeckTinyeyes](https://github.com/wadelab/VischeckTinyeyes)
- **License**: Check the repository for license information
- **Website**: Hosted on GitHub Pages at [vischeck.com](http://vischeck.com)

## Future Enhancements

Potential improvements being considered:
- Additional color vision deficiency types (anomalous trichromacy)
- Batch processing for multiple images
- Animation/video support
- Additional sample images and test patterns
- Mobile app versions
- Integration with design tools (Figma, Adobe plugins)
- API for automated accessibility testing

## Comparison with Other Tools

While there are now several color blindness simulators available (such as [Coblis](https://www.color-blindness.com/coblis-color-blindness-simulator/) and [Toptal's Colorfilter](https://www.toptal.com/designers/colorfilter/)), Vischeck remains notable for:

1. **Historical significance** - The original and most trusted simulator since 2000
2. **Daltonize correction** - One of the few tools that offers correction, not just simulation
3. **Scientific accuracy** - Based on peer-reviewed research (Brettel et al.)
4. **TinyEyes** - Unique infant vision simulation not found in other tools
5. **Open source** - Code is available for inspection and use
6. **Privacy** - Client-side processing ensures images never leave your browser

## Getting Started

### For Users
Simply visit the website and start using the tools - no installation or account required.

### For Developers
To run the website locally:

```bash
# Clone the repository
git clone https://github.com/wadelab/VischeckTinyeyes.git
cd VischeckTinyeyes/website

# Serve the website (use any static server)
python3 -m http.server 8000
# or
npx http-server

# Open in browser
open http://localhost:8000
```

### For Researchers
If you use Vischeck in your research, please cite:
- Brettel, H., Viénot, F., & Mollon, J. D. (1997) for the simulation algorithm
- The original Vischeck website (vischeck.com) for the implementation

## Support and Contact

- **Issues/Bug Reports**: [GitHub Issues](https://github.com/wadelab/VischeckTinyeyes/issues)
- **Email**: wade@wadelab.net
- **Twitter**: @vischeck

## Conclusion

The new Vischeck website brings a 24-year-old pioneering tool into the modern web era. By moving to client-side processing while maintaining scientific accuracy, it continues to serve its mission: making the visual world more accessible and helping designers create more inclusive content.

Whether you're a designer checking your work for accessibility, a researcher studying color vision, an educator teaching about perception, or someone with a color vision deficiency exploring how to adapt images - the new Vischeck website provides powerful, privacy-respecting tools that work right in your browser.

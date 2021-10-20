#include "runSimulation.h"
#include "colorTools.h"
#include <iostream>

#include "imglib.h"
#include "kernlib.h"
#include <time.h>
#include <math.h>


void runSimulation(unsigned char *dataPtr, int x, int y, float viewDist, 
		   float dpi, char *sensorType, char *simDisplayType, 
		   char *viewDisplayType, float *kernelWt, float *kernelSD, 
		   float *kernelScale)
{
  // 
  // This functions takes an RGB image (unsigned chars of format RGBRGBRGB... 
  // with dimention x,y) and simulates how that image would look given the 
  // particular parameters.
  //
  // The following params are a set- if any one is not specified, then 
  // defaults override any that are specified. 
  // kernelWeights, kernelFWHMs: if either is NULL, then default weights 
  // and widths will be used (from SCIE-LAB; Poirson & Wandell parameters).  
  // If both are not NULL, then both had better be arrays of length 9 in the 
  // following format: 
  // 		weights = lum1 lum2 lum3 l-m1 l-m2 l-m3 s1 s2 s3
  // 		SDs     = lum1 lum2 lum3 l-m1 l-m2 l-m3 s1 s2 s3
  // kernelScale: scale factor applied to lum, l-m, s channel kernels; 
  // defaults to 1,1,1
  //

  // create the 3-plane image structure
  img image(x,y);

  // Load simulated display device data
  // 
  displayDevice myDisplay(simDisplayType);
  //  myDisplay.loadDevice(simDisplayType);

  // Load raw image data (uchars in dataPtr) into the float array
  // 
  if (myDisplay.gammaLen()-1 != image.getMaxImgVal()) // then we have to scale
    image.assignUchar(dataPtr, (1.0*myDisplay.gammaLen()/image.getMaxImgVal()));
  else
    image.assignUchar(dataPtr);

  // Apply Gamma correction
  //
  image.applyLookupTable(myDisplay.gammaPtrR(), myDisplay.gammaPtrG(), myDisplay.gammaPtrB());
			
  // Do Brettel/Vienot/Mollon transform only if sensor-type is not 'normal'
  if(sensorType[0]!='n'){
    // we need to go to LMS space to do the Brettel transform
    image.changeColorSpace(myDisplay.getRGB2LMS());
    image.colorSpaceLabel = LMS;
    image.brettelTransform(sensorType[0], myDisplay.getRGB2LMS());
  }else if (simDisplayType[0]!=viewDisplayType[0] && (viewDist<=0.0 || dpi<=0.0)){
    // if we get here, then all that is different is the display type.
    // To get the color effects, we need to do some kind of color transform.
    // We opted to do rgb2lms just because it's way cool.
    // (without this conditional, we'd wind up doing no color-space transforms
    // when all is normal except the display type.)
    image.changeColorSpace(myDisplay.getRGB2LMS());
    image.colorSpaceLabel = LMS;
  }
  // Do spatial filtering
  //
  if (viewDist>0.0 && dpi>0.0) {
    // convert dpi and viewDist into samples-per-degree
    float sampPerDeg = viewDist * 0.0174550649282176 * dpi;

    // The spatial work is done in opponent color space
    // 
    switch (image.colorSpaceLabel){
    case RGB: image.changeColorSpace(myDisplay.getRGB2OPP()); break;
    case LMS: image.changeColorSpace(myDisplay.getLMS2OPP()); break;
    case OPP: break;
    }    
    image.colorSpaceLabel = OPP;
    
    // SPATIAL FILTER
    // Generate kernels here - either in F space or R-space and then transform
    // These are the parameters for generating the filters,
    //
    // Create the pattern-color separable filters according to
    // the Poirson & Wandell 1993 fitted spatial response. The filters
    // are each weighted sum of 2 or 3 gaussians.
    // in the format [halfwidth weight halfwidth weight ...].
    // The halfwidths are in degrees of visual angle.
    // k1 = [0.05      0.9207    0.225    0.105    7.0   -0.1080];
    // k2 = [0.0685    0.5310    0.826    0.33];
    // k3 = [0.0920    0.4877    0.6451    0.3711];
    // Converting fullwidths to SDs (SD = fwhm/sqrt(8*log(2)) = fwhm/2.3548), we get:
    // Converting halfwidths to SDs (SD = hwhm/sqrt(2*log(2)) = hwhm/1.1774), we get:
    // k1 = [0.0425    0.9207    0.1911    0.105    5.9453   -0.1080];
    // k2 = [0.0582    0.5310    0.7015    0.33];
    // k3 = [0.0781    0.4877    0.5479    0.3711];
    // Old values (with incorrect hwhm->SD conversion):
    // k1 = [0.0213    0.9207    0.0957    0.105    2.9787   -0.1080];
    // k2 = [0.0291    0.5310    0.3515    0.33];
    // k3 = [0.0391    0.4877    0.2745    0.3711];
    // Limit the width of filters to 1 degree visual angle? No- this seems like a hack
    // done in scie lab to save time by making the convolution kernels smaller.
    // Convert the unit of SDs of visual angle to pixels by * sampPerDeg   
				
    image.doFFT(FFTW_FORWARD);

    //    kernel convKern(x,y,HAS_FFT);
    kernelSep convKern(image.getFourierRows(),image.getFourierCols());
    if (kernelWt==NULL || kernelSD==NULL){
    //convKern.setKernFFT(1, .9207, 0.0425*sampPerDeg, .1050, 0.1911*sampPerDeg, -.1080, 5.9453*sampPerDeg, 1.0); 
    //convKern.setKernFFT(2, .5310, 0.0582*sampPerDeg, .3300, 0.7015*sampPerDeg, 0.0, 1.0, 1.0);
    //convKern.setKernFFT(3, .4877, 0.0781*sampPerDeg, .3711, 0.5479*sampPerDeg, 0.0, 1.0, 1.0);
    //convKern.setKernFFT(1, .9207, 0.0107*sampPerDeg, .1050, 0.0479*sampPerDeg, -.1080, 1.4894*sampPerDeg, 1.0); 
    //convKern.setKernFFT(2, .5310, 0.0146*sampPerDeg, .3300, 0.1758*sampPerDeg, 0.0, 1.0, 1.0);
    //convKern.setKernFFT(3, .4877, 0.0191*sampPerDeg, .3711, 0.1373*sampPerDeg, 0.0, 1.0, 1.0);
      convKern.setKernFFT(1, 0.9207, 0.0107*sampPerDeg, 0.0, 0.0479*sampPerDeg, 
			  0.0, 1.4894*sampPerDeg, 1.0); 
      convKern.setKernFFT(2, 0.5310, 0.0146*sampPerDeg, 0.0, 0.1758*sampPerDeg, 
			  0.0, 1.0, 1.0);
      convKern.setKernFFT(3, 0.4877, 0.0191*sampPerDeg, 0.0, 0.1373*sampPerDeg, 
			  0.0, 1.0, 1.0);
    }
    else{
      convKern.setKernFFT(1, kernelWt[0], kernelSD[0]*sampPerDeg, kernelWt[1], 
			  kernelSD[1]*sampPerDeg, kernelWt[2], 
			  kernelSD[2]*sampPerDeg, kernelScale[0]); 
      convKern.setKernFFT(2, kernelWt[3], kernelSD[3]*sampPerDeg, kernelWt[4], 
			  kernelSD[4]*sampPerDeg, kernelWt[5], 
			  kernelSD[5]*sampPerDeg, kernelScale[1]);
      convKern.setKernFFT(3, kernelWt[6], kernelSD[6]*sampPerDeg, kernelWt[7], 
			  kernelSD[7]*sampPerDeg, kernelWt[8], 
			  kernelSD[8]*sampPerDeg, kernelScale[2]);
    }		

    //		convKern.doFFT(FFTW_FORWARD);

    // For Debugging: 
    //convKern.writeRawFFT("/tmp/convKern.txt");
    //std::cerr << "sampPerDeg=" << sampPerDeg << std::endl;
    // just doing forward fft/reverse fft is fine. The problem is with the
    // kernel itself (mostly NaNs).
    image.dotMultiplyFFT(convKern); // This does the convolution in F-space
    image.doFFT(FFTW_BACKWARD);
  }
    
  // Convert back to RGB
  // 
  myDisplay.loadDevice(viewDisplayType);
  switch (image.colorSpaceLabel){
  case LMS: image.changeColorSpace(myDisplay.getLMS2RGB()); break;
  case OPP: image.changeColorSpace(myDisplay.getOPP2RGB()); break;
  case RGB: break;
  }  
  image.colorSpaceLabel = RGB;

  // Make sure that the image values are still in range (0-maxImgVal)
  //
  image.clipValRange();
  //image.scaleValRange();

  // Apply Inverse Gamma
  //
  image.applyLookupTable(myDisplay.invGammaPtrR(), myDisplay.invGammaPtrG(), myDisplay.invGammaPtrB());

  // Put image data back into the uchar array
  // 
  image.extractUchar(dataPtr);

}


void runCorrection(unsigned char *dataPtr, int x, int y, char *simDisplayType, 
		   char *viewDisplayType, float lmStretch, float lumScale, 
		   float sScale)
{
  // 
  // This functions takes an RGB image (unsigned chars of format RGBRGBRGB... 
  // with dimention x,y) and applies the Daltonize correction.
  //

  // create the 3-plane image structure
  img image(x,y);

  // Load simulated display device data
  // 
  displayDevice myDisplay(simDisplayType);
  //  myDisplay.loadDevice(simDisplayType);

  // Load raw image data (uchars in dataPtr) into the float array
  // 
  if (myDisplay.gammaLen()-1 != image.getMaxImgVal()) // then we have to scale
    image.assignUchar(dataPtr, (1.0*myDisplay.gammaLen()/image.getMaxImgVal()));
  else
    image.assignUchar(dataPtr);

  // *** FIX ME: The following is inefficient
  image.daltonize(lumScale, sScale, lmStretch);

//   // Apply Gamma correction
//   //
//   image.applyLookupTable(myDisplay.gammaPtrR(), myDisplay.gammaPtrG(), myDisplay.gammaPtrB());
			
//   // we need to go to OPP space for the correction
//   image.changeColorSpace(myDisplay.getRGB2OPP());
//   image.colorSpaceLabel = OPP;
//   // *** WORK HERE
//   // Inputs are always 0-1- it's up to us to scale them to the apropriate range.
//   lmStretch = lmStretch*2.0+1.0;
//   float xform[16];
//   image.computeDaltonize(xform,lmStretch,lumScale,sScale); 
//   image.changeColorSpace4Matrix(xform);
   
//   // Convert back to RGB
//   // 
//   myDisplay.loadDevice(viewDisplayType);
//   switch (image.colorSpaceLabel){
//   case LMS: image.changeColorSpace(myDisplay.getLMS2RGB()); break;
//   case OPP: image.changeColorSpace(myDisplay.getOPP2RGB()); break;
//   case RGB: break;
//   }  
//   image.colorSpaceLabel = RGB;

//   // Make sure that the image values are still in range (0-maxImgVal)
//   //
//   image.clipValRange();
//   //image.scaleValRange();

//   // Apply Inverse Gamma
//   //
//   image.applyLookupTable(myDisplay.invGammaPtrR(), myDisplay.invGammaPtrG(), myDisplay.invGammaPtrB());

  // Put image data back into the uchar array
  // 
  image.extractUchar(dataPtr);

}
  

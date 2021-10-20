#include "imglib.h"
#include "kernlib.h"
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string>
#include <fstream>
#include <fftw3.h>

// -------------
kernel::kernel(int rows, int cols, int hasFFT)
{ // Here we also allocate space for FFT
  r = rows;
  c = cols;
  allocateFFTspace();

  // since this is a kernel intended for convolution in frequency space, it
  // should span the whole data set, including any padding:
  r = fourierRows;
  c = fourierCols;
  npix = r*c;
	
  // Allocate one big block of memory, then divy it up ourselves.
  // (makes processor caching more effective? seems to provide ~ 20% speed gain.)
  red = new float [npix*3];
  green = red+npix;
  blue = green+npix;
	
  if (red==NULL) r = c = npix = 0;

	
  return;
}

// ------------------------------------------------------
void kernel::setKern(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale)
{
  // prototype kernel generation routine
  // sets appropriate kernel (kNum) with a normalised gaussian, fftshifted (so that 0 is at 0,0), power=1, SD=kWidth;
  int x,y,i;
	
  float kSD1sq,kSD2sq,kSD3sq,cx,cy,rad,maxRad;
  float totalSum = 0.0;
  float *kPointer = NULL;

  // avoid divide-by-zero:
  if (kSD1==0.0) kSD1 = 0.001;
  if (kSD2==0.0) kSD2 = 0.001;
  if (kSD3==0.0) kSD3 = 0.001;

  kSD1sq = 2*kSD1*kSD1;
  kSD2sq = 2*kSD2*kSD2;
  kSD3sq = 2*kSD3*kSD3;
  cx = c/2+0.5;
  cy = r/2+0.5;
  maxRad = sqrt(cx*cx+cy*cy);
	
  if (kNum==1) kPointer = red;
  else if (kNum==2) kPointer = green;
  else kPointer = blue;
	
  // we've got to normalize the weights of each component to a total area of 1.
  // We can do this empirically (the commented-out loop below), which is how scie-lab
  // does it.  However, for really wide gaussians, this gives a different answer than
  // the much faster method of normalizing by (sqrt(2*pi)*SD)^2 (squared because it's
  // a 2D gaussian).  But which method is correct?
  // My intuition is that we want to adjust the height so that we get a true unit gaussian,
  // regardless of how much we clip it's tails.  Plus, that method is faster!
  /*	float sumVal1, sumVal2, sumVal3;
    sumVal1 = sumVal2 = sumVal3 = 0.0;
    for (x=0;x<c;x++){
    for(y=0;y<r;y++){
    rad = maxRad-sqrt(((x-cx)*(x-cx))+((y-cy)*(y-cy)));
    rad *= rad;
    sumVal1 += exp(-rad/kSD1sq); 
    sumVal2 += exp(-rad/kSD2sq); 
    sumVal3 += exp(-rad/kSD3sq); 
    } // next y
    } // next x
    kW1 /= fabs(sumVal1);
    kW2 /= fabs(sumVal2);
    kW3 /= fabs(sumVal3);*/

  // normalize the weights by (sqrt(2*pi)*SD)^2 = 2*pi*SD^2, so that the weight
  // will specify the true height of a unit-area gaussian.
  // 2*pi = 6.283185307180
  kW1 /= 6.283185307180*kSD1sq;
  kW2 /= 6.283185307180*kSD2sq;
  kW3 /= 6.283185307180*kSD3sq;
	
  for (x=0;x<c;x++){
    for(y=0;y<r;y++){
      rad = maxRad-sqrt(((x-cx)*(x-cx))+((y-cy)*(y-cy)));
      rad *= rad;
      *kPointer = (kW1*exp(-rad/kSD1sq) + kW2*exp(-rad/kSD2sq) + kW3*exp(-rad/kSD3sq)); 
      totalSum += *(kPointer++);
    } // next y
  } // next x
	
  totalSum = fabs(totalSum);

  if (kNum==1) kPointer = red;
  else if (kNum==2) kPointer = green;
  else kPointer = blue;
  // do normalization to power = 1*scale

  for (i=npix-1; i>=0; i--){
    *kPointer = (*kPointer++)/totalSum * scale;
  }
}

// --------------------------------------
void kernel::setKernFFT(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale)
{
  // prototype  F-space kernel generation routine
  // Directly generates the >FFT< of the gaussian specified in setKern with >real< space sd kWidth	
  // If f(x)=e^(-a(x*x))
  // F(k)=1/sqrt(2pi*kWidth)*exp(-(k*k)/2*kWidth^2)
  // **********************************************
  // This isn't finished yet - need more time to think about how the power is distributed in a rectangular array..
  int x,y,i;
	
  float kSD1sq,kSD2sq,kSD3sq,cx,cy,rad;
  float totalSum = 0.0;
  fftw_complex *kPointer;

  // avoid divide-by-zero:
  if (kSD1==0.0) kSD1 = 0.001;
  if (kSD2==0.0) kSD2 = 0.001;
  if (kSD3==0.0) kSD3 = 0.001;

  kSD1sq = 2*kSD1*kSD1;
  kSD2sq = 2*kSD2*kSD2;
  kSD3sq = 2*kSD3*kSD3;
  cx = c/2+0.5;
  cy = r/2+0.5;
	
  if (kNum==1) kPointer = (fftw_complex *)FFT_red;
  else if (kNum==2) kPointer = (fftw_complex *)FFT_green;
  else kPointer = (fftw_complex *)FFT_blue;
	
  // normalize the weights by (sqrt(2*pi)*SD)^2 (= 2*pi*SD^2), so that the weight
  // will specify the true height of a unit-area gaussian.
  // 2*pi = 6.283185307180
  kW1 /= 6.283185307180*kSD1sq;
  kW2 /= 6.283185307180*kSD2sq;
  kW3 /= 6.283185307180*kSD3sq;
	
  for (x=0;x<c;x++){
    for(y=0;y<r;y++){
      rad = sqrt(((x-cx)*(x-cx))+((y-cy)*(y-cy)));
      rad *= rad;
      // I think part of the problem is that the phases of the fft of a gaussian are not all 0.
      // z = amp*exp(i*phase)
      (*kPointer)[0] = (kW1*exp(-rad/kSD1sq) + kW2*exp(-rad/kSD2sq) + kW3*exp(-rad/kSD3sq)); 
      (*kPointer)[1] = 0.0;
      totalSum += (*kPointer)[0];
      kPointer++;
    } // next y
  } // next x
	
  totalSum = fabs(totalSum);
	 
  if (kNum==1) kPointer = (fftw_complex *)FFT_red;
  else if (kNum==2) kPointer = (fftw_complex *)FFT_green;
  else kPointer = (fftw_complex *)FFT_blue;
  // do normalization to power = 1*scale

  for (i=npix-1; i>=0; i--){
    //((*(kPointer++))[0]) *= scale / totalSum;
    (*kPointer)[0] *= scale;
    kPointer++;
  }
}


kernelSep::kernelSep(int rows, int cols)
{
  fourierRows = rows;
  fourierCols = cols;
  fourierRowsComplex = int(fourierRows/2+1);
  fourierColsComplex = fourierCols;

  int totalRows = fourierRowsComplex*2;
  int totalCols = fourierColsComplex*2;

  FFT_redColKern = new float [totalCols*3];
  FFT_greenColKern = FFT_redColKern+totalCols;
  FFT_blueColKern = FFT_greenColKern+totalCols;
  FFT_redRowKern = new float [totalRows*3];
  FFT_greenRowKern = FFT_redRowKern+totalRows;
  FFT_blueRowKern = FFT_greenRowKern+totalRows;
	
  if (FFT_redColKern==NULL || FFT_redRowKern==NULL){
    std::cerr<<"Memory Error in kernelSep!"<<std::endl;
    fourierRows = fourierCols = fourierRowsComplex = fourierColsComplex = 0;
    FFT_redRowKern=FFT_greenColKern=FFT_greenRowKern=FFT_blueColKern=FFT_blueRowKern=NULL;
  }

  return;
}

//kernelSep::~kernelSep(void)
//{
  // The following cause a segmentation fault in main when imageMagick tries to
  // write the outSimFile.  Very weird!
  //  delete FFT_redRowKern;
  //  delete FFT_greenRowKern;
  //  delete FFT_blueRowKern;
  //  delete FFT_redColKern;
  //  delete FFT_greenColKern;
  //  delete FFT_blueColKern;
//  return;
//}


void kernelSep::setKernFFT(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale)
{
  // prototype  F-space kernel generation routine for the row,col separable kernel
  fftwf_plan plan;
  int i;
  float kSD1sq,kSD2sq,kSD3sq,center,d;
  float totalSum;
  float *kPtr, *thisKernel;

  // remember which kernel we're supposed to work on:
  if (kNum==1) thisKernel = (float *)FFT_redColKern;
  else if (kNum==2) thisKernel = (float *)FFT_greenColKern;
  else thisKernel = (float *)FFT_blueColKern;

  // avoid divide-by-zero:
  if (kSD1==0.0) kSD1 = 0.001;
  if (kSD2==0.0) kSD2 = 0.001;
  if (kSD3==0.0) kSD3 = 0.001;

  kSD1sq = 2*kSD1*kSD1;
  kSD2sq = 2*kSD2*kSD2;
  kSD3sq = 2*kSD3*kSD3;

  // avoid divide-by-zero:
  if (kSD1sq==0.0) kSD1sq = 0.0001;
  if (kSD2sq==0.0) kSD2sq = 0.0001;
  if (kSD3sq==0.0) kSD3sq = 0.0001;	

  // Normalize weights of the Gaussian components by sqrt(4*pi)*SD (= 3.544907701811*SD), 
  // so that the incomming weight specifies the true height of a unit-area (1d) gaussian.
  kW1 /= 3.544907701811*kSD1;
  kW2 /= 3.544907701811*kSD2;
  kW3 /= 3.544907701811*kSD3;

  // Normalization of the final kernel to power = (1/sqrt(2))*scale 
  // (sqrt(2)=1.414213562373). It's this rather than 1*scale because 
  // we are applying the row and column separately. Actually, when we 
  // do this, it comes out too dark.  Leaving it at 1*scale seems to 
  // be what we were doing with the full kernel (& seems to match the 
  // original SCIE lab).
  //scale /= 1.414213562373;

  // Compute the column kernel...
  kPtr = thisKernel;
  center = 0.5*fourierCols+0.5;	
  totalSum = 0.0;
  for (i=0;i<fourierCols;i++){
    d = center-fabs(i-center);
    d *= d;
    *kPtr = (kW1*exp(-d/kSD1sq) + kW2*exp(-d/kSD2sq) + kW3*exp(-d/kSD3sq)); 
    totalSum += *kPtr++;
    *kPtr++ = 0.0;	// the pad value
  }
  kPtr = thisKernel;
  //std::cerr << "scale=" << scale << "; totalSum=" << totalSum << std::endl;
  totalSum = scale/fabs(totalSum);
  for (i=0;i<fourierCols;i++){
    *kPtr++ *= totalSum; 
    kPtr++;	// skip the pad value
  }
  // Column kernel FFT:	
  plan = fftwf_plan_dft_r2c_2d(fourierCols,1,(float *)thisKernel,(float (*)[2])thisKernel,FFTW_ESTIMATE);

  if(plan == NULL){
    std::cerr << "can't create plan - error !!!" << std::endl;
    exit(0);
  }
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);

  // Now the row kernel...
  // Remember, the row dimention is half-complex, so there is no pad value to deal with
  if (kNum==1) thisKernel = (float *)FFT_redRowKern;
  else if (kNum==2) thisKernel = (float *)FFT_greenRowKern;
  else thisKernel = (float *)FFT_blueRowKern;
  kPtr = thisKernel;
  center = 0.5*fourierRows+.5;
  totalSum = 0.0;
  for (i=0;i<fourierRows;i++){
    d = center-fabs(i-center);
    d *= d;
    *kPtr = (kW1*exp(-d/kSD1sq) + kW2*exp(-d/kSD2sq) + kW3*exp(-d/kSD3sq)); 
    totalSum += *kPtr++;
  }
  kPtr = thisKernel;
  totalSum = scale/fabs(totalSum);
  // do normalization to power = 1*scale
  for (i=0;i<fourierRows;i++){
    *kPtr++ *= totalSum; 
  }
	 
  // Row kernel FFT
  plan = fftwf_plan_dft_r2c_2d(1,fourierRows,(float *)thisKernel,(float (*)[2])thisKernel,FFTW_ESTIMATE);
  if(plan == NULL){
    std::cerr << "can't create plan - error !!!" << std::endl;
    exit(0);
  }
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);
}

void kernelSep::writeRawFFT(const char *fileName)
{
  int i;
  FILE *fid;

  fid = fopen(fileName, "wt");
  fprintf(fid, "RED:\n");
  for (i=0; i<fourierCols*2; i++){
    fprintf(fid, "%.4f\t",FFT_redColKern[i]);
  }
  fprintf(fid,"\n");
  for (i=0; i<2*(fourierRows/2+1); i++){
    fprintf(fid, "%.4f\t",FFT_redRowKern[i]);
  }

  fprintf(fid, "\nGREEN:\n");
  for (i=0; i<fourierCols*2; i++){
    fprintf(fid, "%.4f\t",FFT_greenColKern[i]);
  }
  fprintf(fid,"\n");
  for (i=0; i<2*(fourierRows/2+1); i++){
    fprintf(fid, "%.4f\t",FFT_greenRowKern[i]);
  }

  fprintf(fid, "\nBLUE:\n");
  for (i=0; i<fourierCols*2; i++){
    fprintf(fid, "%.4f\t",FFT_blueColKern[i]);
  }
  fprintf(fid,"\n");
  for (i=0; i<2*(fourierRows/2+1); i++){
    fprintf(fid, "%.4f\t",FFT_blueRowKern[i]);
  }
  fprintf(fid,"\n");
  fclose(fid);
  return;
}


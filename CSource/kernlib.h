#ifndef __kernlib_h
#define __kernlib_h


#include <fftw3.h>
#include "imglib.h"
/*
 *    KERNLIB header file
 *
 *    Copyright (c) Bob Dougherty 2000
 *                  Alex Wade 2000
 *    kernel class is derived from img class.
 *    Has an additional function to generate real space fftshifted gaussians 
 *    Also - Direct Fourier space kernel generation is planned.
 */



class kernel: public img{
 public:
  kernel(int rows, int cols, int hasFFT); // Can explicitly allocate FFT space on construction
  void setKern(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale);
  void setKernFFT(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale);
 private:

};

class kernelSep;

class kernelSep{

 public:

  kernelSep(int rows, int cols);
  //kernelSep::~kernelSep(void);

  void setKernFFT(int kNum, float kW1, float kSD1, float kW2, float kSD2, float kW3, float kSD3, float scale);

  float *FFT_redColKern;
  float *FFT_redRowKern;
  float *FFT_greenColKern;
  float *FFT_greenRowKern;
  float *FFT_blueColKern;
  float *FFT_blueRowKern;

  int fourierRows, fourierCols, fourierRowsComplex, fourierColsComplex;

  void writeRawFFT(const char *fileName);
	
 private:

};


#endif // __kernlib_h

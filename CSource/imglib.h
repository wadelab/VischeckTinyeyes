#ifndef __imglib_h
#define __imglib_h

/*
 *    IMGLIB header file
 *
 *    Copyright (c) Bob Dougherty 2000
 *		changes ARW 02/05/00 : 
 *		Changed private to protected in img: so that kernel class can be derived more easily
 *		Added FFT functionality to img class. 
 *		Derived kernel class from img
 *		Added fftshifted real-space gaussian generation to kernel class 
 *		Some minor changes (added to enable Win32 compilation - see ifdef in runSimulation.cxx)
 *		Proposed: Change red,green,blue etc to plane1,plane2,plane3.
 */		

#include <fftw3.h>

#define FOURIER_SPACE 1
#define REAL_SPACE 0
#define HAS_FFT 1
#define NO_FFT 0

#define PAD_PROPORTION .05

#define forwardPlanFile  "FFT_for_plan.pln"
#define backwardPlanFile "FFT_back_plan.pln"

// color space labels:
enum colorSpaceLabelType {RGB, LMS, OPP};


class img;

class img {
protected:
	float *red;
	float *green;
	float *blue;

	// These planes hold FFT data only
	float *FFT_red;
	float *FFT_green;
	float *FFT_blue;

	int r, c;
	int fourierRows, fourierCols, fourierRowsTotal;
	int npix, nFourierPix;
	float maxImgVal;
	int FFT_MEMORY_ALLOCATED; // Memory for the FFT data is allocated by the constructor only if required
	int allocateFFTspace();
public:
	img() {}
	img(int rows, int cols);
	img(int rows, int cols, float maxImageValue);
	img(int rows, int cols, int hasFFT); // Can explicitly allocate FFT space on construction
	~img();

	colorSpaceLabelType colorSpaceLabel;

	int getRows() {return r;}
	int getCols() {return c;}
	int getFourierRows() {return fourierRows;}
	int getFourierCols() {return fourierCols;}
	int getNpix() {return npix;}
	float getMaxImgVal() {return maxImgVal;}

	void assignUchar(unsigned char *dataPtr);
	void extractUchar(unsigned char *dataPtr);

	void assignUchar(unsigned char *dataPtr, const float scale);
	void extractUchar(unsigned char *dataPtr, const float scale);

	float getRedVal(const int row, const int col) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) return red[row*c+col]; else return -999;}
	float getGreenVal(const int row, const int col) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) return green[row*c+col]; else return -999;}
	float getBlueVal(const int row, const int col) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) return blue[row*c+col]; else return -999;}

	float getRedVal(const int pixnum) 
			{if ((pixnum<npix)&&(pixnum>=0)) return red[pixnum]; else return -999;}
	float getGreenVal(const int pixnum) 
			{if ((pixnum<npix)&&(pixnum>=0)) return green[pixnum]; else return -999;}
	float getBlueVal(const int pixnum) 
			{if ((pixnum<npix)&&(pixnum>=0)) return blue[pixnum]; else return -999;}

	void setRedVal(const int row, const int col, float val) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) red[row*c+col] = val;}
	void setGreenVal(const int row, const int col, float val) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) green[row*c+col] = val;}
	void setBlueVal(const int row, const int col, float val) 
			{if ((row<r)&&(row>=0)&&(col<c)&&(col>=0)) blue[row*c+col] = val;}

	void setRedVal(const int pixnum, float val) 
			{if ((pixnum<npix)&&(pixnum>=0)) red[pixnum] = val;}
	void setGreenVal(const int pixnum, float val) 
			{if ((pixnum<npix)&&(pixnum>=0)) green[pixnum] = val;}
	void setBlueVal(const int pixnum, float val) 
			{if ((pixnum<npix)&&(pixnum>=0)) blue[pixnum] = val;}

	void changeColorSpace(float transformMatrix[]);
	void changeColorSpace4Matrix(float tm[]);

	void applyLookupTable(float *tableR, float *tableG, float *tableB);
	void clipValRange();
	void scaleValRange();

	void brettelTransform(char viewerType, float rgb2lms[]);

	void computeDaltonize(float outMat[], float lmStretch, float lumScale, float sScale);
	void daltonize(float lumScale, float sScale, float lmStretch);
	void daltonize(float lumScale, float sScale, float lmStretch, float *xform);
	int doFFT(int direction);
	void dotMultiplyFFT(img Multiplier);
	void dotMultiplyFFT(class kernelSep Multiplier);

	void writeRaw(const char *fileName);
	void writeRawFFT(const char *fileName);
};

#endif // __imglib_h

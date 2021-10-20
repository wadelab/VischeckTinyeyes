#include "imglib.h"
#include "kernlib.h"
#include "colorTools.h"
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fftw3.h>
#include "jama/tnt_array1d.h"
#include "jama/tnt_array2d.h"
#include "jama/tnt_array1d_utils.h"
#include "jama/tnt_array2d_utils.h"

img::img(int rows, int cols)
{
  maxImgVal = 255.0;	// assume 0-255 image values
  r = rows;
  c = cols;
  npix = r*c;
  fourierRows = 0;
  fourierCols = 0;
  nFourierPix = 0;
  FFT_MEMORY_ALLOCATED = 0;
  colorSpaceLabel = RGB;

  // Allocate one big block of memory, then divy it up ourselves.
  // (makes processor caching more effective? seems to provide ~ 20% speed gain.)
  red = new float [npix*3];
  green = red+npix;
  blue = green+npix;

  if (red==NULL){
    r = c = npix = 0;
  }
  else{
    r = rows;
    c = cols;
  }

  FFT_red = FFT_green = FFT_blue = NULL;

  return;
}

img::img(int rows, int cols, float maxImageValue)
{
  maxImgVal = maxImageValue;
  r = rows;
  c = cols;
  npix = r*c;
  fourierRows = 0;
  fourierCols = 0;
  nFourierPix = 0;
  FFT_MEMORY_ALLOCATED = 0;
  colorSpaceLabel = RGB;

  red = new float [npix*3];
  green = red+npix;
  blue = green+npix;

  if (red==NULL){
    r = c = npix = 0;
  }

  FFT_red = FFT_green = FFT_blue = NULL;

  return;
}

img::img(int rows, int cols, int hasFFT)
{ // Here we also allocate space for FFT
  maxImgVal = 255;	// assume 0-255 image values
  r = rows;
  c = cols;
  npix = r*c;
  fourierRows = 0;
  fourierCols = 0;
  nFourierPix = 0;
  FFT_MEMORY_ALLOCATED = 0;
  colorSpaceLabel = RGB;
	
  // Allocate one big block of memory, then divy it up ourselves.
  // (makes processor caching more effective? seems to provide ~ 20% speed gain.)
  red = new float [npix*3];
  green = red+npix;
  blue = green+npix;
	
  if (red==NULL) r = c = npix = 0;

  allocateFFTspace();

  return;
}

int img::allocateFFTspace(){
  // Allocate memory for a forward real FFT. 
  // FFTW requires fourierCols x 2*floor(fourierRows/2+1).
  // We also allow for a pad of some % of the image size to reduce edge artifacts.
  int i,tmp;
	
  // allow for a pad:
  fourierRows = r + (int)(PAD_PROPORTION*r);
  fourierCols = c + (int)(PAD_PROPORTION*c);

  // Go up to an optimized number of rows and cols.
  // Here are all of the optimal values (values of n where max(factor(n))<=7).
  // Note that I've biased things a bit torward powers of 2 by removing:
  // 	*g++=63;
  int goodVals[167], *g=goodVals;
  *g++=32;*g++=35;*g++=36;*g++=40;*g++=42;*g++=45;*g++=48;*g++=49;*g++=50;*g++=54;
  *g++=56;*g++=60;*g++=64;*g++=70;*g++=72;*g++=75;*g++=80;*g++=81;*g++=84;*g++=90;
  *g++=96;*g++=98;*g++=100;*g++=105;*g++=108;*g++=112;*g++=120;*g++=125;*g++=126;*g++=128;
  *g++=135;*g++=140;*g++=144;*g++=147;*g++=150;*g++=160;*g++=162;*g++=168;*g++=175;*g++=180;
  *g++=189;*g++=192;*g++=196;*g++=200;*g++=210;*g++=216;*g++=224;*g++=225;*g++=240;*g++=243;
  *g++=245;*g++=250;*g++=252;*g++=256;*g++=270;*g++=280;*g++=288;*g++=294;*g++=300;*g++=315;
  *g++=320;*g++=324;*g++=336;*g++=343;*g++=350;*g++=360;*g++=375;*g++=378;*g++=384;*g++=392;
  *g++=400;*g++=405;*g++=420;*g++=432;*g++=441;*g++=448;*g++=450;*g++=480;*g++=486;*g++=490;
  *g++=500;*g++=504;*g++=512;*g++=525;*g++=540;*g++=560;*g++=567;*g++=576;*g++=588;*g++=600;
  *g++=625;*g++=630;*g++=640;*g++=648;*g++=672;*g++=675;*g++=686;*g++=700;*g++=720;*g++=729;
  *g++=735;*g++=750;*g++=756;*g++=768;*g++=784;*g++=800;*g++=810;*g++=840;*g++=864;*g++=875;
  *g++=882;*g++=896;*g++=900;*g++=945;*g++=960;*g++=972;*g++=980;*g++=1000;*g++=1008;*g++=1024;
  *g++=1029;*g++=1050;*g++=1080;*g++=1120;*g++=1125;*g++=1134;*g++=1152;*g++=1176;*g++=1200;*g++=1215;
  *g++=1225;*g++=1250;*g++=1260;*g++=1280;*g++=1296;*g++=1323;*g++=1344;*g++=1350;*g++=1372;*g++=1400;
  *g++=1440;*g++=1458;*g++=1470;*g++=1500;*g++=1512;*g++=1536;*g++=1568;*g++=1575;*g++=1600;*g++=1620;
  *g++=1680;*g++=1701;*g++=1715;*g++=1728;*g++=1750;*g++=1764;*g++=1792;*g++=1800;*g++=1875;*g++=1890;
  *g++=1920;*g++=1944;*g++=1960;*g++=2000;*g++=2016;*g++=2025;*g++=2048;	
  tmp = 0;
  if (fourierRows < goodVals[0]) fourierRows = goodVals[0];
  if (fourierCols < goodVals[0]) fourierCols = goodVals[0];
  for (i=1;i<167;i++){
    if (fourierRows < goodVals[i] & fourierRows > goodVals[i-1]){
      fourierRows = goodVals[i];
      tmp++;
    }
    if (fourierCols < goodVals[i] & fourierCols > goodVals[i-1]){
      fourierCols = goodVals[i];
      tmp++;
    }
    if (tmp>1) break;
  }
  std::cerr <<"fourier-r,c="<<fourierRows<<","<<fourierCols<<std::endl;
  // rfft wants fourierCols columns and 2*floor(fourierRows/2+1) rows:
  fourierRowsTotal = 2*(int)(fourierRows/2+1);
  nFourierPix = fourierRowsTotal*fourierCols;

  FFT_red = new float [nFourierPix*3];
	
  if (FFT_red==NULL){
    FFT_MEMORY_ALLOCATED = 0;
    FFT_green = NULL;
    FFT_blue = NULL;
    return(-1);
  }
  else {
    FFT_MEMORY_ALLOCATED = 1;
    FFT_green = FFT_red+nFourierPix;
    FFT_blue = FFT_green+nFourierPix;
    return(1);
  }	
} // end fn

img::~img()
{
  // *** why can't we let the destructor fee memory?  If the following is
  // uncommented, we get a segment fault near the end of the runSimulation loop!

  //	We allocated red, green and blue as one big block, so freeing
  //	the red frees green and blue as well
  //	delete red;
}

void img::assignUchar(unsigned char *dataPtr)
{
  int i;
  float *rtmp, *gtmp, *btmp;

  rtmp = red;
  gtmp = green;
  btmp = blue;

  // With gcc 2.95.2, I get a weird assembler error ("instruction filds is not a 386 instruction")
  // if I don't do "*rtmp++ = (float)(1.0 * *dataPtr++);" below.

  for (i=npix-1; i>=0; i--){
    *rtmp++ = (float)(*dataPtr++);
    *gtmp++ = (float)(*dataPtr++);
    *btmp++ = (float)(*dataPtr++);
  }    
  return;
}

void img::extractUchar(unsigned char *dataPtr)
{
  int i;
  float *rtmp, *gtmp, *btmp;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;
	
  for (i=npix-1; i>=0; i--){
    *dataPtr++ = (unsigned char)((*rtmp++) + .5);
    *dataPtr++ = (unsigned char)((*gtmp++) + .5);
    *dataPtr++ = (unsigned char)((*btmp++) + .5);
  }
  return;
}

void img::assignUchar(unsigned char *dataPtr, const float scale)
{
  int i;
  float *rtmp, *gtmp, *btmp;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;

  for (i=npix-1; i>=0; i--){
    *rtmp++ = (float)((*dataPtr++) / scale);
    *gtmp++ = (float)((*dataPtr++) / scale);
    *btmp++ = (float)((*dataPtr++) / scale);
  }    
  return;
}

void img::extractUchar(unsigned char *dataPtr, const float scale)
{
  int i;
  float *rtmp, *gtmp, *btmp;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;
	
  for (i=npix-1; i>=0; i--){
    *dataPtr++ = (unsigned char)((*rtmp++) * scale + .5);
    *dataPtr++ = (unsigned char)((*gtmp++) * scale + .5);
    *dataPtr++ = (unsigned char)((*btmp++) * scale + .5);
  }
  return;
}

void img::changeColorSpace(float tm[]){
  // post-multiply by tm' to convert the pixels to the output color space
  float redOld, greenOld;
  int i;
  float *rtmp, *gtmp, *btmp;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;

  for (i=npix-1; i>=0; i--){
    redOld = *rtmp;
    greenOld = *gtmp;
    *rtmp = redOld*tm[0] + greenOld*tm[1] + (*btmp)*tm[2];
    *gtmp = redOld*tm[3] + greenOld*tm[4] + (*btmp)*tm[5];
    *btmp = redOld*tm[6] + greenOld*tm[7] + (*btmp)*tm[8];
    btmp++; gtmp++; rtmp++;
  }
  return;
}

void img::applyLookupTable(float *tableR, float *tableG, float *tableB){
  // this function assumes that the image data are 0-imgValMax and that the look-up
  // table length is equal to imgValMax.
  float *rtmp, *gtmp, *btmp;
  int i;

  rtmp = red;
  gtmp = green;
  btmp = blue;

  for(i=npix-1; i>=0; i--){
    *rtmp = tableR[(int)(*rtmp + 0.5)];
    *gtmp = tableG[(int)(*gtmp + 0.5)];
    *btmp = tableB[(int)(*btmp + 0.5)];
    btmp++; gtmp++; rtmp++;
  }

  // Unrolling these loops seems to speed em up a bit, 
  // but the follow code doesn't quite work ;)
  //   int m = npix>>2;
  //   int n = npix&3;
  //   switch (m){
  //     while(n--){
  //       *rtmp = tableR[(int)(*rtmp++ +0.5)];
  //     case 3:	*rtmp = tableR[(int)(*rtmp++ +0.5)];
  //     case 2:	*rtmp = tableR[(int)(*rtmp++ +0.5)];
  //     case 1:	*rtmp = tableR[(int)(*rtmp++ +0.5)];
  //     }
  //   }
  //   n = npix&3;
  //   switch (m){
  //     while(n--){
  //       *gtmp = tableG[(int)(*gtmp++ +0.5)];
  //     case 3:	*gtmp = tableG[(int)(*gtmp++ +0.5)];
  //     case 2:	*gtmp = tableG[(int)(*gtmp++ +0.5)];
  //     case 1:	*gtmp = tableG[(int)(*gtmp++ +0.5)];
  //     }
  //   }
  //   n = npix&3;
  //   switch (m){
  //     while(n--){
  //       *btmp = tableB[(int)(*btmp++ +0.5)];
  //     case 3:	*btmp = tableB[(int)(*btmp++ +0.5)];
  //     case 2:	*btmp = tableB[(int)(*btmp++ +0.5)];
  //     case 1:	*btmp = tableB[(int)(*btmp++ +0.5)];
  //     }
  //   }
  return;
}

void img::clipValRange()
{
  // this function ensures that the image data are 0-imgValMax.
  float *rtmp, *gtmp, *btmp;
  int i;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;
	
  for (i=npix-1; i>=0; i--){
    if (*rtmp>maxImgVal) *rtmp = maxImgVal;
    if (*gtmp>maxImgVal) *gtmp = maxImgVal;
    if (*btmp>maxImgVal) *btmp = maxImgVal;
    if (*btmp<0.0) *btmp = 0.0;
    if (*gtmp<0.0) *gtmp = 0.0;
    if (*rtmp<0.0) *rtmp = 0.0;
    rtmp++;
    gtmp++;
    btmp++;
  } 
  return;
}

void img::scaleValRange()
{
  // this function ensures that the image data are 0-imgValMax.
  // unlike clipValRange, this one scales all 3 values in the 
  // offending RGB triplet down to the correct range.
  float *rtmp, *gtmp, *btmp, scale;
  int i;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;
	
  for (i=npix-1; i>=0; i--){
    if (*rtmp>maxImgVal || *gtmp>maxImgVal || *btmp>maxImgVal){
      if (*rtmp>*gtmp && *rtmp>*btmp)
	scale = maxImgVal/(*rtmp);
      else if (*gtmp>*rtmp && *gtmp>*btmp)
      	scale = maxImgVal/(*gtmp);
      else
	scale = maxImgVal/(*btmp);
      *rtmp *= scale;
      *gtmp *= scale;
      *btmp *= scale;
    }

    if (*rtmp<0.0 || *gtmp<0.0 || *btmp<0.0){
      if (*rtmp<*gtmp && *rtmp<*btmp)
	scale = *rtmp;
      else if (*gtmp<*rtmp && *gtmp<*btmp)
      	scale = *gtmp;
      else
	scale = *btmp;
      *rtmp -= scale;
      *gtmp -= scale;
      *btmp -= scale;
    }
    rtmp++;
    gtmp++;
    btmp++;
  } 
  return;
}

void img::changeColorSpace4Matrix(float tm[]){
  // pre-multiply by tm to convert the pixels to the output color space
  // This is similar to changeColorSpace except that we can use a 4x4 matrix
  // to include translations as well as all the tranforms possible with a 3x3,
  // plus it is a pre-multipy convention.
  
  float redOld, greenOld;
  int i;
  float *rtmp, *gtmp, *btmp;
	
  rtmp = red;
  gtmp = green;
  btmp = blue;
  

  for (i=npix-1; i>=0; i--){
    redOld = *rtmp;
    greenOld = *gtmp;
    *rtmp = redOld*tm[0] + greenOld*tm[4] + (*btmp)*tm[ 8] + tm[12];
    *gtmp = redOld*tm[1] + greenOld*tm[5] + (*btmp)*tm[ 9] + tm[13];
    *btmp = redOld*tm[2] + greenOld*tm[6] + (*btmp)*tm[10] + tm[14];
    
    btmp++; gtmp++; rtmp++;
  }
  return;
}


void img::computeDaltonize(float *outMat, float lmStretch, float lumScale, float sScale){
  // Computes a 4x4 matrix from an LMS space image
  // The matrix is composed of two parts: a stretch that increases the LM contrast
  // and a projection from the LM plane to the S and L+M planes. 
  // I think that these can all be rolled into one:
  // If the original image is I(n*3) then the final image I' is computed as some weighted sum of
  // I1 (I*Istretch)  + 
  // I2 (I*IprojectToLM) +
  // I3 (I*IprojectToS)
  // I'=I*(Istretch+IprojectToLM+IprojectToS)
  // Then applies this matrix using changeColorSpace4Matrix
  // Assumes that the image is in opponent (OPP) color space.
  
  //float Istretch[16];
  //float IprojectToLM[16];
  //float IprojectToS[16];
  float amountToLM;
  float amountToS;
   
  float tempDiff;  
  float meanVector[3]; // Holds the means of each (opponent) color plane
  float varVector[3]; // Holds the variances of each (opponent) color plane.
  long int i; 
   
  //float maxVal[3]; // Store the max and min vals of each plane 
  //float minVal[3];
   
  // Loop through all the color planes computing their variances.
  // Have to compute the mean first, then the var. 
  // In Java we used the JAMA numeric library. This was largely overkill. 
  // I don't think we need to do SVD decomposition - just compute the variance 
  // in the different image planes.
   
   // Zero the accumulators first..
    for (int t=0;t<3;t++) {
      varVector[t]=0;
      meanVector[t]=0;
    }
   
    for (i=npix-1; i>=0; i--){
      meanVector[0]+=red[i];
      meanVector[1]+=green[i];
      meanVector[2]+=blue[i];
    }
    
    meanVector[0]=meanVector[0]/npix;
    meanVector[1]=meanVector[1]/npix;
    meanVector[2]=meanVector[2]/npix;
     
    // Compute the sum of squares 
    for (i=npix-1; i>=0; i--){
      tempDiff=(red[i]-meanVector[0]);
      varVector[0]+=(tempDiff*tempDiff);
        
      tempDiff=(green[i]-meanVector[1]);
      varVector[1]+=(tempDiff*tempDiff);
         
      tempDiff=(blue[i]-meanVector[2]);
      varVector[2]+=(tempDiff*tempDiff);    
    }  
    
    // We could just factor npix in to the subsequent calculations but it's worth spending 
    // a few cycles to keep things simple. 
    varVector[0]=varVector[0]/npix;
    varVector[1]=varVector[1]/npix;
    varVector[2]=varVector[2]/npix;
    
    // we now have the variance of each color plane (L+M), (L-M) and S
    // add a certain amount of the L-M plane to both the L+M and S planes
    // amountToLM=(varVector[1]/varVector[0])*lumScale*1000;
    // amountToS=(varVector[1]/varVector[2])*sScale*10;
   
    amountToLM = -lumScale*50/(varVector[0]+1); 
    // The minus bit here makes reds brighter. Which seems right somehow.
    amountToS = -sScale*20/(varVector[2]+1); 
    // This also seems right. But both of these polarities could be placed under user control.
    
    // The final transform matrix is computed as a sum of many xforms:
    // I + LMStretchXForm + PTLumXForm + PTSXForm
   
    // A crucial step (I think) is removing the mean from the L-M axis before 
    // adding it to the other axes
    // or stretching it.
    // This also doubles as the MeanAddition matrix later:
    TNT::Array2D<double> subtractRGMeanMat(4,4); 
    TNT::Array2D<double> xformMat(4,4);  
   
    for(int i=0;i<4;i++) for(int j=0;j<4; j++) xformMat[j][i]=0;
    for(int i=0;i<4;i++) for(int j=0;j<4; j++) subtractRGMeanMat[j][i]=0;
   
    // Set ones on the diagonals
    for(int i=0;i<4;i++){
      subtractRGMeanMat[i][i]=1;
      xformMat[i][i]=1;
    }
   
    xformMat[1][0] = amountToLM; 
    xformMat[1][1] = (lmStretch-1)/4+1;
    xformMat[1][2] = amountToS;
  
    subtractRGMeanMat[3][1]=-meanVector[1]; // This doesn't seem to do anything. But it should be important. wassup?
    xformMat=TNT::matmult(subtractRGMeanMat,xformMat);
    subtractRGMeanMat[3][1]=meanVector[1]; // Change the subtract matrix to an add
    xformMat=TNT::matmult(xformMat,subtractRGMeanMat);
   
    // Place everything back into a regular float array
    for(int i=0;i<4;i++) for(int j=0;j<4; j++) outMat[i*4+j]=xformMat[j][i];
    // Apply the transform
    std::cerr <<outMat[0]<<", "<<outMat[1]<<", "<<outMat[2]<<", "<<outMat[3]<<std::endl;
    std::cerr <<outMat[4]<<", "<<outMat[5]<<", "<<outMat[6]<<", "<<outMat[7]<<std::endl;
    std::cerr <<outMat[8]<<", "<<outMat[9]<<", "<<outMat[10]<<", "<<outMat[11]<<std::endl;
    std::cerr <<outMat[12]<<", "<<outMat[13]<<", "<<outMat[14]<<", "<<outMat[15]<<std::endl;
    //changeColorSpace4Matrix(outMat);
}

void img::brettelTransform(char viewerType, float rgb2lms[]) {
  // Assumes that the image is in LMS space
  float anchor_e[3], anchor[12];
  float a1,b1,c1,a2,b2,c2,inflectionVal,tmp;
  int i;
    
  // Performs protan, deutan or tritan color image simulation based on 
  // Brettel, Vienot and Mollon JOSA 14/10 1997
  // L,M,S for lambda=475,485,575,660
  // 0.08008    0.1579    0.5897
  // 0.1284    0.2237    0.3636
  // 0.9856    0.7325  0.001079
  // 0.0914  0.007009         0

  // Load the LMS anchor-point values for lambda = 475 & 485 nm (for protans & deutans)
  // and the LMS values for lambda = 575 & 660 nm (for tritans)
  // 
  anchor[0]=0.08008;anchor[1]=0.1579;anchor[2]=0.5897;
  anchor[3]=0.1284;anchor[4]=0.2237;anchor[5]=0.3636;
  anchor[6]=0.9856;anchor[7]=0.7325;anchor[8]=0.001079;
  anchor[9]=0.0914;anchor[10]=0.007009;anchor[11]=0.0;    
    
  // We also need LMS for RGB=(1,1,1)- the equal-energy point (one of our anchors)
  // (we can just peel this out of the rgb2lms transform matrix)
  anchor_e[0] = rgb2lms[0]+rgb2lms[1]+rgb2lms[2];
  anchor_e[1] = rgb2lms[3]+rgb2lms[4]+rgb2lms[5];
  anchor_e[2] = rgb2lms[6]+rgb2lms[7]+rgb2lms[8];
	    
    
  // The image is in LMS space
  // Now follow the rules....
    
  switch (viewerType) {
  case 'n':
    //		disp(TM("Normal observer - nothing to do"));
    return;
      
  case 'd':
    //		disp(TM("Doing deuteranope conversion"));
    // find a,b,c...
    // find a,b,c for lam=575nm and lam=475
    a1 = anchor_e[1]*anchor[8]-anchor_e[2]*anchor[7];
    b1 = anchor_e[2]*anchor[6]-anchor_e[0]*anchor[8];
    c1 = anchor_e[0]*anchor[7]-anchor_e[1]*anchor[6];
    a2 = anchor_e[1]*anchor[2]-anchor_e[2]*anchor[1];
    b2 = anchor_e[2]*anchor[0]-anchor_e[0]*anchor[2];
    c2 = anchor_e[0]*anchor[1]-anchor_e[1]*anchor[0];
    inflectionVal = (anchor_e[2]/anchor_e[0]);
    // split image up into two sets.
    // Set 1: regions where lambda_a=575, set 2: lambda_a=475
    // construct the two parts of the M-component 
    // from pixels which fall on differnt sides of the two 'wings'
    // *** unroll this loop
    for (i=npix-1; i>=0; i--){
      tmp = (blue[i]) / (red[i]);
      if(tmp<inflectionVal)
	green[i] = -(a1 * red[i] + c1 * blue[i]) / b1;
      else
	green[i] = -(a2 * red[i] + c2 * blue[i]) / b2;
    }
    break;
      
  case 'p':
    //		disp(TM("Doing protanope conversion"));       
    // find a,b,c for lam=575nm and lam=475
    a1 = anchor_e[1]*anchor[8]-anchor_e[2]*anchor[7];
    b1 = anchor_e[2]*anchor[6]-anchor_e[0]*anchor[8];
    c1 = anchor_e[0]*anchor[7]-anchor_e[1]*anchor[6];
    a2 = anchor_e[1]*anchor[2]-anchor_e[2]*anchor[1];
    b2 = anchor_e[2]*anchor[0]-anchor_e[0]*anchor[2];
    c2 = anchor_e[0]*anchor[1]-anchor_e[1]*anchor[0];
    inflectionVal = (anchor_e[2]/anchor_e[1]);
    // split image up into two sets.
    // Set 1: regions where lambda_a=575, set 2: lambda_a=475
    // construct the two parts of the M-component 
    // from pixels which fall on differnt sides of the two 'wings'
    for (i=npix-1; i>=0; i--){
      tmp = blue[i]/green[i];
      if(tmp<inflectionVal)
	red[i] = -(b1*green[i]+c1*blue[i])/a1;
      else
	red[i] = -(b2*green[i]+c2*blue[i])/a2;
    }
    break;
      
  case 't':
    //		disp(TM("Doing tritanope conversion"));
    // split image up into two sets.
    // Set 1: regions where lambda_a=575, set 2: lambda_a=475
    a1 = anchor_e[1]*anchor[11]-anchor_e[2]*anchor[10];
    b1 = anchor_e[2]*anchor[9]-anchor_e[0]*anchor[11];
    c1 = anchor_e[0]*anchor[10]-anchor_e[1]*anchor[9];
    a2 = anchor_e[1]*anchor[5]-anchor_e[2]*anchor[4];
    b2 = anchor_e[2]*anchor[3]-anchor_e[0]*anchor[5];
    c2 = anchor_e[0]*anchor[4]-anchor_e[1]*anchor[3];
    inflectionVal = (anchor_e[1]/anchor_e[0]);
    for (i=npix-1; i>=0; i--){
      tmp = green[i]/red[i];
      if(tmp<inflectionVal)
	blue[i] = -(a1*red[i]+b1*green[i])/c1;
      else
	blue[i] = -(a2*red[i]+b2*green[i])/c2;
    }
    break;

  default:
    //        disp(TM("This condition is not catered for yet..."));
    std::cerr << "This condition is not catered for yet..." << viewerType << std::endl;
  } // end switch
    
}


int img::doFFT(int direction)
{
  // Function to do FFT on image data using FFTW routines. 
  // Uses the FFTW routines which allow you to hold real-space transforms in 1/2 Fourier space (since they're Hermitian)
  // This is handy as it allows us to do in-place operations on image data

  // NB - currently, we force the routine to generate a new plan each time.
  // the wisdom write to file routine causes a crash for some reason.
  // However, FFTs seem to be very fast even just using the 'ESTIMATE' option.

  // NOTE: Image pixels typically stacked row-by-row, but 
  // rfftw2d wants col-by-col, so we pretend that cols are rows and rows
  // are col:  nFourierPix should have an extra row, so we give it an extra col.
  // rfftw2d wants the first two parameters to be 'rows,cols', but we give it
  // 'cols,rows'.  It all seems to work out fine in the end...

  fftwf_plan plan;
  int i,j,index,reflect;
  float *imPtrR, *imPtrG, *imPtrB, *fftPtrR, *fftPtrG, *fftPtrB, scale;
	
  // See if FFT memory has been allocated
  if (FFT_MEMORY_ALLOCATED==0) {
    if (allocateFFTspace()<0) return (-1);
  } // end if fft is allocated

  int n[2];
  n[0] = fourierCols;
  n[1] = fourierRows;	
  if (direction==FFTW_FORWARD) {
    // Perform forward transforms
		
    // Put the image into the fourier memory space.
    // Here, we add some padding.  Reflecting the image seems to be a good
    // way to fill-in our pad region. 
    imPtrR = red;
    imPtrG = green;
    imPtrB = blue;
    fftPtrR = (float *)FFT_red;
    fftPtrG = (float *)FFT_green;
    fftPtrB = (float *)FFT_blue;
    for (i=0;i<fourierCols;i++){
      for (j=0;j<fourierRowsTotal;j++){
	index = i*fourierRowsTotal+j;
	if (i<c & j<r){
	  fftPtrR[index] = *(imPtrR++);
	  fftPtrG[index] = *(imPtrG++);
	  fftPtrB[index] = *(imPtrB++);
	}
	else{	
	  // fill this part with a reflection
	  if (i>c & j>r) reflect = (i-c)*fourierRowsTotal+j-r;
	  else if (i<c) reflect = i*fourierRowsTotal+j-r;
	  else reflect = (i-c)*fourierRowsTotal+j;
	  fftPtrR[index] = fftPtrR[reflect];
	  fftPtrG[index] = fftPtrG[reflect];
	  fftPtrB[index] = fftPtrB[reflect];
	}
      }
    }

    plan = fftwf_plan_many_dft_r2c(2, (const int *)&n,
			       3, // howmany
			       (float *)FFT_red,
			       NULL, // inembed
			       1, // istride
			       nFourierPix, // idist
			       (fftwf_complex *)FFT_red,  //(float (*)[2])FFT_red,
			       NULL, // onembed
			       1,
			       nFourierPix/2, // odist
			       FFTW_ESTIMATE);

    if(plan == NULL) std::cout << "can't create plan - error !!!" << std::endl;

    //fftwf_print_plan(plan);
    // Have a plan, have some data, now do the forward transforms...
    //
    // CRASH HERE
    // Works fine for howmany=1, but not howmany=3.
    // I think the problem is that odist (idist for c2r below) is in units 
    // of fftwf_complex, NOT float.
    fftwf_execute(plan);

    std::cerr << "nFourierPix=" << nFourierPix << std::endl;


    // destroy the plan	
    //
    fftwf_destroy_plan(plan);


    // That's it!
	
  }else{
    // Doing the back transform. This is similar to the forward one except that there's a division at the end
			
    plan = fftwf_plan_many_dft_c2r(2, (const int *)&n,
			       3, // howmany
			       (float (*)[2])FFT_red,
			       NULL, // inembed
			       1, // istride
			       nFourierPix/2, // idist
			       (float *)FFT_red,
			       NULL, // onembed
			       1,
				   nFourierPix,   // odist
			       FFTW_ESTIMATE);
    if(plan == NULL) std::cout << "can't create plan - error !!!" << std::endl;

    // Have a plan, have some data, now do the forward transforms...
    //
    fftwf_execute(plan);

    // destroy the plan     
    //
    fftwf_destroy_plan(plan);
    // end if (opened wisdom file)*/
		
    // Finally, have to divide all the elements by npix;
    // Put the image back into into the image memory space, 
    // dividing by the appropriate scale factor (total number of pixels) along the way...
    scale = fourierRows*fourierCols;
    //		scale = npix;
    imPtrR = red;
    imPtrG = green;
    imPtrB = blue;
    fftPtrR = (float *)FFT_red;
    fftPtrG = (float *)FFT_green;
    fftPtrB = (float *)FFT_blue;
    for (i=0;i<c;i++){
      for (j=0;j<r;j++){
	index = i*fourierRowsTotal+j;
	*(imPtrR++) = fftPtrR[index]/scale;
	*(imPtrG++) = fftPtrG[index]/scale;
	*(imPtrB++) = fftPtrB[index]/scale;
      }
    }

  } // end if FFTW_FORWARD

  return (1);
	
} // end fn


void img::dotMultiplyFFT(img Multiplier) {
  // Dot multiply the complex FFT components...
  fftwf_complex tmp, *multR, *multG, *multB, *R, *G, *B;
  int i,j,ij;
	
  multR = (fftwf_complex *)Multiplier.FFT_red;
  multG = (fftwf_complex *)Multiplier.FFT_green;
  multB = (fftwf_complex *)Multiplier.FFT_blue;
  R = (fftwf_complex *)FFT_red;
  G = (fftwf_complex *)FFT_green;
  B = (fftwf_complex *)FFT_blue;
  for (i=0;i<fourierCols;i++){
    for (j=0;j<fourierRows/2+1;j++){
      ij = (fourierRows/2+1) * i + j;
      tmp[0] = multR[ij][0] * R[ij][0] - multR[ij][1] * R[ij][1];
      tmp[1] = multR[ij][0] * R[ij][1] + multR[ij][1] * R[ij][0];
      R[ij][0] = tmp[0]; R[ij][1] = tmp[1];
      tmp[0] = multG[ij][0] * G[ij][0] - multG[ij][1] * G[ij][1];
      tmp[1] = multG[ij][0] * G[ij][1] + multG[ij][1] * G[ij][0];
      G[ij][0] = tmp[0]; G[ij][1] = tmp[1];
      tmp[0] = multB[ij][0] * B[ij][0] - multB[ij][1] * B[ij][1];
      tmp[1] = multB[ij][0] * B[ij][1] + multB[ij][1] * B[ij][0];
      B[ij][0] = tmp[0]; B[ij][1] = tmp[1];
    }
  }
  return;		
} // end fn



void img::dotMultiplyFFT(kernelSep Multiplier) 
{
  // Dot multiply the complex FFT components for a row,col separable kernel 

  fftwf_complex *multColR, *multColG, *multColB, *multRowR, *multRowG, *multRowB;
  fftwf_complex tmp, *R, *G, *B;
  int i, j, ij;
  multColR = (fftwf_complex *)Multiplier.FFT_redColKern;
  multColG = (fftwf_complex *)Multiplier.FFT_greenColKern;
  multColB = (fftwf_complex *)Multiplier.FFT_blueColKern;
  multRowR = (fftwf_complex *)Multiplier.FFT_redRowKern;
  multRowG = (fftwf_complex *)Multiplier.FFT_greenRowKern;
  multRowB = (fftwf_complex *)Multiplier.FFT_blueRowKern;
  R = (fftwf_complex *)FFT_red;
  G = (fftwf_complex *)FFT_green;
  B = (fftwf_complex *)FFT_blue;
  for (i=0;i<fourierRows/2+1;i++){
    for (j=0;j<fourierCols;j++){
      ij = (fourierRows/2+1) * j + i;
      tmp[0] = multRowR[i][0] * R[ij][0] - multRowR[i][1] * R[ij][1]; 
      tmp[1] = multRowR[i][0] * R[ij][1] + multRowR[i][1] * R[ij][0];
      tmp[0] = multColR[j][0] * tmp[0] - multColR[j][1] * tmp[1]; 
      tmp[1] = multColR[j][0] * tmp[1] + multColR[j][1] * tmp[0];
      R[ij][0] = tmp[0]; R[ij][1] = tmp[1];
      tmp[0] = multRowG[i][0] * G[ij][0] - multRowG[i][1] * G[ij][1];
      tmp[1] = multRowG[i][0] * G[ij][1] + multRowG[i][1] * G[ij][0];
      tmp[0] = multColG[j][0] * tmp[0] - multColG[j][1] * tmp[1];
      tmp[1] = multColG[j][0] * tmp[1] + multColG[j][1] * tmp[0];
      G[ij][0] = tmp[0]; G[ij][1] = tmp[1];
      tmp[0] = multRowB[i][0] * B[ij][0] - multRowB[i][1] * B[ij][1];
      tmp[1] = multRowB[i][0] * B[ij][1] + multRowB[i][1] * B[ij][0];
      tmp[0] = multColB[j][0] * tmp[0] - multColB[j][1] * tmp[1];
      tmp[1] = multColB[j][0] * tmp[1] + multColB[j][1] * tmp[0];
      B[ij][0] = tmp[0]; B[ij][1] = tmp[1];
    }
  }
  return;	
} // end fn

void img::writeRaw(const char *fileName)
{
  int i,j,ij;
  FILE *fid;

  fid = fopen(fileName, "wt");
  fprintf(fid, "RED:\n");
  for (i=0; i<c; i++){
    for (j=0; j<r; j++){
      ij = r * i + j;
      fprintf(fid, "%.4f\t",red[ij]);
    }
    fprintf(fid,"\n");
  }
  fprintf(fid, "GREEN:\n");
  for (i=0; i<c; i++){
    for (j=0; j<r; j++){
      ij = r * i + j;
      fprintf(fid, "%.4f\t",green[ij]);
    }
    fprintf(fid,"\n");
  }
  fprintf(fid, "BLUE:\n");
  for (i=0; i<c; i++){
    for (j=0; j<r; j++){
      ij = r * i + j;
      fprintf(fid, "%.4f\t",blue[ij]);
    }
    fprintf(fid,"\n");
  }

  fclose(fid);
  return;
}

void img::writeRawFFT(const char *fileName)
{
  int i,j,ij;
  FILE *fid;

  fid = fopen(fileName, "wt");
  fprintf(fid, "RED:\n");
  for (i=0; i<fourierCols; i++){
    for (j=0; j<fourierRowsTotal; j++){
      ij = fourierRowsTotal * i + j;
      fprintf(fid, "%.4f\t",FFT_red[ij]);
    }
    fprintf(fid,"\n");
  }
  fprintf(fid, "GREEN:\n");
  for (i=0; i<fourierCols; i++){
    for (j=0; j<fourierRowsTotal; j++){
      ij = fourierRowsTotal * i + j;
      fprintf(fid, "%.4f\t",FFT_green[ij]);
    }
    fprintf(fid,"\n");
  }
  fprintf(fid, "BLUE:\n");
  for (i=0; i<fourierCols; i++){
    for (j=0; j<fourierRowsTotal; j++){
      ij = fourierRowsTotal * i + j;
      fprintf(fid, "%.4f\t",FFT_blue[ij]);
    }
    fprintf(fid,"\n");
  }

  fclose(fid);
  return;
}

void img::daltonize(float lumScale, float sScale, float lmStretch){
     float xform[16];
     daltonize(lumScale, sScale, lmStretch, xform);
     changeColorSpace4Matrix(xform);
     clipValRange();
}

void img::daltonize(float lumScale, float sScale, float lmStretch, float *xform){
  // Inputs are always 0-1- it's up to us to scale them to the apropriate range.
  lmStretch = lmStretch*2.0+1.0;

  // Create a display struct
  displayDevice myDisp("CRT");
  float *r2o = myDisp.getRGB2OPP();
  float *o2r = myDisp.getOPP2RGB();

  // Go to Opponent colors space
  switch (colorSpaceLabel){
  case LMS: changeColorSpace(myDisp.getLMS2OPP()); break;
  case RGB: 
    // Apply gamma (transform RGB values to luminance values)
    applyLookupTable(myDisp.gammaPtrR(), myDisp.gammaPtrG(), myDisp.gammaPtrB());
    changeColorSpace(myDisp.getRGB2OPP()); 
    break;
  case OPP: break;
  }  
  colorSpaceLabel = OPP;
  // Do Daltonize xform with some reasonable params

  // The logic here is that we want to be able to extract just the transform matrix
  // so that we can appy it later in, say, an openGL routine.
  computeDaltonize(xform,lmStretch,lumScale,sScale); 
  // Stretch, project Lum, project S

  //changeColorSpace4Matrix(xform);

  // Here we compute the final xform matrix for the openGL routine.
  // The xform we currently have will operate on opponent images
  // But we need something you can apply to RGB images.
  // So : [[C*RGB2OPP]*xform]*OPP2RGB
  // Since matrix multiplication is associative, we want the following matrix:

  // xform = r2o*xform*o2r;

  // We have gone to the trouble of including TNT libs so...

  TNT::Array2D<double> xformMat(4,4);
  TNT::Array2D<double> R2O(4,4);
  TNT::Array2D<double> O2R(4,4);

   for(int i=0;i<4;i++) for(int j=0;j<4; j++) xformMat[j][i] = xform[i*4+j];
   for(int i=0;i<3;i++) for(int j=0;j<3; j++) R2O[j][i] = r2o[i*3+j];
   for(int i=0;i<3;i++) for(int j=0;j<3; j++) O2R[j][i] = o2r[i*3+j];

   // Zero the bottom row and last column
   for(int i=0;i<4;i++){
           R2O[i][3]=0; R2O[3][i]=0;
           O2R[i][3]=0; O2R[3][i]=0;
    }

  // Perform two matrix multiplies to combine the transforms
  xformMat=TNT::matmult(R2O,xformMat);
  xformMat=TNT::matmult(xformMat,O2R); // Can we do this in-place?

   // Copy the data back into the xform array that we passed a pointer to
   for(int i=0;i<4;i++) for(int j=0;j<4; j++) xform[i*4+j]=xformMat[i][j];

  // back to RGB
  changeColorSpace(o2r);
  colorSpaceLabel = RGB;
  // Clip out-of-gamut values
  //clipValRange();
  // Apply inverse gamma
  applyLookupTable(myDisp.invGammaPtrR(), myDisp.invGammaPtrG(), myDisp.invGammaPtrB());
}

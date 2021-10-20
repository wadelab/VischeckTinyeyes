#include "ImageLib.h"
#include <stdlib.h>
#include <math.h>
#include <iostream>

#include "precomp.h"

#include "ColorTools.h"
#include "jama/tnt_array1d.h"
#include "jama/tnt_array2d.h"
#include "jama/tnt_array1d_utils.h"
#include "jama/tnt_array2d_utils.h"


img::img(int rows, int cols){
	maxImgVal = 255.0;	// assume 0-255 image values
	r = rows;
	c = cols;
	npix = r*c;
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

	return;
}

img::img(int rows, int cols, float maxImageValue)
{
	maxImgVal = maxImageValue;
	r = rows;
	c = cols;
	npix = r*c;
	colorSpaceLabel = RGB;

	red = new float [npix*3];
	green = red+npix;
	blue = green+npix;

	if (red==NULL){
		r = c = npix = 0;
	}

	return;
}

img::~img()
{
//	We allocated red, green and blue as one big block, so freeing
//	the red frees green and blue as well
	delete red;
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

void img::computeDaltonizeTransformMatrix(float* outMat, float LMStretch, float lumScale, float sScale)
{   // Computes a 4x4 matrix from an LMS space image
   // The matrix is composed of two parts: a stretch that increases the LM contrast
   // and a projection from the LM plane to the S and L+M planes. 
   // I think that these can all be rolled into one:
   // If the original image is I(n*3) then the final image I' is computed as some weighted sum of
   // I1 (I*Istretch)  + 
   // I2 (I*IprojectToLM) +
   // I3 (I*IprojectToS)
   // I'=I*(Istretch+IprojectToLM+IprojectToS)
   // Then applies this matrix using changeColorSpace4Matrix
   // The matrix itself
   float Istretch[16];
   float IprojectToLM[16];
   float IprojectToS[16];
   float amountToLM;
   float amountToS;
   
   float tempDiff;  
   float meanVector[3]; // Holds the means of each (opponent) color plane
   float varVector[3]; // Holds the variances of each (opponent) color plane.
   long int i; 
   
   float maxVal[3]; // Store the max and min vals of each plane 
   float minVal[3];
   
   // Loop through all the color planes computing their variances.
   // Have to compute the mean first, then the var. 
   // In Java we used the JAMA numeric library. This was largely overkill. I don't think we need to do 
   /// SVD decomposition - just compute the variance in the different image planes.
   
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
   
   amountToLM=-lumScale*50/(varVector[0]+1); // The minus bit here makes reds brighter. Which seems right somehow.
   amountToS=-sScale*20/(varVector[2]+1); // This also seems right. But both of these polarities could be placed under user control.
    
   // The final transform matrix is computed as a sum of many xforms:
   // I + LMStretchXForm + PTLumXForm + PTSXForm
   
   // A crucial step (I think) is removing the mean from the L-M axis before adding to the other axes
   // or stretching it.
   TNT::Array2D<double> subtractRGMeanMat(4,4); // This also doubles as the MeanAddition matrix later
   TNT::Array2D<double> xformMat(4,4);  
   
   for(int i=0;i<4;i++) for(int j=0;j<4; j++) xformMat[j][i]=0;
   for(int i=0;i<4;i++) for(int j=0;j<4; j++) subtractRGMeanMat[j][i]=0;
   
   // Set ones on the diagonals
   for(int i=0;i<4;i++){
           subtractRGMeanMat[i][i]=1;
           xformMat[i][i]=1;
   }
       
   
   xformMat[1][0] = amountToLM; 
   xformMat[1][1] = (LMStretch-1)/4+1;
   xformMat[1][2] = amountToS;
  
   subtractRGMeanMat[3][1]=-meanVector[1]; // This doesn't seem to do anything. But it should be important. wassup?
   xformMat=TNT::matmult(subtractRGMeanMat,xformMat);
   subtractRGMeanMat[3][1]=meanVector[1]; // Change the subtract matrix to an add
   xformMat=TNT::matmult(xformMat,subtractRGMeanMat);
   
   // Place everything back into a regular float array
   for(int i=0;i<4;i++) for(int j=0;j<4; j++) outMat[i*4+j]=xformMat[j][i];
   
   
}

void img::brettelTransform(char viewerType, float rgb2lms[]) {

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
                assert(0);
//        disp(TM("This condition is not catered for yet..."));
          //cerr << "This condition is not catered for yet..." << viewerType << endl;
    } // end switch    
}

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

void img::brettelize(){
  // Create a display struct
  displayDevice myDisp("CRT");
  // Apply gamma (transform RGB values to luminance values)
  applyLookupTable(myDisp.gammaPtrR(), myDisp.gammaPtrG(), myDisp.gammaPtrB());
  // Go to LMS space
  changeColorSpace(myDisp.getRGB2LMS());
  colorSpaceLabel = LMS;
  // Do Brettel
  brettelTransform('d', myDisp.getRGB2LMS());
  // back to RGB
  changeColorSpace(myDisp.getLMS2RGB());
  colorSpaceLabel = RGB;
  // Clip out-of-gamut values
  clipValRange();
  // Apply inverse gamma 
  applyLookupTable(myDisp.invGammaPtrR(), myDisp.invGammaPtrG(), myDisp.invGammaPtrB());
}

void img::daltonize(float lumScale, float sScale, float lmStretch){
     float xform[16];
     daltonize(lumScale, sScale, lmStretch, xform);
     /*wxMessageBox(wxString::Format(wxT("[%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f]"),
                  xform[0],xform[1],xform[2],xform[3],xform[4],xform[5],xform[6],xform[7],
                  xform[8],xform[9],xform[10],xform[11],xform[12],xform[13],xform[14],xform[15]),
                  _T("Debug"), wxOK, NULL);*/
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
  // Apply gamma (transform RGB values to luminance values)
  applyLookupTable(myDisp.gammaPtrR(), myDisp.gammaPtrG(), myDisp.gammaPtrB());
  // Go to Opponent colors space
  changeColorSpace(r2o);
  colorSpaceLabel = OPP;
  // Do Daltonize xform with some reasonable params
  
  // The logic here is that we want to be able to extract just the transform matrix 
  // so that we can appy it later in, say, an openGL routine.
  computeDaltonizeTransformMatrix(xform,lmStretch,lumScale,sScale); // Stretch, project Lum, project S
  
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
    
  //wxMessageBox(wxString::Format(wxT("[%f,%f,%f]"),xformMat[0][0],xformMat[1][1],xformMat[2][2]), _T("Debug"), wxOK, NULL);
  
  // back to RGB
  changeColorSpace(o2r);
  colorSpaceLabel = RGB;
  // Clip out-of-gamut values
  clipValRange();
  // Apply inverse gamma 
  applyLookupTable(myDisp.invGammaPtrR(), myDisp.invGammaPtrG(), myDisp.invGammaPtrB());
}

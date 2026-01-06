#include "colorTools.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>


void displayDevice::init() 
{
  strcpy(displayFilename, "none");
  numGammaSamples = 0;

  // we can set up the easy transform matricies now (these are simple 
  // color-space rotations and don't depend on sensor or display specs).
  //lms2opp[0]= 0.9900; lms2opp[1]=-0.1060; lms2opp[2]=-0.0940;
  lms2opp[0]= 0.5000; lms2opp[1]= 0.5000; lms2opp[2]= 0.0000;
  lms2opp[3]=-0.6690; lms2opp[4]= 0.7420; lms2opp[5]=-0.0270;
  lms2opp[6]=-0.2120; lms2opp[7]=-0.3540; lms2opp[8]= 0.9110;
    
  //opp2lms[0]= 1.1954; opp2lms[1]= 0.2329; opp2lms[2]= 0.1303;
  //opp2lms[3]= 1.1035; opp2lms[4]= 1.5820; opp2lms[5]= 0.1608;
  //opp2lms[6]= 0.7070; opp2lms[7]= 0.6690; opp2lms[8]= 1.1905;
  opp2lms[0]= 1.0400; opp2lms[1]=-0.7108; opp2lms[2]=-0.0211;
  opp2lms[3]= 0.9600; opp2lms[4]= 0.7108; opp2lms[5]= 0.0211;
  opp2lms[6]= 0.6151; opp2lms[7]= 0.1108; opp2lms[8]= 1.1010;
  
  	
  gammaR = NULL;
  gammaG = NULL;
  gammaB = NULL;
  invgammaR = NULL;
  invgammaG = NULL;
  invgammaB = NULL;
       
  return;
}

displayDevice::displayDevice(const char *displayType) {
  init();
  loadDevice(displayType);
  return;
}

displayDevice::~displayDevice(){
  delete gammaR;	
  // also frees gammaG, gammaB, invgammaR, invgammaG, invgammaB
}

void displayDevice::loadDevice(const char *displayType) {	
  // Loads the display gamma and spectrum data for the given type.
  // Valid displayTypes:
  //   CRT
  //   LCD
  //   lapLCD
  //   

  if(strcmp(displayType,"CRT")==0){
    rgb2lms[0]= 0.05059983; rgb2lms[1]= 0.08585369; rgb2lms[2]= 0.00952420;
    rgb2lms[3]= 0.01893033; rgb2lms[4]= 0.08925308; rgb2lms[5]= 0.01370054;
    rgb2lms[6]= 0.00292202; rgb2lms[7]= 0.00975732; rgb2lms[8]= 0.07145979;

    lms2rgb[0]= 30.830854; lms2rgb[1]=-29.832659; lms2rgb[2]= 1.610474;
    lms2rgb[3]= -6.481468; lms2rgb[4]= 17.715578; lms2rgb[5]=-2.532642;
    lms2rgb[6]= -0.375690; lms2rgb[7]= -1.199062; lms2rgb[8]=14.273846;
    computeGamma(256, 2.1, 2.0, 2.1);
    //computeGamma(256, 1.0, 8.0, 1.0);
  }else if (strcmp(displayType,"LCD")==0){
    readDeviceFile("displays/LCD1.dat");
  }else if (strcmp(displayType,"lapLCD")==0){
    readDeviceFile("displays/LCD2.dat");
  }else std::cerr << "unknown display type: " << displayType <<std::endl;

  computeOpponentTransforms();

  return;
}

void displayDevice::readDeviceFile(const char *fname) {
  // Reads data in from a device description file. 
  // loads the following class vars: gammaR, gammaG, gammaB, 
  // invgammaR, invgammaG, invgammaB, rgb2lms, lms2rgb.
  FILE *fid;
  float tmp;
  int i;

  strcpy(displayFilename, fname);

  // the display file is binary, all doubles
  fid = fopen(displayFilename,"rb");
  if (fid==NULL){
    std::cerr << "ERROR: can't open file " << displayFilename << std::endl;
    exit(0);
  }
  i = fread(rgb2lms,sizeof(float),9,fid);
  i = fread(lms2rgb,sizeof(float),9,fid);
  i = fread(&tmp,sizeof(float),1,fid);
  numGammaSamples = (int)(0.5+tmp);
 
  gammaR = new float [numGammaSamples*6];
  gammaG = gammaR+numGammaSamples;
  gammaB = gammaG+numGammaSamples;
  invgammaR = gammaB+numGammaSamples;
  invgammaG = invgammaR+numGammaSamples;
  invgammaB = invgammaG+numGammaSamples;

  i = fread(gammaR,sizeof(float),numGammaSamples,fid);
  i = fread(gammaG,sizeof(float),numGammaSamples,fid);
  i = fread(gammaB,sizeof(float),numGammaSamples,fid);
  i = fread(invgammaR,sizeof(float),numGammaSamples,fid);
  i = fread(invgammaG,sizeof(float),numGammaSamples,fid);
  i = fread(invgammaB,sizeof(float),numGammaSamples,fid);
  fclose(fid);
  return;
}

void displayDevice::computeOpponentTransforms(){
  int i;

  for (i=0; i<3; i++){
    rgb2opp[i*3  ] = rgb2lms[0]*lms2opp[i*3] + rgb2lms[3]*lms2opp[i*3+1] + rgb2lms[6]*lms2opp[i*3+2];
    rgb2opp[i*3+1] = rgb2lms[1]*lms2opp[i*3] + rgb2lms[4]*lms2opp[i*3+1] + rgb2lms[7]*lms2opp[i*3+2];
    rgb2opp[i*3+2] = rgb2lms[2]*lms2opp[i*3] + rgb2lms[5]*lms2opp[i*3+1] + rgb2lms[8]*lms2opp[i*3+2];
    opp2rgb[i*3  ] = opp2lms[0]*lms2rgb[i*3] + opp2lms[3]*lms2rgb[i*3+1] + opp2lms[6]*lms2rgb[i*3+2];
    opp2rgb[i*3+1] = opp2lms[1]*lms2rgb[i*3] + opp2lms[4]*lms2rgb[i*3+1] + opp2lms[7]*lms2rgb[i*3+2];
    opp2rgb[i*3+2] = opp2lms[2]*lms2rgb[i*3] + opp2lms[5]*lms2rgb[i*3+1] + opp2lms[8]*lms2rgb[i*3+2];
  }
  /*
    cerr<<"rgb2lms="<<rgb2lms[0]<<","<<rgb2lms[1]<<","<<rgb2lms[2]<<";"<<rgb2lms[3]<<","<<rgb2lms[4]<<","<<rgb2lms[5]<<";"<<rgb2lms[6]<<","<<rgb2lms[7]<<","<<rgb2lms[8]<<endl;
    cerr<<"lms2opp="<<lms2opp[0]<<","<<lms2opp[1]<<","<<lms2opp[2]<<";"<<lms2opp[3]<<","<<lms2opp[4]<<","<<lms2opp[5]<<";"<<lms2opp[6]<<","<<lms2opp[7]<<","<<lms2opp[8]<<endl;
    cerr<<"rgb2opp="<<rgb2opp[0]<<","<<rgb2opp[1]<<","<<rgb2opp[2]<<";"<<rgb2opp[3]<<","<<rgb2opp[4]<<","<<rgb2opp[5]<<";"<<rgb2opp[6]<<","<<rgb2opp[7]<<","<<rgb2opp[8]<<endl;
    cerr<<"opp2lms="<<opp2lms[0]<<","<<opp2lms[1]<<","<<opp2lms[2]<<";"<<opp2lms[3]<<","<<opp2lms[4]<<","<<opp2lms[5]<<";"<<opp2lms[6]<<","<<opp2lms[7]<<","<<opp2lms[8]<<endl;
    cerr<<"lms2rgb="<<lms2rgb[0]<<","<<lms2rgb[1]<<","<<lms2rgb[2]<<";"<<lms2rgb[3]<<","<<lms2rgb[4]<<","<<lms2rgb[5]<<";"<<lms2rgb[6]<<","<<lms2rgb[7]<<","<<lms2rgb[8]<<endl;
    cerr<<"opp2rgb="<<opp2rgb[0]<<","<<opp2rgb[1]<<","<<opp2rgb[2]<<";"<<opp2rgb[3]<<","<<opp2rgb[4]<<","<<opp2rgb[5]<<";"<<opp2rgb[6]<<","<<opp2rgb[7]<<","<<opp2rgb[8]<<endl;
  */
  return;
}

void displayDevice::computeGamma(int numSamples, float r, float g, float b){
  int i;
  float val, inc, scale, invr, invg, invb;

  invr = 1.0/r; invg = 1.0/g; invb = 1.0/b;

  if(numSamples != numGammaSamples){
    delete gammaR;
    gammaR = NULL;
    numGammaSamples = numSamples;
  }
  if(gammaR==NULL){
    gammaR = new float [numGammaSamples*6];
    gammaG = gammaR+numGammaSamples;
    gammaB = gammaG+numGammaSamples;
    invgammaR = gammaB+numGammaSamples;
    invgammaG = invgammaR+numGammaSamples;
    invgammaB = invgammaG+numGammaSamples;
  }

  scale = numSamples-1;
  val = 0.0;
  inc = 1.0/(numGammaSamples-1);
  for(i=0; i<numGammaSamples; i++){
    gammaR[i] = pow(val, r) * scale;
    gammaG[i] = pow(val, g) * scale;
    gammaB[i] = pow(val, b) * scale;

    invgammaR[i] = pow(val, invr) * scale;
    invgammaG[i] = pow(val, invg) * scale;
    invgammaB[i] = pow(val, invb) * scale;
    //if(i%8==0) cerr<<*(gamTmp-3)<<","<<*(gamTmp-2)<<","<<*(gamTmp-1)<<";"<<val<<endl;
    val += inc;
  }
}

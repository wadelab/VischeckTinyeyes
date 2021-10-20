#include <time.h>
#include "runSimulation.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <iostream>
#include <string>

const unsigned int jpegQuality = 82;

void printHelp(void);

int main(int argc, char **argv){
  // vischeck parameters
  int verbose=0;
  char dataType='b';
  float viewDist=0;
  float dpi=90;
  char *sensorType="normal";
  char *simDisp="CRT";
  char *viewDisp="CRT";
  clock_t startTicks;
  float vischeckSecs;
  float kernelWt[9];
  float kernelSD[9]; 
  float kernelScale[3];
  float lmStretch=50.0; 
  float lumScale=50.0;
  float sScale=50.0;
  kernelScale[0] = kernelScale[1] = kernelScale[2] = 1.0;
  // Set some reasonable defaults (from Poirson and Wandell)
  kernelWt[0]=0.9207; kernelWt[1]=0.105;  kernelWt[2]=-0.108;
  kernelWt[3]=0.5310; kernelWt[4]=0.33;   kernelWt[5]=0.0;
  kernelWt[6]=0.4877; kernelWt[7]=0.3711; kernelWt[8]=0.0;
  kernelSD[0]=0.01;   kernelSD[1]=0.05;   kernelSD[2]=1.5; 
  kernelSD[3]=0.015;  kernelSD[4]=0.18;   kernelSD[5]=0.5;
  kernelSD[6]=0.02;   kernelSD[7]=0.14;   kernelSD[8]=0.0;


  int bytesPerPix = 1;	// Currently, only support 8-bit images
  int x=1,y=1;
  int c;
  bool applyCorrection = false;

  while (1) {

    c = getopt(argc, argv, "hvbxcas:l:y:m:t:S:V:d:r:W:D:C:");
    if (c == -1)
      break;

    switch (c) {
    case 'h':
      printHelp();
      return(-1);
      break;
    case 'a' :
      applyCorrection = true;
      break;
    case 's':
      lmStretch = atof(optarg);
      break;
    case 'l':
      lumScale = atof(optarg);
      break;
    case 'y':
      sScale = atof(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'b':
      dataType = 'b';
      break;
    case 'x':
      dataType = 'x';
      break;
    case 'c':
      dataType = 'c';
      break;
    case 'm':
      sscanf(optarg,"%d,%d", &x, &y);
      break;
    case 't':
      sensorType = optarg;
      break;
    case 'S':
      simDisp = optarg;
      break;
    case 'V':
      viewDisp = optarg;
      break;
    case 'd':
      viewDist = atof(optarg);
      break;
    case 'r':
      dpi = atof(optarg);
      break;
    case 'W':
      sscanf(optarg,"%f,%f,%f,%f,%f,%f,%f,%f,%f", &(kernelWt[0]), &(kernelWt[1]), 
	     &(kernelWt[2]), &(kernelWt[3]), &(kernelWt[4]), &(kernelWt[5]), 
	     &(kernelWt[6]), &(kernelWt[7]), &(kernelWt[8]));
      break;
    case 'D':
      sscanf(optarg,"%f,%f,%f,%f,%f,%f,%f,%f,%f", &(kernelSD[0]), &(kernelSD[1]), 
	     &(kernelSD[2]), &(kernelSD[3]), &(kernelSD[4]), &(kernelSD[5]), 
	     &(kernelSD[6]), &(kernelSD[7]), &(kernelSD[8]));
      break;
    case 'C':
      sscanf(optarg,"%f,%f,%f", &(kernelScale[0]), &(kernelScale[1]), &(kernelScale[2]));
      break;
    }

  }

  if(verbose==1){
    std::cerr << sensorType<<","<<simDisp<<","<<viewDisp<<","<<viewDist<<","<<dpi << std::endl;
    std::cerr << "x,y,bbp=" << x << "," << y << "," << bytesPerPix << std::endl; 
  }
  // 
  // Read data
  // 
  //fid = fopen(inFile, "rb");
  if (stdin==NULL){
    std::cerr << "ERROR: stdin not open!" << std::endl;
    exit(0);
  }

  int i;
  unsigned char *rawData; 
  rawData = new unsigned char [x*y*bytesPerPix*3];

  switch(dataType){
  case 'b':
    fread(rawData, bytesPerPix, x*y*3, stdin);
    break;
  case 'x':
    for(i=0; i<x*y*3; i++){
      fscanf(stdin, "%2x", (unsigned int *)&(rawData[i]));
    }
    break;
  case 'c':
    unsigned char *tmp = rawData;
    for(i=0; i<x*y; i++){
      fscanf(stdin, "%x%x%x\n", (unsigned int *)tmp++, (unsigned int *)tmp++, (unsigned int *)tmp++);
    }
    break;
  }

  //fclose(stdin);
  
  // *** FIX ME: The following is inefficient when we want to get multiple images
  // out. For example, for daltonize demos, we usually want 3 out images:
  // the daltonized, the daltonized brettelized, and the original brettelized.
  // Since they all rely on the smae pre-processing and post-processing, it 
  // would be much better to return all three (or at least the first two)
  startTicks = clock();
  if(applyCorrection){
    std::cerr << "Applying Daltonize: lmStretch=" << lmStretch << 
      ", lmScale=" << lumScale << ", sScale=" << sScale << std::endl;
    runCorrection((unsigned char *)rawData, x, y, simDisp, viewDisp, 
		  lmStretch, lumScale, sScale);
  }
  runSimulation((unsigned char *)rawData, x, y, viewDist, dpi, sensorType, 
		simDisp, viewDisp, kernelWt, kernelSD, kernelScale);
  vischeckSecs = (float)(1.0*clock()/CLOCKS_PER_SEC-startTicks/CLOCKS_PER_SEC);

  //
  // Write processed data
  // 
  //fid = fopen(outFile, "wb");
  if (stdout==NULL){
    std::cerr << "ERROR: stdout not open!" << std::endl;
    exit(0);
  }
  switch(dataType){
  case 'b':
    fwrite(rawData, bytesPerPix, x*y*3, stdout);
    break;
  case 'x':
    for(i=0; i<x*y*3; i++){
      fprintf(stdout, "%.2x", rawData[i]);
    }
    break;
  case 'c':
    unsigned char *tmp = rawData;
    for(i=0; i<x*y; i++){
      fprintf(stdout, "%.2x%.2x%.2x\n", *(tmp++), *(tmp++), *(tmp++));
    }
    break;
  }
  
  //fclose(stdout);
  if(verbose==1)		
    std::cerr << "Vischeck: " << vischeckSecs << "s; " << std::endl;
}


void printHelp(void){
    std::cout << std::endl << "runVischeck [options]" <<std::endl<<std::endl;
    std::cout << "  Takes raw RGB image on STDIN, processes it, and delivers result on STDOUT."<<std::endl<<std::endl;
    std::cout << "  -h:    \thelp- print this help message" <<std::endl;
    std::cout << "  -v:    \tverbose- prints some info on stderr" <<std::endl;
    std::cout << "  -a:    \tapply Daltonize correction" <<std::endl;
    std::cout << "  -s:    \tDaltonize lmStretch parameter" <<std::endl;
    std::cout << "  -l:    \tDaltonize lumScale parameter" <<std::endl;
    std::cout << "  -y:    \tDaltonize sScale parameter" <<std::endl;
    std::cout << "  -b,-x or -c: \tdata type- binary, hex or color-table format (default=binary)" <<std::endl;
    std::cout << "  -m: \tx,y pixels in raw RGB image to be processed (default=1,1)" <<std::endl;
    std::cout << "  -t:    \ttype- normal, deuteranope, protanope, tritanope (default=normal)" <<std::endl;
    std::cout << "  -S,-V: \tsimDisp & viewDisp-CRT, LCD, lapLCD (default=CRT)" <<std::endl;
    std::cout << "  -d:    \tdist- simulated viewing distance, in inches (default=0)" <<std::endl;
    std::cout << "  -r:    \tresolution- dots-per-inch of the simulated display (default=90)" <<std::endl;
    std::cout << "  -W:    \t(kernel weights) lum1,lum2,lum3,l-m1,l-m2,l-m3,s1,s2,s3" <<std::endl;
    std::cout << "         \t(default = Poirson & Wandell)" <<std::endl;
    std::cout << "  -D:    \t(kernel widths, SDs) lum1,lum2,lum3,l-m1,l-m2,l-m3,s1,s2,s3" <<std::endl;
    std::cout << "         \t(default = Poirson & Wandell)" <<std::endl;
    std::cout << "  -C:    \t(kernel scale) lum,l-m,s (default = 1,1,1)" <<std::endl<<std::endl;
}

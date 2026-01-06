#ifndef __runSimulation_h
#define __runSimulation_h

void runSimulation(unsigned char *dataPtr, int x, int y, float viewDist, float dpi, char *sensorType,
		   char *simDisplayType, char *viewDisplayType, float *kernelWt, 
		   float *kernelSDdouble, float *kernelScale);


void runCorrection(unsigned char *dataPtr, int x, int y, char *simDisplayType, 
		   char *viewDisplayType, float lmStretch, float lumScale, 
		   float sScale);

#endif // __runSimulation_h

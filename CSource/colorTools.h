#ifndef __display_h
#define __display_h

class displayDevice;
class sensors;

class displayDevice {
private:
	float *gammaR;
	float *gammaG;
	float *gammaB;
	float *invgammaR;
	float *invgammaG;
	float *invgammaB;
	float lms2rgb[9];
	float rgb2lms[9];
	float lms2opp[9];
	float opp2lms[9];
	float rgb2opp[9];
	float opp2rgb[9];
	
	int numGammaSamples;
	char displayFilename[64];

	void init();
	void readDeviceFile(const char *fname);
	void computeOpponentTransforms();
	
public:
	displayDevice(){ init(); return;}
	displayDevice(const char *displayType);

	~displayDevice();

	void loadDevice(const char *displayType);
	void computeGamma(int numSamples, float r, float g, float b);
	
	float getGammaR(int i) {return gammaR[i];}
	float getGammaG(int i) {return gammaG[i];}
	float getGammaB(int i) {return gammaB[i];}
	float getInvGammaR(int i) {return invgammaR[i];}
	float getInvGammaG(int i) {return invgammaG[i];}
	float getInvGammaB(int i) {return invgammaB[i];}

	int gammaLen() {return numGammaSamples;}
	float *gammaPtrR() {return gammaR;}
	float *gammaPtrG() {return gammaG;}
	float *gammaPtrB() {return gammaB;}
	float *invGammaPtrR() {return invgammaR;}
	float *invGammaPtrG() {return invgammaG;}
	float *invGammaPtrB() {return invgammaB;}

	float *getRGB2LMS() {return rgb2lms;}
	float *getLMS2RGB() {return lms2rgb;}
	float *getLMS2OPP() {return lms2opp;}
	float *getOPP2LMS() {return opp2lms;}
	float *getRGB2OPP() {return rgb2opp;}
	float *getOPP2RGB() {return opp2rgb;}
};

#endif // __display_h


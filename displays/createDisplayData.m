%% This is matlab code written a long time ago
% to convert measurements of spectra and gamma from display devices
% (Early LCD displays in this case) into a file format that can be used in the
% Vischeck code.
% The '3T' in the NEC3T below refers to the fact that the display was used to
% display stimuli in one of the first 3T scanners
% installed at the Lucas Center in Stanford.
% Also pretty cool to have found a 10-bit LCD display in 1999!
% CRT data refers to some cathode ray tube...
% ARW 10/20/2021

% clean up gamma and spectra for vischeck
%spect = load('../gammaTables/ViewSonicSpectra');
%gamma = load('../gammaTables/ViewSonicRadius10-bit');

spect = load('../gammaTables/NEC3TSpectra');
gamma = load('../gammaTables/NEC3TRadius10-bit');

gammaTable = gamma.gamma10(2:4:end,:);
invGamma = round(gamma.invGamma10(2:4:end,:).*255);

displaySPD = spect.monitorSpectra(:,1:3);
wavelength = [370:730]';

notes.created = '99.11.10 by RFD, based on NEC2000 LCD data';

save LCD1 gammaTable invGamma displaySPD wavelength notes;

% check displays
cd displays
files = what;
for ii=1:length(files.mat)
   d{ii} = load(files.mat{ii});
end
numDisp = length(d);
for ii=1:numDisp
	figure(ii);
   plot(d{ii}.invGamma);
   d{ii}.notes
end


% octave code to load a vischeck .dat display file
fid = fopen('CRT1.dat');

% the display file is binary, all doubles
fid = fopen(displayFilename,"rb");

i = fread(rgb2lms,sizeof(float),9,fid);
i = fread(lms2rgb,sizeof(float),9,fid);
i = fread(&tmp,sizeof(float),1,fid);
numGammaSamples = (int)(0.5+tmp);
 
i = fread(gammaR,sizeof(float),numGammaSamples,fid);
i = fread(gammaG,sizeof(float),numGammaSamples,fid);
i = fread(gammaB,sizeof(float),numGammaSamples,fid);
i = fread(invgammaR,sizeof(float),numGammaSamples,fid); 
i = fread(invgammaG,sizeof(float),numGammaSamples,fid);
i = fread(invgammaB,sizeof(float),numGammaSamples,fid);
fclose(fid); 

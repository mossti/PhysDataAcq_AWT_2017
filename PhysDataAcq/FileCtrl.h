#ifndef FILECTRL_H
#define FILECTRL_H

#include <iostream>
#include <string>
#include <NIDAQmx.h>
using namespace std;

ifstream* fnOpenLargeFileToRead(string strStimulusFilename, uInt64* ptrFrameNumElem);

ifstream* fnOpenFileToRead(string strStimulusFilename, uInt32* ptrFrameNumElem);

ofstream* fnOpenFileToWrite(string strStimulusFilename);

void fnCloseFile(ifstream* ptrStimFile);

void LoadFullAOBuffer(double* pBuffer, int16 nbChannels, uInt32 nbSamplePerChannel, ifstream* ptrStimFile);

uInt32 ConstructAOBuffer(double* pBuffer, uInt32 nbSamplePerChannel, uInt32 NZSigSamples, uInt32 NGridSamples, ifstream* ptrStimFile, ifstream* ptrAO_X_File, ifstream* ptrAO_Y_File);

ifstream* fnOpenFileToRead_int16(string strStimulusFilename, uInt32* ptrFrameNumElem);

uInt32 ConstructAOBuffer_int16(int16* pBuffer, uInt32 nbSamplePerChannel, uInt32 NZSigSamples, uInt32 NGridSamples, ifstream* ptrAOFile, int16* ptrAO_X_File, int16* ptrAO_Y_File, uInt32 NZSignalRepeat, uInt32 TotNZSample);

#endif
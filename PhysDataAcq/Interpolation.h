#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <iostream>
#include <NIDAQmx.h>

using namespace std;

unsigned int Sub2Ind(float IMAGE_HEIGHT, float IMAGE_WIDTH, unsigned short ROW, unsigned short COULMN);

uInt32 ConstructAOBuffer_RT_int16( int16* pBuffer,
	uInt32 nbSamplePerChannel, uInt32 NZSigSamples, uInt32 NGridSamples, uInt32 NZRpt, uInt32 TotNFrames, int16 NumAOChannels,
	int16* ptrXPixelPos, int16* ptrYPixelPos,
	float* ptrYawPosVec, float* ptrPitchPosVec, float* ptrRollPosVec, 
	float* ptrLED_XPos, float* ptrLED_YPos, float* WorldMapVec, float Height, float Width );


#endif
//%SUB2IND Linear index from multiple subscripts.
//%   SUB2IND is used to determine the equivalent single index
//%   corresponding to a given set of subscript values.
//%
//%   IND = SUB2IND(SIZ,I,J) returns the linear index equivalent to the
//%   row and column subscripts in the arrays I and J for an matrix of
//%   size SIZ. 
//%
//%   IND = SUB2IND(SIZ,I1,I2,...,IN) returns the linear index
//%   equivalent to the N subscripts in the arrays I1,I2,...,IN for an
//%   array of size SIZ.
//%
//%   I1,I2,...,IN must have the same size, and IND will have the same size
//%   as I1,I2,...,IN. For an array A, if IND = SUB2IND(SIZE(A),I1,...,IN)),
//%   then A(IND(k))=A(I1(k),...,IN(k)) for all k.
//%
//%==============================================================================
#include <iostream>
#include <math.h>
#include <NIDAQmx.h>
#include "Interpolation.h"
#define PI 3.14159265

using namespace std;

unsigned int Sub2Ind(float IMAGE_HEIGHT, float IMAGE_WIDTH, unsigned short Row, unsigned short Column)
{
	unsigned int ndx;

	// Need to put in error checking to make sure ROW and COLUMN are not out of bounds
	if (Row > IMAGE_HEIGHT || Column > IMAGE_WIDTH)
		cout << "Sub2Ind: Out of range subscript." << endl;

	//Compute linear indices
	ndx = (unsigned int) ((Column-1)*IMAGE_HEIGHT + Row);

	return ndx;
};

//uInt32 ConstructAOBuffer_RT_int16( int16* pBuffer,
//	uInt32 nbSamplePerChannel, uInt32 NZSigSamples, uInt32 NGridSamples, uInt32 NZRpt, uInt32 TotNFrames, int16 NumAOChannels,
//	int16* ptrXPixelPos, int16* ptrYPixelPos,
//	float* ptrYawPosVec, float* ptrPitchPosVec, float* ptrRollPosVec, 
//	float* ptrLED_XPos, float* ptrLED_YPos, float* WorldMapVec, float Height, float Width )
//{
//
//		//Initialize the arrays for the INPUT buffer
//		uInt32	nFrame = (uInt32) nbSamplePerChannel/NumAOChannels;
//		uInt32	idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;
//		int counter = 0;
//		int temp= 0;
//
//		float WorldMap_XCoord= 0;	// X-coordinates for interpolated LED intensities
//		float WorldMap_YCoord= 0;	// Y-coordinates for interpolated LED intensities
//		float AlphaR		= 0;
//		float AlphaC		= 0;
//		float XPos			= 0;
//		float YPos			= 0;
//		float Frames		= 0;
//		unsigned short kR0	= 0;
//		unsigned short kR1	= 0;
//		unsigned short kC0	= 0;
//		unsigned short kC1	= 0;
//		unsigned int ind1	= 0, ind2 = 0, ind3 = 0, ind4 = 0;
//		
//
//		//Read the data from the INPUT file into the INPUT buffer
//		for ( idx1 = 0; idx1 < nbSamplePerChannel; idx1++ ) {
//
//			idx2 = idx1 % NGridSamples;
//			idx3 = (uInt32) floor( (float) idx1/NGridSamples ) ;
//			if (idx3 != temp) {
//				temp = idx3;
//			}
//
//			for ( idx4 = 0; idx4 < NZRpt; idx4++ ) {
//
//				pBuffer[counter++] = ptrXPixelPos[idx2];	// X Signals NEED TO CONVERT TO CONVERT USING DAC RESOLUTION
//
//				pBuffer[counter++] = ptrYPixelPos[idx2];	// Y signals
//
//
//				// Z signals
//				if ( idx4 == 0 ) {
//					pBuffer[counter++] = 0;
//				}
//
//				else if ( idx4 == 1 ) 
//				{
//					// FIND COORDINATES OF WORLD MAP CLOSEST TO LED POSITIONS
//					XPos =  ( ptrLED_XPos[idx2] * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( ptrLED_YPos[idx2] * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );
//					YPos = -( ptrLED_XPos[idx2] * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( ptrLED_YPos[idx2] * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );
//					WorldMap_XCoord = fmod ( ( XPos + ptrYawPosVec[TotNFrames+idx3] - 1 ) , Width ) + 1;
//					WorldMap_YCoord = fmod ( ( YPos + ptrPitchPosVec[TotNFrames+idx3] - 1 ) , Height ) + 1 ;
//
//					AlphaC = fmod( WorldMap_XCoord, 1 );
//					AlphaR = fmod( WorldMap_YCoord, 1 );
//
//					kR0 = (unsigned short) floor( fmod( ( WorldMap_YCoord - 1 ), Height ) + 1 );
//					kR1 = (unsigned short) fmod( kR0, Height ) + 1;
//
//					kC0 = (unsigned short) floor( fmod( ( WorldMap_XCoord - 1 ), Width ) + 1 );
//					kC1 = (unsigned short) fmod( kC0, Width ) + 1;
//
//					// CREATE THE VECTOR OF WEIGHTED IINTENSITIES THAT DEFINES A LINEARLY INTERPOLATED VALUE.
//					ind1 = Sub2Ind( Height, Width, kR0, kC0 );
//					ind2 = Sub2Ind( Height, Width, kR1, kC0 );
//					ind3 = Sub2Ind( Height, Width, kR0, kC1 );
//					ind4 = Sub2Ind( Height, Width, kR1, kC1 );
//
//					Frames = ( (1-AlphaR) * (1-AlphaC) * WorldMapVec[ ind1 ] + AlphaR * (1-AlphaC) * WorldMapVec[ ind2 ] + 
//						(1-AlphaR) * AlphaC * WorldMapVec[ ind3 ] + AlphaR * AlphaC * WorldMapVec[ ind4 ] ) ;
//					Frames = Frames * pow(2.0, 15.0) / 5;
//
//					pBuffer[counter++] = (int16) Frames;	// Z signals
//				}
//				
//				else {
//					pBuffer[counter++] = (int16) Frames;	// Z signals
//				}
//			}
//		}
//
//		return idx3;
//};



uInt32 ConstructAOBuffer_RT_int16( int16* pBuffer,
	uInt32 nbSamplePerChannel, uInt32 NZSigSamples, uInt32 NGridSamples, uInt32 NZRpt, uInt32 TotNFrames, int16 NumAOChannels,
	int16* ptrXPixelPos, int16* ptrYPixelPos,
	float* ptrYawPosVec, float* ptrPitchPosVec, float* ptrRollPosVec, 
	float* ptrLED_XPos, float* ptrLED_YPos, float* WorldMapVec, float Height, float Width ) {

		//Initialize the arrays for the INPUT buffer
		uInt32	nFrame = (uInt32) nbSamplePerChannel/NumAOChannels;
		uInt32	idx2 = 0, idx3 = 0;
		int counter = 0;
		uInt32 temp = -1;

		float WorldMap_XCoord= 0;	// X-coordinates for interpolated LED intensities
		float WorldMap_YCoord= 0;	// Y-coordinates for interpolated LED intensities
		float AlphaR		= 0;
		float AlphaC		= 0;
		float XPos			= 0;
		float YPos			= 0;
		float Frames		= 0;
		unsigned short kR0	= 0;
		unsigned short kR1	= 0;
		unsigned short kC0	= 0;
		unsigned short kC1	= 0;
		unsigned int ind1	= 0, ind2 = 0, ind3 = 0, ind4 = 0;
		

		//Read the data from the INPUT file into the INPUT buffer
		for ( uInt32 idx1 = 0; idx1 < nbSamplePerChannel; idx1++ ) {
			idx2 = idx1 % NGridSamples;
			idx3 = (uInt32) floor( (float) idx1/NGridSamples ) ;
			if (idx3 != temp) {
				temp = idx3;

				// This one extra point is defined by an (X,Y,Z) triplet. This ensures the AOBuffer has 7500 values, or 2500 triplets.
				// Number of Z-Values = ( 833 * NZSignalRepeat + 1 )
				pBuffer[counter++] = ptrXPixelPos[0];
				pBuffer[counter++] = ptrYPixelPos[0];
				pBuffer[counter++] = 0;
			}

 			for ( uInt32 idx4 = 0; idx4 < NZRpt; ++idx4 ) {
				pBuffer[counter++] = ptrXPixelPos[idx2];	// X Signals NEED TO CONVERT USING DAC RESOLUTION
				pBuffer[counter++] = ptrYPixelPos[idx2];	// Y signals

				// Z signals
				if ( idx4 == 0 || idx2 == 0 || idx2 > (NGridSamples - 6) ) {
					pBuffer[counter++] = 0;
				}

				else if ( idx4 == 1 ) 
				{
					// FIND COORDINATES OF WORLD MAP CLOSEST TO LED POSITIONS
					XPos =  ( ptrLED_XPos[idx2] * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( ptrLED_YPos[idx2] * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );
					YPos = -( ptrLED_XPos[idx2] * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( ptrLED_YPos[idx2] * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );
					WorldMap_XCoord = fmod ( ( XPos + ptrYawPosVec[TotNFrames+idx3] - 1 ) , Width ) + 1;
					WorldMap_YCoord = fmod ( ( YPos + ptrPitchPosVec[TotNFrames+idx3] - 1 ) , Height ) + 1 ;


					//XPos = fmod ( ( ptrLED_XPos[idx2] + ptrYawPosVec[TotNFrames+idx3] - 1 ) , Width ) + 1;
					//YPos = fmod ( ( ptrLED_YPos[idx2] + ptrPitchPosVec[TotNFrames+idx3] - 1 ) , Height ) + 1 ;
					//WorldMap_XCoord =  ( XPos * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( YPos * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );
					//WorldMap_YCoord = -( XPos * sin(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) ) + ( YPos * cos(2 * PI * ptrRollPosVec[TotNFrames+idx3]/360) );



					AlphaC = fmod( WorldMap_XCoord, 1 );
					AlphaR = fmod( WorldMap_YCoord, 1 );

					kR0 = (unsigned short) floor( fmod( ( WorldMap_YCoord - 1 ), Height ) + 1 );
					kR1 = (unsigned short) fmod( kR0, Height ) + 1;

					kC0 = (unsigned short) floor( fmod( ( WorldMap_XCoord - 1 ), Width ) + 1 );
					kC1 = (unsigned short) fmod( kC0, Width ) + 1;

					// CREATE THE VECTOR OF WEIGHTED IINTENSITIES THAT DEFINES A LINEARLY INTERPOLATED VALUE.
					ind1 = Sub2Ind( Height, Width, kR0, kC0 );
					ind2 = Sub2Ind( Height, Width, kR1, kC0 );
					ind3 = Sub2Ind( Height, Width, kR0, kC1 );
					ind4 = Sub2Ind( Height, Width, kR1, kC1 );

					Frames = ( (1-AlphaR) * (1-AlphaC) * WorldMapVec[ ind1 ] + AlphaR * (1-AlphaC) * WorldMapVec[ ind2 ] + 
						(1-AlphaR) * AlphaC * WorldMapVec[ ind3 ] + AlphaR * AlphaC * WorldMapVec[ ind4 ] ) ;
					Frames = Frames * pow(2.0, 15.0) / 5;

					pBuffer[counter++] = (int16) Frames;	// Z signals
				}
				
				else {
					pBuffer[counter++] = (int16) Frames;	// Z signals
				}
			}
		}

		return idx3;
};
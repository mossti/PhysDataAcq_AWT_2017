/* Version 5 of the Frame Generation Program
This program implements a simple linear interpolation algorithm that computes for each sampling array position the interpolated values
of the more coarse underlying static image matrix.


UPDATES:
1. Created a separate header file with a function to do the interpolation. This version calls the new function. 

*/

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <math.h>
#include <windows.h>

#include "FileCtrl.h"
#include "Interpolation.h"

#define PI 3.14159265

using namespace std;

int main() {

	time_t start,end;
	double dif;

	float outputBuffer = 0;
	int16 outputBuffer_16 = 0;
	unsigned int jj = 0;
	unsigned int kk = 0;
	unsigned int ind1 = 0, ind2 = 0, ind3 = 0, ind4 = 0; 

	//---------------------------------------------
	// LOAD WORLD MAP
	string WorldMapFileName("WorldMap.dat");
	uInt32 filesize_WorldMapVec;
	ifstream* ptrWorldMapVec = 0;
	ptrWorldMapVec = fnOpenFileToRead(WorldMapFileName,&filesize_WorldMapVec);

	float * WorldMapVec;
	WorldMapVec = new float [ filesize_WorldMapVec ];
	ptrWorldMapVec->read(reinterpret_cast<char *> (WorldMapVec), filesize_WorldMapVec * sizeof (float) );
	ptrWorldMapVec->close();

	//---------------------------------------------
	// LOAD YAW POSITION
	string YawPosFileName("YawPos.dat");
	uInt32 filesize_YawPos;
	vector<float> YawPosVec;
	ifstream* ptrYawPos = 0;
	ptrYawPos = fnOpenFileToRead(YawPosFileName,&filesize_YawPos);
	YawPosVec.assign(filesize_YawPos,0);
	for(jj = 0; jj < filesize_YawPos; jj++) {
		ptrYawPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
		YawPosVec[jj] = outputBuffer;
	}
	ptrYawPos->close();

	//---------------------------------------------
	// LOAD PITCH POSITION
	string PitchPosFileName("PitchPos.dat");
	uInt32 filesize_PitchPos;
	vector<float> PitchPosVec;
	ifstream* ptrPitchPos = 0;
	ptrPitchPos = fnOpenFileToRead(PitchPosFileName,&filesize_PitchPos);
	PitchPosVec.assign(filesize_PitchPos,0);
	for(jj = 0; jj < filesize_PitchPos; jj++) {
		ptrPitchPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
		PitchPosVec[jj] = outputBuffer;
	}
	ptrPitchPos->close();

	cout << "3. Creating Roll Jitter File." << endl;
	//---------------------------------------------
	// LOAD ROLL POSITION
	vector<float> RollPosVec;
	RollPosVec.assign(filesize_YawPos,0);
	for(jj = 0; jj < filesize_YawPos; jj++) {
		RollPosVec[jj] = 0;
	}

	cout << "4. Loading Sampling Array Yaw Positions." << endl;
	//---------------------------------------------
	// LOAD LED X-POSITION
	string LED_XPosFileName("LED_XPos.dat");
	uInt32 filesize_LED_XPos;
	vector<float> LED_XPos;
	ifstream* ptrLED_XPos = 0;
	ptrLED_XPos = fnOpenFileToRead(LED_XPosFileName,&filesize_LED_XPos);
	LED_XPos.assign(filesize_LED_XPos,0);
	for(jj=0; jj<filesize_LED_XPos; jj++) {
		ptrLED_XPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
		LED_XPos[jj] = outputBuffer;
	}
	ptrLED_XPos->close();

	cout << "5. Loading Sampling Array Pitch Positions." << endl;
	//---------------------------------------------
	// LOAD LED Y-POSITION
	string LED_YPosFileName("LED_YPos.dat");
	uInt32 filesize_LED_YPos;
	vector<float> LED_YPos;
	ifstream* ptrLED_YPos = 0;
	ptrLED_YPos = fnOpenFileToRead(LED_YPosFileName,&filesize_LED_YPos);
	LED_YPos.assign(filesize_LED_YPos,0);
	for(jj=0; jj<filesize_LED_YPos; jj++) {
		ptrLED_YPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
		LED_YPos[jj] = outputBuffer;
	}
	ptrLED_YPos->close();


	cout << "6. Loading X Pixels Positions." << endl;
	// Initialize the Stimulus file (X-signal)
	/*********************************************/
	ifstream* ptrXPixelPos = 0;
	uInt32 filesize_XPixelPos = 0;
	string strAO_X_FileName;
	vector<int16> X;

	strAO_X_FileName = "XPixelPos.dat";
	ptrXPixelPos = fnOpenFileToRead_int16(strAO_X_FileName, &filesize_XPixelPos); //Call function to open Analog Output file
	X.assign(filesize_XPixelPos,0);
	for( uInt32 idx2=0; idx2<filesize_XPixelPos; idx2++ ) {
		ptrXPixelPos -> read( reinterpret_cast<char *>( &outputBuffer_16 ), sizeof(int16));
		X[idx2] = outputBuffer_16;
	}
	ptrXPixelPos->close();

	cout << "7. Loading Y Pixels Positions." << endl;
	// Initialize the Stimulus file (Y-signal)
	/*********************************************/
	ifstream* ptrYPixelPos = 0;
	uInt32 filesize_YPixelPos = 0;
	string strAO_Y_FileName;
	vector<int16> Y;

	strAO_Y_FileName = "YPixelPos.dat";	//Name of Analog Stimulus file
	ptrYPixelPos = fnOpenFileToRead_int16(strAO_Y_FileName, &filesize_YPixelPos); //Call function to open Analog Output file
	Y.assign(filesize_YPixelPos,0);
	for( uInt32 idx2=0; idx2<filesize_YPixelPos; idx2++ ) {
		ptrYPixelPos->read( reinterpret_cast<char *>( &outputBuffer_16 ), sizeof(int16));
		Y[idx2] = outputBuffer_16;
	}
	ptrYPixelPos->close();


	cout << "8. Opening Binary Output File to Interpolated Values." << endl;
	//---------------------------------------------
	// CREATE FILE TO WRITE INTERPOLATED VALUES
	ofstream* ptrInterpPixelVals = 0;
	ptrInterpPixelVals = fnOpenFileToWrite( "FrameValues.dat" );
	cout << "File for interpolated values opened." << endl;


	// DEFINE PARAMETERS
	double BinSize				= 0.1;
	unsigned int NFrames		= filesize_YawPos;
	unsigned int NPixels		= filesize_LED_XPos;
	float Height				= sqrt( (float) filesize_WorldMapVec);
	float Width					= sqrt( (float) filesize_WorldMapVec);
	float * Frames				= new float [NPixels];

	float WorldMap_XCoord= 0;	// X-coordinates for interpolated LED intensities
	float WorldMap_YCoord= 0;	// Y-coordinates for interpolated LED intensities
	float AlphaR		= 0;
	float AlphaC		= 0;
	float XPos			= 0;
	float YPos			= 0;
	unsigned short kR0	= 0;
	unsigned short kR1	= 0;
	unsigned short kC0	= 0;
	unsigned short kC1	= 0;

	int NumAOChannels		= 3;
	int NZSignalRepeat		= 3;
	uInt32	NZSample		= 0;
	uInt32	TotNFrames		= 0;
	uInt32	AOOneChanBufSiz	= 200000;
	uInt32	AOHalfBuf_Siz	= AOOneChanBufSiz * NumAOChannels * NZSignalRepeat;
	uInt32	AOBuffer_Siz	= AOHalfBuf_Siz * 2;
	vector<int16>	iAOBuffer;
	iAOBuffer.assign(AOBuffer_Siz,0);
	int NChunks = filesize_YawPos / (AOOneChanBufSiz / filesize_XPixelPos);
	cout << "Number of Chunks = " << NChunks << endl;

	//-----------------------------------------------------------
	cout << "BEGIN CALCULATIONS" << endl << endl;
	time (&start);	// Timing set-up

	for ( int kk = 0; kk < NChunks; kk++ )
	{
		NZSample = ConstructAOBuffer_RT_int16( &iAOBuffer[0],
			AOOneChanBufSiz, filesize_YawPos, filesize_XPixelPos, NZSignalRepeat, TotNFrames, NumAOChannels,
			&X[0], &Y[0], &YawPosVec[0], &PitchPosVec[0], &RollPosVec[0], &LED_XPos[0], &LED_YPos[0],
			WorldMapVec, Height, Width );

		TotNFrames = TotNFrames + NZSample + 1;

		// WRITE DATA, FRAMES, TO A BINARY FILE
		ptrInterpPixelVals->write( reinterpret_cast<char *> (&iAOBuffer[0]), AOHalfBuf_Siz * sizeof (int16) );

	}

	time (&end);
	dif = difftime (end,start);
	printf ("It took you %2.5lf seconds to perform the interpolations.\n", dif );

	ptrWorldMapVec ->close();
	ptrInterpPixelVals->close();
	cout << "Closed file with interpolated values." << endl << endl;

	int counter = 0;
	while ( !_kbhit() ) {
		if (counter == 0) {
			cout << "waiting for keyboard press." << endl;
			counter ++;
		}
	}

	return 0;
}


	//---------------------------------------------
	// LOAD YAW POSITION
	//string YawPosFileName("YawPos.dat");
	//uInt32 filesize_YawPos;
	//vector<float> YawPosVec;
	//ifstream* ptrYawPos = 0;
	//ptrYawPos = fnOpenFileToRead(YawPosFileName,&filesize_YawPos);
	//YawPosVec.assign(filesize_YawPos,0);
	//for(jj = 0; jj < filesize_YawPos; jj++) {
	//	ptrYawPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
	//	YawPosVec[jj] = outputBuffer;
	//}
	//ptrYawPos->close();

	//---------------------------------------------
	// LOAD PITCH POSITION
	//string PitchPosFileName("PitchPos.dat");
	//uInt32 filesize_PitchPos;
	//vector<float> PitchPosVec;
	//ifstream* ptrPitchPos = 0;
	//ptrPitchPos = fnOpenFileToRead(PitchPosFileName,&filesize_PitchPos);
	//PitchPosVec.assign(filesize_PitchPos,0);
	//for(jj = 0; jj < filesize_PitchPos; jj++) {
	//	ptrPitchPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
	//	PitchPosVec[jj] = outputBuffer;
	//}
	//ptrPitchPos->close();

	//cout << "1. Loading Yaw Jitter File." << endl;
	////---------------------------------------------
	//// LOAD YAW POSITION
	//string YawPosFileName("YawPos.dat");
	//uInt32 filesize_YawPos;
	//ifstream* ptrYawPos = 0;
	//ptrYawPos = fnOpenFileToRead(YawPosFileName,&filesize_YawPos);

	//float * YawPosVec;
	//YawPosVec = new float [ filesize_YawPos ]; 
	//ptrYawPos -> read( reinterpret_cast<char *>( &YawPosVec ) , filesize_YawPos * sizeof(float) );
	//ptrYawPos->close();

	//cout << "2. Loading Pitch Jitter File." << endl;
	////---------------------------------------------
	//// LOAD PITCH POSITION
	//string PitchPosFileName("PitchPos.dat");
	//uInt32 filesize_PitchPos;
	//ifstream* ptrPitchPos = 0;
	//ptrPitchPos = fnOpenFileToRead(PitchPosFileName,&filesize_PitchPos);

	//float * PitchPosVec;
	//PitchPosVec = new float [ filesize_PitchPos ];
	//ptrPitchPos -> read( reinterpret_cast<char *>( &PitchPosVec ) , filesize_PitchPos * sizeof(float) );
	//ptrPitchPos->close();
//Things to do:
// Need to add a way to re-read from a small x, y coordinate file since these values are repeated
//Add the input to the SCB to record a second channel and test the values recorded

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <NIDAQmx.h>

#include "AO_TimeStamp_RTInterp.h"
#include "FileCtrl.h"
#include "Interpolation.h"
#include "MenuReturnValues.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void AO_TimeStamp_RTInterp( MenuReturnValues mValues, int idx )
{
	bool32		done=0;
	char		errBuff[2048]={'\0'};
	int32		error=0;
	uInt32		jj = 0;
	uInt32		NZSample=0;
	double		NHalfBufs=0;
	int32		totalReadCh0=0;
	int32		totalReadCh1=0;
	uInt32		TotNFrames=0;
	float		outputBuffer = 0;
	int16		outputBuffer_16 = 0;

	// Initialize variables for the Counters, and Digital Input Timestamps
	int32		samplesPerChanRead = 0;
	int32		numSampsPerChan = mValues.slNumSampsPerChan; //This # is chosen using an estimated H1 firing rate of 500 spikes/s & => at least 4 seconds before acquiring 2000 samples
	uInt32		ReadBufferSize  = mValues.ulReadBufferSize; //This # is chosen based on 2 estimates: 1)Smallish frequency of disk writes, 2)H1 firing rate ~500 Spikes/s
	double		CIRecTimeout	= mValues.dCIRecTimeOut;
	ofstream*	ptrAIFileCh0	= 0;
	ofstream*	ptrAIFileCh1	= 0;
	string		strAIFileNameCh0;
	string		strAIFileNameCh1;

	// Initialize variables for the Analog Output (Z-signal for 608 oscilloscope)
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int16		NRepeats		= mValues.iNumRepeats;
	int16		NRasterPoints	= mValues.iNRasterPoints;
	int32 		NumAOSampWritten= 0;
	uInt32		NZSignalRepeat	= 1;
	ifstream*	ptrAOFile		= 0;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	string		strAOFileName;
	double		TotalHalfBufs	= 0;

	bool		AOAutoStart		= mValues.bStimAutoStart;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	float64		ActualSampRate	= 0;
	uInt32		AOBufferSpaceFree=0;
	uInt32		NFramesPerHalfBufer = 250;
	uInt32		AOOneChanBufSiz	= NRasterPoints * NFramesPerHalfBufer; // This is to accommodate 250 frames per half buffer
	uInt32		AOHalfBuf_Siz	= AOOneChanBufSiz * NumAOChannels * NZSignalRepeat + 250 * 3; //The last 3 is for a (X,Y,Z) point, included to permit (833*3+1)=2500 points in each frame
	uInt32		AOBuffer_Siz	= AOHalfBuf_Siz * 2;	//KEY FOR THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	
	
	int SingleChannel = mValues.iNumUEIAOChans;
	char PhysicalAOChannels[15];
	if (SingleChannel == 1)
		sprintf(PhysicalAOChannels, "/Dev2/ao%i", NumAOChannels-1);
	else if (SingleChannel == 0)
		sprintf(PhysicalAOChannels, "/Dev2/ao0:%i", NumAOChannels-1);
	else {
		cout << "Menu Option J Set Incorrectly" << endl;
		return;
	}

	// Create and initialize arrays and vectors
	vector<uInt32>	readArrayCICh0;
	vector<uInt32>	readArrayCICh1;
	vector<int16>	iAOBuffer;
	readArrayCICh0.assign(ReadBufferSize,0);
	readArrayCICh1.assign(ReadBufferSize,0);
	iAOBuffer.assign(AOBuffer_Siz,0);
	//RandomVals.assign(0,0); // SUVA
	

	// Initialize the handle to the NI tasks
	TaskHandle  CO1Handle = 0;
	TaskHandle  CO2Handle = 0;
	TaskHandle  AOHandle  = 0;

	//double val = 1.234567888765;
	//cout << fixed;
	//cout << setprecision(10) << val << endl;
	//cout << "This is to test the precision output to the screen: " << val << endl << endl;
	//cout << endl << endl;


	/*********************************************/	
	// Initialize the Response files
	/*********************************************/
	char fileIndex[65];
	itoa(idx,fileIndex,10);
	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "_Ch1.dat" << endl;
	strAIFileNameCh0 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch1.dat";
	ptrAIFileCh0	 = fnOpenFileToWrite( strAIFileNameCh0 );

	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "_Ch2.dat" << endl;
	strAIFileNameCh1 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch2.dat";
	ptrAIFileCh1	 = fnOpenFileToWrite( strAIFileNameCh1 );


	cout << "1. Loading World Map File." << endl;
	//---------------------------------------------
	// LOAD WORLD MAP
	string WorldMapFileName;
	WorldMapFileName = mValues.strStimFileDirPath + "\\" + "WorldMap.dat";
	uInt32 filesize_WorldMapVec;
	ifstream* ptrWorldMapVec = 0;
	ptrWorldMapVec = fnOpenFileToRead(WorldMapFileName,&filesize_WorldMapVec);

	float* WorldMapVec;
	WorldMapVec = new float [ filesize_WorldMapVec ];
	ptrWorldMapVec->read(reinterpret_cast<char *> (WorldMapVec), filesize_WorldMapVec * sizeof (float) );
	ptrWorldMapVec->close();
	float Height= sqrt( (float) filesize_WorldMapVec);
	float Width = sqrt( (float) filesize_WorldMapVec);

	cout << "2. Loading Yaw Jitter File." << endl;
	//---------------------------------------------
	// LOAD YAW POSITION
	string YawPosFileName;
	YawPosFileName = mValues.strStimFileDirPath + "\\" + "YawPos.dat";
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

	cout << "3. Loading Pitch Jitter File." << endl;
	//---------------------------------------------
	// LOAD PITCH POSITION
	string PitchPosFileName;
	PitchPosFileName = mValues.strStimFileDirPath + "\\" + "PitchPos.dat";
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

	cout << "4. Loading Roll Jitter File." << endl;
	//---------------------------------------------
	// LOAD ROLL POSITION
	string RollPosFileName;
	RollPosFileName = mValues.strStimFileDirPath + "\\" + "RollPos.dat";
	uInt32 filesize_RollPos;
	vector<float> RollPosVec;
	ifstream* ptrRollPos = 0;
	ptrRollPos = fnOpenFileToRead(RollPosFileName,&filesize_RollPos);
	RollPosVec.assign(filesize_YawPos,0);
	for(jj = 0; jj < filesize_YawPos; jj++) {
		ptrRollPos -> read( reinterpret_cast<char *>( &outputBuffer ) , sizeof(float) );
		RollPosVec[jj] = outputBuffer;
	}
	ptrPitchPos->close();

	cout << "5. Loading Sampling Array Yaw Positions." << endl;
	//---------------------------------------------
	// 4. Loading Sampling Array Yaw Positions.
	string LED_XPosFileName;
	LED_XPosFileName = mValues.strStimFileDirPath + "\\" + "LED_XPos.dat";
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

	cout << "6. Loading Sampling Array Pitch Positions." << endl;
	//---------------------------------------------
	// 5. Loading Sampling Array Pitch Positions.
	string LED_YPosFileName;
	LED_YPosFileName = mValues.strStimFileDirPath + "\\" + "LED_YPos.dat";
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


	cout << "7. Loading X Pixels Positions." << endl;
	//---------------------------------------------
	// 6. Loading X Pixels Positions.
	ifstream* ptrXPixelPos = 0;
	uInt32 filesize_XPixelPos = 0;
	string strAO_X_FileName;
	vector<int16> X;

	strAO_X_FileName = mValues.strStimFileDirPath + "\\" + "XCoords.dat";
	ptrXPixelPos = fnOpenFileToRead_int16(strAO_X_FileName, &filesize_XPixelPos); //Call function to open Analog Output file
	X.assign(filesize_XPixelPos,0);
	for( uInt32 idx2=0; idx2<filesize_XPixelPos; idx2++ ) {
		ptrXPixelPos -> read( reinterpret_cast<char *>( &outputBuffer_16 ), sizeof(int16));
		X[idx2] = outputBuffer_16;
	}
	ptrXPixelPos->close();

	cout << "8. Loading Y Pixels Positions." << endl;
	//---------------------------------------------
	// 7. Loading Y Pixels Positions.
	ifstream* ptrYPixelPos = 0;
	uInt32 filesize_YPixelPos = 0;
	string strAO_Y_FileName;
	vector<int16> Y;

	strAO_Y_FileName = mValues.strStimFileDirPath + "\\" + "YCoords.dat";	//Name of Analog Stimulus file
	ptrYPixelPos = fnOpenFileToRead_int16(strAO_Y_FileName, &filesize_YPixelPos); //Call function to open Analog Output file
	Y.assign(filesize_YPixelPos,0);
	for( uInt32 idx2=0; idx2<filesize_YPixelPos; idx2++ ) {
		ptrYPixelPos->read( reinterpret_cast<char *>( &outputBuffer_16 ), sizeof(int16));
		Y[idx2] = outputBuffer_16;
	}
	ptrYPixelPos->close();


	/*********************************************/	
	// Load Analog Output Buffer
	/*********************************************/
	//TotalHalfBufs = ceil( ( filesize_YawPos / NFramesPerHalfBufer ) * NRepeats ); //SUVA
	//int * RandomVals; // SUVA (pointer to a dynamic array) //SUVA
	//int space  = (NRasterPoints+1)*NFramesPerHalfBufer*TotalHalfBufs; //SUVA
	//RandomVals = unsigned short int[space]; // SUVA (2 bytes, values from 0 to 2*32767)  
	clock_t start;
	start = clock();
	NZSample = ConstructAOBuffer_RT_int16( &iAOBuffer[0],
		AOOneChanBufSiz * 2, filesize_YawPos, filesize_XPixelPos, NZSignalRepeat, TotNFrames, NumAOChannels,
		&X[0], &Y[0], &YawPosVec[0], &PitchPosVec[0], &RollPosVec[0], &LED_XPos[0], &LED_YPos[0],
		WorldMapVec, Height, Width); // Suva_20130830
	
	TotNFrames = TotNFrames + NZSample + 1;
	cout << "Number of frames preloaded into AO buffer: " << TotNFrames << endl << endl;
	NHalfBufs += 2;
	start = clock()-start;
	printf("%f seconds",(float)(start)/CLOCKS_PER_SEC);
	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	/*********************************************/
	// DAQmx Configure Code for Counter 1 - Timestamp channel 1
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO1",&CO1Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO1Handle,"Dev2/ctr0","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO1Handle,"/Dev2/PFI9",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,2000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO1Handle,"","/Dev2/100kHzTimebase"));

	/*********************************************/
	// DAQmx Configure Code for Counter 2 - Timestamp channel 2
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO2",&CO2Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO2Handle,"Dev2/ctr1","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO2Handle,"/Dev2/PFI4",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,2000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO2Handle,"","/Dev2/100kHzTimebase"));
	DAQmxErrChk (DAQmxGetSampClkRate(CO2Handle, &ActualSampRate));	//Read the actual sample clock rate (eventually coerced depending on the hardware used).
	cout << "The time stamp rate is: " << ActualSampRate << endl << endl;

	/*********************************************/
	// DAQmx Configure Code for Analog Output
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("AO",&AOHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOHandle,PhysicalAOChannels,"",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOHandle,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,AOOneChanBufSiz*NZSignalRepeat));
	DAQmxErrChk (DAQmxCfgOutputBuffer(AOHandle,AOBuffer_Siz/NumAOChannels)); // LOOK AT THIS!@!! Made this change and need to check. Switched AOHalfBuf_Siz for AOOneChanBufSiz
	DAQmxErrChk (DAQmxSetWriteRegenMode(AOHandle,DAQmx_Val_DoNotAllowRegen));
	DAQmxErrChk (DAQmxGetSampClkRate(AOHandle, &ActualSampRate));	//Read the actual sample clock rate (eventually coerced depending on the hardware used).
	cout << "The actual sample clock rate is: " << ActualSampRate << endl << endl;

	/*********************************************/
	// DAQmx Configure Start Trigger
	/*********************************************/
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(AOHandle,"/Dev2/PFI6",DAQmx_Val_Rising));
	
	DAQmxSetArmStartTrigType(CO1Handle,DAQmx_Val_DigEdge);
	DAQmxSetDigEdgeArmStartTrigSrc(CO1Handle,"/Dev2/PFI6");
	DAQmxSetDigEdgeArmStartTrigEdge(CO1Handle,DAQmx_Val_Rising);
	
	DAQmxSetArmStartTrigType(CO2Handle,DAQmx_Val_DigEdge);
	DAQmxSetDigEdgeArmStartTrigSrc(CO2Handle,"/Dev2/PFI6");
	DAQmxSetDigEdgeArmStartTrigEdge(CO2Handle,DAQmx_Val_Rising);


	/*********************************************/
	// DAQmx Initial Analog Output Write Code
	/*********************************************/
	//DAQmxErrChk (DAQmxWriteBinaryI16(AOHandle,AOOneChanBufSiz*NZSignalRepeat*2,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByScanNumber,&iAOBuffer[0],&NumAOSampWritten,NULL));
	DAQmxErrChk (DAQmxWriteBinaryI16(AOHandle,AOBuffer_Siz/3,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByScanNumber,&iAOBuffer[0],&NumAOSampWritten,NULL));
	

	/*********************************************/
	// Everything is prepared, start operation
	/*********************************************/
	cout << "Continuously Producing Raster for Tektronix 608." << endl;
	cout << "Continuously Timestamping Data." << endl;
	cout << "Press any key to TERMINATE trial." << endl << endl;
	
	TotalHalfBufs = ( ( filesize_YawPos / NFramesPerHalfBufer ) * NRepeats );
	TotalHalfBufs = ceil( TotalHalfBufs ) + 0; 
	cout << "Number of half buffers to play: " << TotalHalfBufs << endl;

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOHandle));
	DAQmxErrChk (DAQmxStartTask(CO2Handle));
	DAQmxErrChk (DAQmxStartTask(CO1Handle));

	cout << "Reached main while loop." << endl;

	// CREATE FILE TO WRITE INTERPOLATED VALUES
	//ofstream* ptrInterpPixelVals = 0;
	//ptrInterpPixelVals = fnOpenFileToWrite( "C:\\Data\\Physiology\\FrameValues.dat" );
	//cout << "File for interpolated values opened." << endl;


	while ( !done && !kbhit() && NHalfBufs < TotalHalfBufs )
	{
		//********************************************
		// DAQmx record the digital events
		//********************************************
		DAQmxErrChk (DAQmxReadCounterU32(CO1Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh0[0],ReadBufferSize,&samplesPerChanRead,NULL));
		if (samplesPerChanRead > 0) {
			totalReadCh0 += samplesPerChanRead;
			ptrAIFileCh0->write(reinterpret_cast<char*>(&readArrayCICh0[0]),sizeof(uInt32)*samplesPerChanRead);
			if (totalReadCh0%numSampsPerChan == 0) {
				cout << "Total # of Spikes Recorded on Ch 1: \t" << totalReadCh0 << endl;
			}
		}

		DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh1[0],ReadBufferSize,&samplesPerChanRead,NULL));
		if (samplesPerChanRead > 0) {
			totalReadCh1 += samplesPerChanRead;
			ptrAIFileCh1->write(reinterpret_cast<char*>(&readArrayCICh1[0]),sizeof(uInt32)*samplesPerChanRead);
			if (totalReadCh1%numSampsPerChan == 0) {
				cout << "Total # of Spikes Recorded on Ch 2: \t" << totalReadCh1 << endl;
			}
		}

		//********************************************
		// DAQmx Check output buffer space available
		//********************************************
		DAQmxErrChk (DAQmxGetWriteSpaceAvail(AOHandle, &AOBufferSpaceFree));
		if(AOBufferSpaceFree > AOOneChanBufSiz) {
			++NHalfBufs;
			
			NZSample = ConstructAOBuffer_RT_int16( &iAOBuffer[0],
				AOOneChanBufSiz, filesize_YawPos, filesize_XPixelPos, NZSignalRepeat, TotNFrames, NumAOChannels,
				&X[0], &Y[0], &YawPosVec[0], &PitchPosVec[0], &RollPosVec[0], &LED_XPos[0], &LED_YPos[0],
				WorldMapVec, Height, Width);

			TotNFrames = TotNFrames + NZSample + 1;

			DAQmxErrChk (DAQmxWriteBinaryI16(AOHandle,AOHalfBuf_Siz/3,0,1,DAQmx_Val_GroupByScanNumber,&iAOBuffer[0],&NumAOSampWritten,NULL));

			cout << NHalfBufs << " ; AO Buffer Space: " << AOBufferSpaceFree << " ; AO Samples Written: " << NumAOSampWritten << endl << endl;
			//ptrInterpPixelVals->write( reinterpret_cast<char *> (&iAOBuffer[0]), AOHalfBuf_Siz * sizeof (int16) );
		}
	}

	getchar();
	cout << "Number of half buffer transfered: " << NHalfBufs << endl;
	cout << "Number of samples transfered : " << TotNFrames << endl;
	


Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( CO1Handle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(CO1Handle);
		DAQmxClearTask(CO1Handle);
	}
	if( CO2Handle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(CO2Handle);
		DAQmxClearTask(CO2Handle);
	}
	if( AOHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(AOHandle);
		DAQmxClearTask(AOHandle);
	}

	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	ptrAIFileCh0->close();
	ptrAIFileCh1->close();
	//ptrInterpPixelVals->close();

	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	return;

}
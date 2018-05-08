//Things to do:
// Need to add a way to re-read from a small x, y coordinate file since these values are repeated
//Add the input to the SCB to record a second channel and test the values recorded

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <NIDAQmx.h>

#include "AO.h"
#include "FileCtrl.h"
#include "MenuReturnValues.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void AO( MenuReturnValues mValues, int idx )
{
	bool32		done=0;
	char		errBuff[2048]={'\0'};
	int32		totalReadCh0=0;
	int32		totalReadCh1=0;
	int32		error=0;
	uInt32		NHalfBufs=0;
	uInt32		NZSample=0;
	uInt32		TotNZSample=0;

	// Initialize variables for the Analog Output (Z-signal for 608 oscilloscope)
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int16		NRepeats		= mValues.iNumRepeats;
	int32 		NumAOSampWritten= 0;
	ifstream*	ptrAOFile		= 0;
	uInt32		AOOneChanBufSiz	= StimSampRate;
	uInt32		AOBuffer_Siz	= AOOneChanBufSiz*NumAOChannels*2;	//KEY FOR THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	uInt32		AOHalfBuf_Siz	= AOOneChanBufSiz*NumAOChannels;
	uInt32		AOBufferSpaceFree=0;
	uInt32		AOFileSize		= 0;
	string		strAOFileName;

	// Create and initialize arrays and vectors
	vector<uInt16>	dAOBuffer;
	dAOBuffer.assign(AOBuffer_Siz,0);

	// Initialize the handle to the NI tasks
	TaskHandle  AOHandle  = 0;

	/*********************************************/	
	// Initialize the Stimulus file (Z-signal)
	/*********************************************/
	strAOFileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0 + ".dat";	//Name of Analog Stimulus file
	ptrAOFile = fnOpenFileToRead_uInt16(strAOFileName, &AOFileSize);	//Call function to open Analog Output file

	/*********************************************/	
	// Load Analog Output Buffer
	/*********************************************/
	NZSample = ConstructAOBuffer_uInt16(&dAOBuffer[0],AOOneChanBufSiz,AOFileSize,ptrAOFile);
	TotNZSample += NZSample;
	cout << "Number of Z samples written: " << TotNZSample << endl;

	if ((TotNZSample % AOFileSize) == 0)
		ptrAOFile->seekg(0, ios::beg );
	cout << "Z reset" << endl;


	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	/*********************************************/
	// DAQmx Configure Code for Analog Output
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("AO",&AOHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOHandle,"/Dev2/ao0","",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOHandle,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,AOOneChanBufSiz));
	DAQmxErrChk (DAQmxCfgOutputBuffer(AOHandle,AOOneChanBufSiz*2)); // LOOK AT THIS!@!! Made this change and need to check. Switched AOHalfBuf_Siz for AOOneChanBufSiz
	DAQmxErrChk (DAQmxSetWriteRegenMode(AOHandle,DAQmx_Val_DoNotAllowRegen));

	/*********************************************/
	// DAQmx Configure Start Trigger
	/*********************************************/
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(AOHandle,"/Dev2/PFI6",DAQmx_Val_Rising));

	/*********************************************/
	// DAQmx Initial Analog Output Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteBinaryU16(AOHandle,AOOneChanBufSiz,0,1,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));


	/*********************************************/
	// Everything is prepared, start operation
	/*********************************************/
	cout << "Press any key to terminate trial." << endl << endl;
	cout << "Number of half buffers to play: " << (AOFileSize/AOOneChanBufSiz*NRepeats) << endl;

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOHandle));

	while ( !done && !kbhit() && NHalfBufs < (AOFileSize/AOOneChanBufSiz*NRepeats) )
	{
		//********************************************
		// DAQmx Check output buffer space available
		//********************************************
		DAQmxErrChk (DAQmxGetWriteSpaceAvail(AOHandle, &AOBufferSpaceFree));
		if(AOBufferSpaceFree > AOOneChanBufSiz) {  // Made this change and need to check. Switched AOHalfBuf_Siz for AOOneChanBufSiz
			++NHalfBufs;
			NZSample = ConstructAOBuffer_uInt16(&dAOBuffer[0],AOOneChanBufSiz,AOFileSize,ptrAOFile);

			DAQmxErrChk (DAQmxWriteBinaryU16(AOHandle,AOOneChanBufSiz,0,1,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));

			TotNZSample += NZSample;
			cout << "AO Buffer Space: " << AOBufferSpaceFree << " ; AO Samples Written: " << NumAOSampWritten*NumAOChannels << endl << endl;

			if ((TotNZSample % AOFileSize) == 0)
				ptrAOFile->seekg(0, ios::beg );
			cout << "Z reset" << endl;
		}
	}
	getchar();
	cout << "Number of half buffer transfered: " << NHalfBufs << endl;
	cout << "Number of samples transfered : " << TotNZSample << endl;

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
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
	ptrAOFile->close();

	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	return;

}
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

#include "AOContDITimeStamp.h"
#include "FileCtrl.h"
#include "MenuReturnValues.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void AOContDITimeStamp( MenuReturnValues mValues, int idx )
{
	bool32		done=0;
	char		errBuff[2048]={'\0'};
	int32		totalReadCh0=0;
	int32		totalReadCh1=0;
	int32		error=0;

	// Initialize variables for the Counters, and Digital Input Timestamps
	int32		samplesPerChanRead;
	int32		numSampsPerChan = mValues.slNumSampsPerChan; //This # is chosen using an estimated H1 firing rate of 500 spikes/s & => at least 4 seconds before acquiring 2000 samples
	uInt32		ReadBufferSize  = mValues.ulReadBufferSize; //This # is chosen based on 2 estimates: 1)Smallish frequency of disk writes, 2)H1 firing rate ~500 Spikes/s
	double		CIRecTimeout	= mValues.dCIRecTimeOut;
	ofstream*	ptrAIFileCh0	= 0;
	ofstream*	ptrAIFileCh1	= 0;
	string		strAIFileNameCh0;
	string		strAIFileNameCh1;

	// Initialize variables for the Analog Output (Z-signal for 608 oscilloscope)
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int32 		NumAOSampWritten= 0;
	ifstream*	ptrAOFile		= 0;
	uInt32		AOOneChanBufSiz	= 2000000;
	uInt32		AOBuffer_Siz	= AOOneChanBufSiz*NumAOChannels*2;	//KEY FOR THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	uInt32		AOHalfBuf_Siz	= AOBuffer_Siz/2;
	uInt32		AOBufferSpaceAvail=0;
	uInt64		AOFileSize		= 0;
	string		strAOFileName;

	// Initialize the handle to the NI tasks
	TaskHandle  CO1Handle = 0;
	TaskHandle  CO2Handle = 0;
	TaskHandle  AOHandle  = 0;

	// Create and initialize arrays and vectors 
	vector<uInt32>	readArrayCICh0;
	vector<uInt32>	readArrayCICh1;
	vector<double>	dAOBuffer;
	readArrayCICh0.assign(ReadBufferSize,0);
	readArrayCICh1.assign(ReadBufferSize,0);
	dAOBuffer.assign(AOBuffer_Siz,0);


	/*********************************************/	
	// Initialize the Response files
	/*********************************************/
	char fileIndex[3];
	itoa(idx,fileIndex,5);
	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "_Ch1.dat" << endl;
	strAIFileNameCh0 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch1.dat";
	ptrAIFileCh0	 = fnOpenFileToWrite( strAIFileNameCh0 );

	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "Ch2.dat" << endl;
	strAIFileNameCh1 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch2.dat";
	ptrAIFileCh1	 = fnOpenFileToWrite( strAIFileNameCh1 );


	/*********************************************/	
	// Initialize the Stimulus file (Z-signal)
	/*********************************************/
	strAOFileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0 + ".dat";	//Name of Analog Stimulus file
	ptrAOFile = fnOpenLargeFileToRead(strAOFileName,&AOFileSize);	//Call function to open Analog Output file
	LoadFullAOBuffer(&dAOBuffer[0],NumAOChannels,AOOneChanBufSiz*2,ptrAOFile); //The first time this should load the full buffer


	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev2"));


	/*********************************************/
	// DAQmx Configure Code for Counter 1 - Timestamp channel 1
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO1",&CO1Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO1Handle,"Dev2/ctr0","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO1Handle,"/Dev2/PFI9",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,5000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO1Handle,"","/Dev2/100kHzTimebase"));


	/*********************************************/
	// DAQmx Configure Code for Counter 2 - Timestamp channel 2
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO2",&CO2Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO2Handle,"Dev2/ctr1","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO2Handle,"/Dev2/PFI4",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,5000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO2Handle,"","/Dev2/100kHzTimebase"));


	/*********************************************/
	// DAQmx Configure Code for Analog Output
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("AO",&AOHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOHandle,"/Dev2/ao0:2","",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOHandle,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,AOOneChanBufSiz));
	DAQmxErrChk (DAQmxCfgOutputBuffer (AOHandle,AOBuffer_Siz));
	DAQmxErrChk (DAQmxSetWriteRegenMode(AOHandle,DAQmx_Val_DoNotAllowRegen));
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(AOHandle,"/Dev2/PFI6",DAQmx_Val_Rising));


	/*********************************************/
	// DAQmx Initial Analog Output Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,AOOneChanBufSiz,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));
  

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOHandle));
	DAQmxErrChk (DAQmxStartTask(CO2Handle));
	DAQmxErrChk (DAQmxStartTask(CO1Handle));


	/*********************************************/
	// Everything is prepared, start operation
	/*********************************************/
	cout << "Continuously Timestamping Data." << endl;
	cout << "Continuously Producing Raster for Tektronix 608." << endl;
	cout << "Press any key to terminate trial." << endl;
	while ( !done && !kbhit() )
	{
		//********************************************
		// DAQmx Check available space code
		//********************************************
		DAQmxErrChk (DAQmxReadCounterU32(CO1Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh0[0],ReadBufferSize,&samplesPerChanRead,NULL));//numSampsPerChan
		if (samplesPerChanRead > 0)
		{
			totalReadCh0 += samplesPerChanRead;
			ptrAIFileCh0->write(reinterpret_cast<char*>(&readArrayCICh0[0]),sizeof(uInt32)*samplesPerChanRead);
			if (totalReadCh0%numSampsPerChan == 0)
			{
				cout << "Total # of Spikes Recorded on Ch 1: \t" << totalReadCh0 << endl;
			}
		}

		DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh1[0],ReadBufferSize,&samplesPerChanRead,NULL));//numSampsPerChan
		if (samplesPerChanRead > 0)
		{
			totalReadCh1 += samplesPerChanRead;
			ptrAIFileCh1->write(reinterpret_cast<char*>(&readArrayCICh1[0]),sizeof(uInt32)*samplesPerChanRead);
			if (totalReadCh1%numSampsPerChan == 0)
			{
				cout << "Total # of Spikes Recorded on Ch 2: \t" << totalReadCh1 << endl;
			}
		}

		//********************************************
		// DAQmx Check output buffer space available
		//********************************************
		DAQmxErrChk (DAQmxGetWriteSpaceAvail(AOHandle, &AOBufferSpaceAvail));
		if(AOBufferSpaceAvail > AOHalfBuf_Siz)
		{
			cout << "AO Buffer space available : \t" << AOBufferSpaceAvail << endl;
			LoadFullAOBuffer(&dAOBuffer[0],NumAOChannels,AOOneChanBufSiz,ptrAOFile);
			DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,AOOneChanBufSiz,0,-1,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));
			cout << "Number of AO Samples Per Channel Written : \t" << NumAOSampWritten << endl << endl;
		}

	}
	getchar();


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
	ptrAOFile->close();
	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	return;

}
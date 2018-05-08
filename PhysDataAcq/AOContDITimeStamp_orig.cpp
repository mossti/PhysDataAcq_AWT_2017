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
#include "UEILEDCtrl.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void AOContDITimeStamp( MenuReturnValues mValues, int idx )
{
	bool32		done=0;
	char		errBuff[2048]={'\0'};
	int32		totalRead=0;
	int32		error=0;

	// Initialize variables for the Digital Output, Counters, and Digital Input Timestamps
	int32		samplesPerChanRead;
	int32		DOArraySize		= mValues.slDOArraySize;
	int32		numSampsPerChan = mValues.slNumSampsPerChan; //This # is chosen using an estimated H1 firing rate of 500 spikes/s & => at least 4 seconds before acquiring 2000 samples
	uInt32		ReadBufferSize  = mValues.ulReadBufferSize; //This # is chosen based on 2 estimates: 1)Smallish frequency of disk writes, 2)H1 firing rate ~500 Spikes/s
	double		CIRecTimeout	= mValues.dCIRecTimeOut;
	ofstream*	ptrAIFile		= 0;
	string		strAIFileName;

	// Initialize variables for the Analog Output (Z-signal for 608 oscilloscope)
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int32 		NumAOSampWritten= 0;
	uInt32		writeBufferSpaceAvail=0;
	uInt32		SampPerChan_StimBufSpace= 1000000;	//4000000// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	uInt32		SampPerChan_HalfStimBuf	= SampPerChan_StimBufSpace/2;
	uInt64		AOOutputFileSize= 0;
	//ifstream::pos_type	AOOutputFileSize= 0;
	ifstream*	ptrAOFile		= 0;
	string		strAOFileName;

	TaskHandle  DOHandle  = 0;
	TaskHandle  CO1Handle = 0;
	TaskHandle  CO2Handle = 0;
	TaskHandle  AOHandle  = 0;

	vector<uInt32>	readArray;
	vector<uInt8>	ulDOWriteArray;
	vector<double>	dAOBuffer;
	readArray.assign(ReadBufferSize,0);
	ulDOWriteArray.assign(DOArraySize,0);
	dAOBuffer.assign(SampPerChan_StimBufSpace,0);

	/*********************************************/
	// Digital Output Array for Clocking Tektronix AFG320
	/*********************************************/
	/*
	This array is 200 samples long. These samples are played at 500 microseconds/sample,
	therefore at 100,000 samples/sec.
	*/
	for(int q=0;q<DOArraySize;q++){
		if (q%2 == 0){
			ulDOWriteArray[q] = 0;
		}
		else if (q%2 != 0 ){
			ulDOWriteArray[q] = 1; // 65535
		}
		else
			ulDOWriteArray[q] = 0;
	}
	//for(int q=0;q<DOArraySize;q++){
	//	if (q<=(DOArraySize/2)){
	//		ulDOWriteArray[q] = 1;
	//	}
	//	else if (q>(DOArraySize/2) ){
	//		ulDOWriteArray[q] = 0; // 65535
	//	}
	//	else
	//		ulDOWriteArray[q] = 0;
	//}

	/*********************************************/	
	// Initialize the Response file
	/*********************************************/
	char fileIndex[3];
	itoa(idx,fileIndex,5);
	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << ".dat" << endl;
	strAIFileName=mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + ".dat";
	ptrAIFile = fnOpenFileToWrite( strAIFileName );


	/*********************************************/	
	// Initialize the Stimulus file (Z-signal)
	/*********************************************/
	strAOFileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0 + ".dat";	//Name of Analog Stimulus file
	ptrAOFile = fnOpenLargeFileToRead(strAOFileName,&AOOutputFileSize);	//Call function to open Analog Output file
	LoadFullAOBuffer(&dAOBuffer[0],NumAOChannels,SampPerChan_HalfStimBuf,ptrAOFile);


	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev1"));


	/*********************************************/
	// DAQmx Configure Code for Counter 1 - Timebase for DO trigger signal
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO1",&CO1Handle));
	DAQmxErrChk (DAQmxCreateCOPulseChanTicks(CO1Handle,"Dev1/ctr0","","100kHzTimebase",DAQmx_Val_Low,0,50,50));
	DAQmxErrChk (DAQmxCfgImplicitTiming(CO1Handle,DAQmx_Val_ContSamps,100000));


	/*********************************************/
	// DAQmx Configure Digital Out Code - DO signal timing sequence
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("DO",&DOHandle));
	DAQmxErrChk (DAQmxCreateDOChan(DOHandle,"Dev1/port0/line0","",DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxCfgSampClkTiming(DOHandle,"/Dev1/Ctr0InternalOutput",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,100000));


	/*********************************************/
	// DAQmx Configure Code for Counter 2 - Timestamp	
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO2",&CO2Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO2Handle,"Dev1/ctr1","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO2Handle,"/Dev1/PFI4",1000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO2Handle,"","/Dev1/100kHzTimebase"));


	/*********************************************/
	// DAQmx Configure Code for Analog Output
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("AO",&AOHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOHandle,"/Dev1/ao0","",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOHandle,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SampPerChan_HalfStimBuf));
	DAQmxErrChk (DAQmxCfgOutputBuffer (AOHandle,SampPerChan_StimBufSpace));
	DAQmxErrChk (DAQmxSetWriteRegenMode(AOHandle,DAQmx_Val_DoNotAllowRegen));
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(AOHandle,"/Dev1/PFI6",DAQmx_Val_Rising));


	/*********************************************/
	// DAQmx Digital Trigger Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteDigitalLines(DOHandle,DOArraySize,0,10.0,DAQmx_Val_GroupByScanNumber,&ulDOWriteArray[0],NULL,NULL));


	/*********************************************/
	// DAQmx Initial Analog Output Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,SampPerChan_HalfStimBuf,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));
  

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOHandle));
	DAQmxErrChk (DAQmxStartTask(CO2Handle));
	DAQmxErrChk (DAQmxStartTask(DOHandle));
	DAQmxErrChk (DAQmxStartTask(CO1Handle));


	cout << "Continuously Generating Clock Signals." << endl;
	cout << "Continuously Timestamping Data." << endl;
	cout << "Continuously Producing Z-signal for 608 Oscilloscope." << endl;
	cout << "Press any key to terminate trial." << endl;

	//UEILEDCtrl( mValues, CO2Handle );
	while ( !done && !kbhit() )
	{
		//********************************************
		// DAQmx Check available space code
		//********************************************
		DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,DAQmx_Val_Auto,CIRecTimeout,&readArray[0],ReadBufferSize,&samplesPerChanRead,NULL));//numSampsPerChan
		if (samplesPerChanRead > 0)
		{
			totalRead += samplesPerChanRead;
			//cout << "Total Number of Spikes Recorded : \t" << totalRead << endl;
			ptrAIFile->write(reinterpret_cast<char*>(&readArray[0]),sizeof(uInt32)*samplesPerChanRead);
			if (totalRead%numSampsPerChan == 0)
			{
				cout << "Total Number of Spikes Recorded : \t" << totalRead << endl;
			}
		}

		//********************************************
		// DAQmx Check output buffer space available
		//********************************************
		DAQmxErrChk (DAQmxGetWriteSpaceAvail(AOHandle, &writeBufferSpaceAvail));
		if(writeBufferSpaceAvail > SampPerChan_HalfStimBuf)
		{
			cout << "AO Buffer space available : \t" << writeBufferSpaceAvail << endl;
			LoadFullAOBuffer(&dAOBuffer[0],NumAOChannels,SampPerChan_HalfStimBuf,ptrAOFile);
			DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,SampPerChan_HalfStimBuf,0,-1,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));
			cout << "Number of AO Samples Written : \t" << NumAOSampWritten << endl << endl;
		}

	}
	getchar();


Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);

	if( DOHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(DOHandle);
		DAQmxClearTask(DOHandle);
	}
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
	ptrAIFile->close();
	ptrAOFile->close();
	DAQmxErrChk (DAQmxResetDevice("Dev1"));

	return;

}